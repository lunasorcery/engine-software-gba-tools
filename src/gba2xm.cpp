#include <cstdio>
#include <fmt/core.h>
#include "common-xm.h"
#include "common-gba.h"
#include "misc.h"
#include "version.h"

XMFile convert(GBAMusicBank const& bank, GBASong const& song)
{
	XMFile xm;

	xm.moduleName = "";
	xm.trackerName = "";
	xm.songRestartPos = song.header.loopPoint;
	xm.channelCount = song.header.channelCount;
	xm.frequencyTableFlags = 0; // TODO: figure out whether this matters
	xm.defaultTickrate = song.header.tickrate;
	xm.defaultTempo = song.header.tempo;
	xm.patternOrder = song.patternOrder;
	xm.patterns = song.patterns;

	for (uint32_t instIndex = 0; instIndex < bank.instruments.size(); ++instIndex)
	{
		GBAInstrument const& gbaInst = bank.instruments[instIndex];

		XMInstrument xmInst;
		{
			xmInst.name = fmt::format("Instrument {:02x}", instIndex+1);
			xmInst.type = 0;
			if (gbaInst.sample.size() > 0)
			{
				xmInst.extHeader.sampleHeaderSize = sizeof(xm_sample_header_t);
				memset(xmInst.extHeader.sampleNumberForAllNotes, 0, sizeof(xmInst.extHeader.sampleNumberForAllNotes));
				memcpy(xmInst.extHeader.volumeEnvelopePoints,  gbaInst.header.volumeEnvelope.points,  sizeof(xmInst.extHeader.volumeEnvelopePoints));
				memcpy(xmInst.extHeader.panningEnvelopePoints, gbaInst.header.panningEnvelope.points, sizeof(xmInst.extHeader.panningEnvelopePoints));
				xmInst.extHeader.volumePointCount  = gbaInst.header.volumeEnvelope.pointCount;
				xmInst.extHeader.panningPointCount = gbaInst.header.panningEnvelope.pointCount;
				// not certain about this:
				//{
					xmInst.extHeader.volumeSustainPoint    = gbaInst.header.volumeEnvelope.maybeSustainPoint;
					xmInst.extHeader.volumeLoopStartPoint  = gbaInst.header.volumeEnvelope.maybeLoopStartPoint;
					xmInst.extHeader.volumeLoopEndPoint    = gbaInst.header.volumeEnvelope.maybeLoopEndPoint;

					xmInst.extHeader.panningSustainPoint   = gbaInst.header.panningEnvelope.maybeSustainPoint;
					xmInst.extHeader.panningLoopStartPoint = gbaInst.header.panningEnvelope.maybeLoopStartPoint;
					xmInst.extHeader.panningLoopEndPoint   = gbaInst.header.panningEnvelope.maybeLoopEndPoint;

					xmInst.extHeader.volumeType = 0;
					if (xmInst.extHeader.volumePointCount    != 0)    { xmInst.extHeader.volumeType  |= 0x01; /* on */ }
					if (xmInst.extHeader.volumeSustainPoint  != 0xff) { xmInst.extHeader.volumeType  |= 0x02; /* sustain */ }
					if (xmInst.extHeader.volumeLoopEndPoint  != 0xff) { xmInst.extHeader.volumeType  |= 0x04; /* loop */ }

					xmInst.extHeader.panningType = 0;
					if (xmInst.extHeader.panningPointCount   != 0)    { xmInst.extHeader.panningType |= 0x01; /* on */ }
					if (xmInst.extHeader.panningSustainPoint != 0xff) { xmInst.extHeader.panningType |= 0x02; /* sustain */ }
					if (xmInst.extHeader.panningLoopEndPoint != 0xff) { xmInst.extHeader.panningType |= 0x04; /* loop */ }
				//}
				xmInst.extHeader.vibratoType   = 0; // TODO: figure out what any of these do
				xmInst.extHeader.vibratoSweep  = 0; // TODO: figure out what any of these do
				xmInst.extHeader.vibratoDepth  = 0; // TODO: figure out what any of these do
				xmInst.extHeader.vibratoRate   = 0; // TODO: figure out what any of these do
				xmInst.extHeader.volumeFadeout = gbaInst.header.volumeFadeout;
				xmInst.extHeader.reserved = 0;

				XMSample xmSample;
				{
					xmSample.loopStart  = gbaInst.header.sampleLoopStart;
					xmSample.loopLength = gbaInst.header.sampleLoopLength;
					xmSample.volume     = gbaInst.header.sampleVolume;
					xmSample.finetine   = gbaInst.header.sampleFinetune;

					xmSample.typeFlags = 0;
					if (gbaInst.header.sampleLoopLength > 0)
					{
						xmSample.typeFlags |= 0x01; // Forward loop
					}

					xmSample.panning = gbaInst.header.samplePanning;
					xmSample.relativeNoteNumber = gbaInst.header.sampleRelativeNoteNumber;
					xmSample.name = fmt::format("Sample {:02x}", instIndex+1);
					xmSample.data = gbaInst.sample;

					// delta-encoding data
					for (int i = xmSample.data.size() - 1; i > 0; --i)
					{
						xmSample.data[i] -= xmSample.data[i-1];
					}
				}
				xmInst.samples.push_back(xmSample);
			}
		}
		xm.instruments.push_back(xmInst);
	}

	return xm;
}

int main(int argc, char** argv)
{
	if (argc <= 2)
	{
		fmt::print(stderr, "Expected two args! usage:\n");
		fmt::print(stderr, "gba2xm romfile.gba <bank offset>\n");
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
	fmt::print(
		"Loaded a music bank with {} songs and {} shared instruments\n",
		gbaMusicBank.songs.size(),
		gbaMusicBank.instruments.size());

	// get the fourcc from the cart header
	char fourcc[5] = { 0 };
	fseek(fhIn, 0xAC, SEEK_SET);
	fread(fourcc, 1, 4, fhIn);

	fclose(fhIn);

	std::string const trackerName = fmt::format("esgba2xm-{}.{}.{}", kToolVersionMajor, kToolVersionMinor, kToolVersionPatch);

	for (uint32_t songIndex = 0; songIndex < gbaMusicBank.songs.size(); ++songIndex)
	{
		std::string const songName = fmt::format("{}-{:06X}-song{:02X}", fourcc, bankAddress, songIndex);
		std::string const outfilePath = songName + ".xm";

		GBASong const& song = gbaMusicBank.songs[songIndex];
		XMFile xm = convert(gbaMusicBank, song);

		xm.moduleName = songName;
		xm.trackerName = trackerName;

		// strip unused samples
		bool usedInstruments[256] = {0};
		for (auto const& pattern : xm.patterns)
			for (auto const& row : pattern.rows)
				for (auto const& cell : row.cells)
					if (cell.inst)
						usedInstruments[cell.inst-1] = true;
		for (uint32_t i = 0; i < xm.instruments.size(); ++i)
			if (!usedInstruments[i])
				xm.instruments[i].samples.clear();

		fmt::print("Saving song {:02x} to {}\n", songIndex, outfilePath);

		FILE* fhOut = fopen(outfilePath.c_str(), "wb");
		if (!fhOut)
		{
			fmt::print(stderr, "failed to open {} for writing\n", outfilePath);
			exit(1);
		}
		xm.save(fhOut);
		fclose(fhOut);
	}
}
