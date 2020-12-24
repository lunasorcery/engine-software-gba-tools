#include <cstdio>
#include <fmt/core.h>
#include "common-xm.h"

int main(int argc, char** argv)
{
	if (argc <= 1)
	{
		fmt::print(stderr, "Expected one arg! usage:\n");
		fmt::print(stderr, "xmprint xmfile.xm\n");
		exit(1);
	}

	char const* xmPath = argv[1];

	FILE* fhIn = fopen(xmPath, "rb");
	if (!fhIn)
	{
		fmt::print(stderr, "Failed to open {} for reading\n", xmPath);
		exit(1);
	}

	XMFile xm(fhIn);

	fclose(fhIn);

	fmt::print("name: [{}]\n", xm.moduleName.c_str());
	fmt::print("tracker: [{}]\n", xm.trackerName.c_str());
	fmt::print("pattern order: [");
	for (uint8_t entry : xm.patternOrder)
	{
		fmt::print(" {:02x}", entry);
	}
	fmt::print(" ]\n");

	for (uint32_t patternIndex = 0; patternIndex < xm.patterns.size(); ++patternIndex)
	{
		fmt::print("-- pattern {:02x} --\n", patternIndex);

		SharedPattern const& pattern = xm.patterns[patternIndex];

		for (uint32_t rowIndex = 0; rowIndex < pattern.rows.size(); ++rowIndex)
		{
			fmt::print("\t{:02x} |", rowIndex);

			SharedRow const& row = pattern.rows[rowIndex];

			char const* notes[] = {
				"C-", "C#", "D-",
				"D#", "E-", "F-",
				"F#", "G-", "G#",
				"A-", "A#", "B-",
			};

			for (int i = 0; i < xm.channelCount; ++i)
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

	for (uint32_t instIndex = 0; instIndex < xm.instruments.size(); ++instIndex)
	{
		XMInstrument const& inst = xm.instruments[instIndex];

		fmt::print("-- instrument {} --\n", instIndex+1);
		fmt::print("\tname: [{}]\n", inst.name.c_str());
		fmt::print("\ttype: {}\n", inst.type);
		if (inst.samples.size() > 0)
		{
			fmt::print("\tsample number for all notes:");
			for (uint8_t entry : inst.extHeader.sampleNumberForAllNotes)
			{
				fmt::print(" {},", entry);
			}
			fmt::print("\n");
			if (inst.extHeader.volumePointCount > 0)
			{
				fmt::print("\tvolume envelope points:");
				for (int i = 0; i < inst.extHeader.volumePointCount; ++i)
				{
					fmt::print(" [{}, {}],",
						inst.extHeader.volumeEnvelopePoints[i].x,
						inst.extHeader.volumeEnvelopePoints[i].y);
				}
				fmt::print("\n");
			}
			if (inst.extHeader.panningPointCount > 0)
			{
				fmt::print("\tpanning envelope points:");
				for (int i = 0; i < inst.extHeader.panningPointCount; ++i)
				{
					fmt::print(" [{}, {}],",
						inst.extHeader.panningEnvelopePoints[i].x,
						inst.extHeader.panningEnvelopePoints[i].y);
				}
				fmt::print("\n");
			}
			fmt::print("\tvolume point count: {}\n", inst.extHeader.volumePointCount);
			fmt::print("\tpanning point count: {}\n", inst.extHeader.panningPointCount);
			fmt::print("\tvolume sustain point: {}\n", inst.extHeader.volumeSustainPoint);
			fmt::print("\tvolume loop start point: {}\n", inst.extHeader.volumeLoopStartPoint);
			fmt::print("\tvolume loop end point: {}\n", inst.extHeader.volumeLoopEndPoint);
			fmt::print("\tpanning sustain point: {}\n", inst.extHeader.panningSustainPoint);
			fmt::print("\tpanning loop start point: {}\n", inst.extHeader.panningLoopStartPoint);
			fmt::print("\tpanning loop end point: {}\n", inst.extHeader.panningLoopEndPoint);
			fmt::print("\tvolume type {:02x}\n", inst.extHeader.volumeType);
			fmt::print("\tpanning type {:02x}\n", inst.extHeader.panningType);
			fmt::print("\tvibrato type {}\n", inst.extHeader.vibratoType);
			fmt::print("\tvibrato sweep {}\n", inst.extHeader.vibratoSweep);
			fmt::print("\tvibrato depth {}\n", inst.extHeader.vibratoDepth);
			fmt::print("\tvibrato rate {}\n", inst.extHeader.vibratoRate);
			fmt::print("\tvolume fadeout {}\n", inst.extHeader.volumeFadeout);
		}
		for (uint32_t sampleIndex = 0; sampleIndex < inst.samples.size(); ++sampleIndex)
		{
			XMSample const& sample = inst.samples[sampleIndex];

			fmt::print("\t-- sample {} --\n", sampleIndex);
			fmt::print("\t\tloop start: {}\n", sample.loopStart);
			fmt::print("\t\tloop length: {}\n", sample.loopLength);
			fmt::print("\t\tvolume: {}\n", sample.volume);
			fmt::print("\t\tfinetine: {}\n", sample.finetine);
			fmt::print("\t\ttype flags: {:02x}\n", sample.typeFlags);
			fmt::print("\t\tpanning: {}\n", sample.panning);
			fmt::print("\t\trelative note number: {}\n", sample.relativeNoteNumber);
			fmt::print("\t\tname: [{}]\n", sample.name.c_str());
		}
	}

	return 0;
}