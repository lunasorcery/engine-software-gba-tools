#include <cstdio>
#include <cstdint>
#include <vector>
#include <fmt/core.h>
#include "common-gba.h"
#include "misc.h"

int main(int argc, char const* const* argv)
{
	if (argc <= 1)
	{
		fmt::print(stderr, "Expected at least one arg! usage:\n");
		fmt::print(stderr, "gbafind romfile.gba [romfile2.gba, ...]\n");
		exit(1);
	}

	int const kMaxSongLength = 0xff;
	int const kMaxSongCount = 0xff;

	FILE* fh = nullptr;
	for (int argIndex = 1; argIndex < argc; ++argIndex)
	{
		if (fh)
		{
			fclose(fh);
			fh = nullptr;
		}

		char const* filepath = argv[argIndex];
		bool didPrintName = false;

		fh = fopen(filepath, "rb");
		if (!fh)
			continue;

		fseek(fh, 0, SEEK_END);
		size_t const fileLength = ftell(fh);
		fseek(fh, 0, SEEK_SET);

		for (uint32_t baseAddr = 0; baseAddr < fileLength; baseAddr += 4)
		{
			fseek(fh, baseAddr, SEEK_SET);

			gba_musicbank_header_t header;
			fread(&header, sizeof(gba_musicbank_header_t), 1, fh);
			if (header.version != 0x0121)
				continue;
			if (header.instrumentCount == 0)
				continue;
			if (header.songCount == 0)
				continue;

			uint32_t songOffsets[kMaxSongCount];
			int const songOffsetsRead = fread(songOffsets, sizeof(uint32_t), header.songCount, fh);
			if (songOffsetsRead != header.songCount)
				continue;

			bool instsValid = true;
			for (int inst = 0; instsValid && inst < header.instrumentCount; ++inst)
			{
				gba_instrument_header_t instHeader;
				int const instHeaderRead = fread(&instHeader, sizeof(instHeader), 1, fh);
				instsValid &= (instHeaderRead == 1);
				instsValid &= (instHeader.volumeEnvelope.pointCount <= 12);
				instsValid &= (instHeader.panningEnvelope.pointCount <= 12);

				int const seekResult = fseek(fh, instHeader.sampleLength, SEEK_CUR);
				instsValid &= (seekResult == 0);
				falign(fh, 4);
			}
			if (!instsValid)
				continue;

			// instruments shouldn't overshoot and overlap the songs
			if (ftell(fh)-baseAddr > songOffsets[0])
				continue;

			// song offsets should be sorted
			bool songOffsetsAreMonotonic = true;
			for (int i = 1; i < header.songCount; ++i)
				songOffsetsAreMonotonic &= (songOffsets[i-1] < songOffsets[i]);
			if (!songOffsetsAreMonotonic)
				continue;

			bool songsValid = true;
			for (int song = 0; songsValid && song < header.songCount; ++song)
			{
				// songs should be 4byte aligned
				songsValid &= (songOffsets[song] == (songOffsets[song] & 0xfffffffcu));
				songsValid &= (songOffsets[song] < fileLength);

				int const seekResult = fseek(fh, baseAddr+songOffsets[song], SEEK_SET);
				songsValid &= (seekResult == 0);

				gba_song_header_t songHeader;
				int const songHeaderRead = fread(&songHeader, sizeof(songHeader), 1, fh);
				songsValid &= (songHeaderRead == 1);
				songsValid &= (songHeader.channelCount != 0);
				songsValid &= (songHeader.songLength != 0);
				songsValid &= (songHeader.patternCount != 0);
				songsValid &= (songHeader.tickrate != 0);
				songsValid &= (songHeader.tempo != 0);

				falign(fh, 4);
				uint8_t patternOrder[kMaxSongLength];
				int const patternOrderRead = fread(patternOrder, 1, songHeader.songLength, fh);
				songsValid &= (patternOrderRead == songHeader.songLength);

				for (int i = 0; i < songHeader.songLength; ++i)
					songsValid &= (patternOrder[i] < songHeader.patternCount);
			}
			if (!songsValid)
				continue;

			if (!didPrintName)
				fmt::print("\n{}\n", filepath);
			didPrintName = true;

			fmt::print(
				"@ {:06x}: v{:04x} {} instruments, {} songs\n",
				baseAddr,
				header.version,
				header.instrumentCount,
				header.songCount
			);
		}

		fclose(fh);
	}
}
