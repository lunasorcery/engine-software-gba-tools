#include <fmt/core.h>
#include "misc.h"
#include "common-gba.h"

GBAMusicBank::GBAMusicBank(FILE* fh, size_t baseAddr)
{
	fseek(fh, baseAddr, SEEK_SET);

	gba_musicbank_header_t const bankHeader = read<gba_musicbank_header_t>(fh);

	std::vector<uint32_t> songOffsets = readArray<uint32_t>(fh, bankHeader.songCount);

	this->instruments.resize(bankHeader.instrumentCount);

	for (int instrIndex = 0; instrIndex < bankHeader.instrumentCount; ++instrIndex)
	{
		GBAInstrument& instrument = this->instruments[instrIndex];
		{
			instrument.header = read<gba_instrument_header_t>(fh);
			instrument.sample = readArray<int8_t>(fh, instrument.header.sampleLength);
			falign(fh, 4);
		}
	}

	this->songs.resize(bankHeader.songCount);

	for (uint32_t songIndex = 0; songIndex < bankHeader.songCount; ++songIndex)
	{
		fseek(fh, baseAddr+songOffsets[songIndex], SEEK_SET);

		GBASong& song = this->songs[songIndex];
		{
			song.header = read<gba_song_header_t>(fh);
			falign(fh, 4);

			song.patternOrder = readArray<uint8_t>(fh, song.header.songLength);
			falign(fh, 4);

			song.patterns.resize(song.header.patternCount);

			for (uint32_t patternIndex = 0; patternIndex < song.header.patternCount; ++patternIndex)
			{
				SharedPattern& pattern = song.patterns[patternIndex];

				uint16_t rowCount = readU16(fh);
				falign(fh, 4);

				pattern.rows.resize(rowCount);

				for (uint32_t rowIndex = 0; rowIndex < rowCount; ++rowIndex)
				{
					SharedRow& row = pattern.rows[rowIndex];
					row.cells.resize(song.header.channelCount);

					uint32_t const rowDataOffset = readU32(fh);

					// empty rows are encoded as a zero offset, rather than an explicit offset to an empty row
					if (rowDataOffset == 0)
						continue;

					size_t const currPos = ftell(fh);

					fseek(fh, baseAddr+rowDataOffset, SEEK_SET);

					std::vector<uint8_t> bitmask = readArray<uint8_t>(fh, ((song.header.channelCount*5)+7)/8);

					for (uint32_t channel = 0; channel < song.header.channelCount; ++channel)
					{
						SharedCell& cell = row.cells[channel];

						int const bitPosNote       = channel*5+0;
						int const bitPosInstrument = channel*5+1;
						int const bitPosVolume     = channel*5+2;
						int const bitPosEffect     = channel*5+3;
						int const bitPosParam      = channel*5+4;

						if (bitmask[bitPosNote/8] & (0x80>>(bitPosNote%8)))
						{
							cell.note = readU8(fh);
						}
						if (bitmask[bitPosInstrument/8] & (0x80>>(bitPosInstrument%8)))
						{
							cell.inst = readU8(fh);
						}
						if (bitmask[bitPosVolume/8] & (0x80>>(bitPosVolume%8)))
						{
							cell.vol = readU8(fh);
						}
						if (bitmask[bitPosEffect/8] & (0x80>>(bitPosEffect%8)))
						{
							cell.effect = readU8(fh);
						}
						if (bitmask[bitPosParam/8] & (0x80>>(bitPosParam%8)))
						{
							cell.param = readU8(fh);
						}
					}

					fseek(fh, currPos, SEEK_SET);
				}
			}
		}
	}
}
