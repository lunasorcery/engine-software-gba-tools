#include <cstdio>
#include <fmt/core.h>
#include "common-gba.h"
#include "misc.h"

int main(int argc, char** argv)
{
	if (argc <= 2)
	{
		fmt::print(stderr, "Expected two args! usage:\n");
		fmt::print(stderr, "gbaprint romfile.gba <bank offset>\n");
		exit(1);
	}

	char const* romPath = argv[1];
	char const* bankAddressStr = argv[2];

	FILE* fhIn = fopen(romPath, "rb");
	if (!fhIn)
	{
		fmt::print(stderr, "Failed to open {} for reading\n", romPath);
		exit(1);
	}

	int64_t bankAddress = 0;
	if (!tryParseNumber(bankAddressStr, &bankAddress))
	{
		fmt::print(stderr, "Failed to parse '{}' as a number\n", bankAddressStr);
		exit(1);
	}
	// in case the user used an 08xxxxxx address, mask off the top bits
	bankAddress &= 0x00ffffff;

	GBAMusicBank gbaMusicBank(fhIn, bankAddress);

	for (uint32_t instrIndex = 0; instrIndex < gbaMusicBank.instruments.size(); ++instrIndex)
	{
		GBAInstrument const& instrument = gbaMusicBank.instruments[instrIndex];

		fmt::print("------ instrument {:02x} ------\n", instrIndex+1);
		fmt::print("\tsample length: {} bytes\n", instrument.header.sampleLength);
		fmt::print("\tsample loop start:  {}\n", instrument.header.sampleLoopStart);
		fmt::print("\tsample loop length: {}\n", instrument.header.sampleLoopLength);
		fmt::print("\tsample volume:      {}\n", instrument.header.sampleVolume);
		fmt::print("\tsample panning:     {}\n", instrument.header.samplePanning);
		fmt::print("\tsample finetune:    {}\n", instrument.header.sampleFinetune);
		fmt::print("\tsample relative note #: {}\n", instrument.header.sampleRelativeNoteNumber);
		fmt::print("\tvolume fadeout:     {}\n", instrument.header.volumeFadeout);
		fmt::print("\tunknown bytes:      {:02x} {:02x}\n", instrument.header.unknownBytes[0], instrument.header.unknownBytes[1]);
		{
			fmt::print("\t-- volume envelope --\n");
			fmt::print("\t\tpoint count:      {}\n", instrument.header.volumeEnvelope.pointCount);
			fmt::print("\t\tsustain point?    {}\n", instrument.header.volumeEnvelope.maybeSustainPoint);
			fmt::print("\t\tloop start point? {}\n", instrument.header.volumeEnvelope.maybeLoopStartPoint);
			fmt::print("\t\tloop end point?   {}\n", instrument.header.volumeEnvelope.maybeLoopEndPoint);
			if (instrument.header.volumeEnvelope.pointCount != 0)
			{
				fmt::print("\t\tpoints:");
				for (int j = 0; j < instrument.header.volumeEnvelope.pointCount; ++j)
				{
					fmt::print(" [{}, {}],",
						instrument.header.volumeEnvelope.points[j].x,
						instrument.header.volumeEnvelope.points[j].y);
				}
				fmt::print("\n");
			}
		}
		{
			fmt::print("\t-- panning envelope --\n");
			fmt::print("\t\tpoint count:      {}\n", instrument.header.panningEnvelope.pointCount);
			fmt::print("\t\tsustain point?    {}\n", instrument.header.panningEnvelope.maybeSustainPoint);
			fmt::print("\t\tloop start point? {}\n", instrument.header.panningEnvelope.maybeLoopStartPoint);
			fmt::print("\t\tloop end point?   {}\n", instrument.header.panningEnvelope.maybeLoopEndPoint);
			if (instrument.header.panningEnvelope.pointCount != 0)
			{
				fmt::print("\t\tpoints:");
				for (int j = 0; j < instrument.header.panningEnvelope.pointCount; ++j)
				{
					fmt::print(" [{}, {}],",
						instrument.header.panningEnvelope.points[j].x,
						instrument.header.panningEnvelope.points[j].y);
				}
				fmt::print("\n");
			}
		}
		fmt::print("\n");
	}

	for (uint32_t songIndex = 0; songIndex < gbaMusicBank.songs.size(); ++songIndex)
	{
		fmt::print("------ song {:02x} ------\n", songIndex);

		GBASong const& song = gbaMusicBank.songs[songIndex];

		fmt::print("\tchannel count: {}\n", song.header.channelCount);
		fmt::print("\tsong length:   {}\n", song.header.songLength);
		fmt::print("\tloop point:    {}\n", song.header.loopPoint);
		fmt::print("\tpattern count: {}\n", song.header.patternCount);
		fmt::print("\ttickrate:      {} ticks/row\n", song.header.tickrate);
		fmt::print("\ttempo:         {} beats/min\n", song.header.tempo);

		fmt::print("\tpattern order: [");
		for (uint8_t patternInstance : song.patternOrder)
		{
			fmt::print(" {:02x}", patternInstance);
		}
		fmt::print(" ]\n");

		for (int patternIndex = 0; patternIndex < song.header.patternCount; ++patternIndex)
		{
			fmt::print("\t-- pattern {:02x} --\n", patternIndex);

			SharedPattern const& pattern = song.patterns[patternIndex];

			for (uint32_t rowIndex = 0; rowIndex < pattern.rows.size(); ++rowIndex)
			{
				fmt::print("\t\t{:02x} |", rowIndex);

				SharedRow const& row = pattern.rows[rowIndex];

				char const* notes[] = {
					"C-", "C#", "D-",
					"D#", "E-", "F-",
					"F#", "G-", "G#",
					"A-", "A#", "B-",
				};

				for (int i = 0; i < song.header.channelCount; ++i)
				{
					SharedCell const& cell = row.cells[i];
					uint8_t const note = (cell.note-1);
					fmt::print(cell.note ? " {}{}" : " ---", notes[note%12], note/12);
					fmt::print(cell.inst   ? " {:02x}" : " --", cell.inst);
					fmt::print(cell.vol    ? " {:02x}" : " --", cell.vol);
					fmt::print(cell.effect ? " {:02x}" : " --", cell.effect);
					fmt::print(cell.param  ? " {:02x}" : " --", cell.param);
					fmt::print(" |");
				}

				fmt::print("\n");
			}
		}
		fmt::print("\n");
	}

	return 0;
}
