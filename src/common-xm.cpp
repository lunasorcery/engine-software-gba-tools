#include <cstdlib>
#include "common-xm.h"
#include "misc.h"
#include <fmt/core.h>

XMFile::XMFile()
{
}

XMFile::XMFile(FILE* fh)
{
	xm_header_t const xmHeader = read<xm_header_t>(fh);
	this->moduleName = std::string(xmHeader.moduleName, xmHeader.moduleName+sizeof(xmHeader.moduleName));
	this->trackerName = std::string(xmHeader.trackerName, xmHeader.trackerName+sizeof(xmHeader.trackerName));
	this->songRestartPos = xmHeader.songRestartPos;
	this->channelCount = xmHeader.channelCount;
	this->frequencyTableFlags = xmHeader.frequencyTableFlags;
	this->defaultTickrate = xmHeader.defaultTickrate;
	this->defaultTempo = xmHeader.defaultTempo;
	this->patternOrder = std::vector<uint8_t>(xmHeader.patternOrderTable, xmHeader.patternOrderTable+xmHeader.songLength);

	if (ftell(fh) != 60+xmHeader.headerSize)
	{
		fseek(fh, 60+xmHeader.headerSize, SEEK_SET);
	}

	this->patterns.resize(xmHeader.patternCount);

	for (uint32_t patternIndex = 0; patternIndex < xmHeader.patternCount; ++patternIndex)
	{
		SharedPattern& pattern = this->patterns[patternIndex];

		xm_pattern_header_t const patternHeader = read<xm_pattern_header_t>(fh);

		pattern.rows.resize(patternHeader.rowCount);

		for (int rowIndex = 0; rowIndex < patternHeader.rowCount; ++rowIndex)
		{
			SharedRow& row = pattern.rows[rowIndex];

			row.cells.resize(xmHeader.channelCount);

			if (patternHeader.packedDataSize != 0)
			{
				for (int channel = 0; channel < xmHeader.channelCount; ++channel)
				{
					SharedCell& cell = row.cells[channel];

					uint8_t const bits = readU8(fh);
					if (bits & 0x80)
					{
						if (bits & 0x01) { cell.note   = readU8(fh); }
						if (bits & 0x02) { cell.inst   = readU8(fh); }
						if (bits & 0x04) { cell.vol    = readU8(fh); }
						if (bits & 0x08) { cell.effect = readU8(fh); }
						if (bits & 0x10) { cell.param  = readU8(fh); }
					}
					else
					{
						cell.note = bits;
						cell.inst   = readU8(fh);
						cell.vol    = readU8(fh);
						cell.effect = readU8(fh);
						cell.param  = readU8(fh);
					}
				}
			}
		}
	}

	this->instruments.resize(xmHeader.instrumentCount);

	for (int instIndex = 0; instIndex < xmHeader.instrumentCount; ++instIndex)
	{
		XMInstrument& inst = this->instruments[instIndex];

		size_t const instHeaderPos = ftell(fh);
		xm_instrument_header_t const instHeader = read<xm_instrument_header_t>(fh);
		inst.name = std::string(instHeader.name, instHeader.name+sizeof(instHeader.name));
		inst.type = instHeader.type;

		if (instHeader.sampleCount > 0)
		{
			inst.extHeader = read<xm_instrument_extended_header_t>(fh);
		}

		// seek because the instrument header has bonus extra data apparently?
		fseek(fh, instHeaderPos+instHeader.headerSize, SEEK_SET);

		std::vector<xm_sample_header_t> sampleHeaders;
		sampleHeaders.resize(instHeader.sampleCount);
		for (int sampleIndex = 0; sampleIndex < instHeader.sampleCount; ++sampleIndex)
		{
			sampleHeaders[sampleIndex] = read<xm_sample_header_t>(fh);
		}

		inst.samples.resize(instHeader.sampleCount);
		for (int sampleIndex = 0; sampleIndex < instHeader.sampleCount; ++sampleIndex)
		{
			XMSample& sample = inst.samples[sampleIndex];
			xm_sample_header_t const& sampleHeader = sampleHeaders[sampleIndex];

			sample.loopStart  = sampleHeader.loopStart;
			sample.loopLength = sampleHeader.loopLength;
			sample.volume     = sampleHeader.volume;
			sample.finetine   = sampleHeader.finetine;
			sample.typeFlags  = sampleHeader.typeFlags;
			sample.panning    = sampleHeader.panning;
			sample.relativeNoteNumber = sampleHeader.relativeNoteNumber;
			sample.name = std::string(sampleHeader.name, sampleHeader.name+sizeof(sampleHeader.name));

			sample.data = readArray<int8_t>(fh, sampleHeader.sampleLength);
		}
	}
}

void XMFile::save(FILE* fh) const
{
	xm_header_t xmHeader;
	{
		memcpy(xmHeader.idText, "Extended Module: ", sizeof(xmHeader.idText));

		if (this->moduleName.length() > sizeof(xmHeader.moduleName))
		{
			fputs("Module name is too long!\n", stderr);
			exit(1);
		}
		memset(xmHeader.moduleName, 0, sizeof(xmHeader.moduleName));
		memcpy(xmHeader.moduleName, this->moduleName.c_str(), this->moduleName.length());

		xmHeader.always1a = 0x1a;

		if (this->trackerName.length() > sizeof(xmHeader.trackerName))
		{
			fputs("Tracker name is too long!\n", stderr);
			exit(1);
		}
		memset(xmHeader.trackerName, 0, sizeof(xmHeader.trackerName));
		memcpy(xmHeader.trackerName, this->trackerName.c_str(), this->trackerName.length());

		xmHeader.versionNumber = 0x0104;
		xmHeader.headerSize = 276;
		xmHeader.songLength = this->patternOrder.size();
		xmHeader.songRestartPos = this->songRestartPos;
		xmHeader.channelCount = this->channelCount;
		xmHeader.patternCount = this->patterns.size();
		xmHeader.instrumentCount = this->instruments.size();
		xmHeader.frequencyTableFlags = this->frequencyTableFlags;
		xmHeader.defaultTickrate = this->defaultTickrate;
		xmHeader.defaultTempo = this->defaultTempo;

		if (this->patternOrder.size() > sizeof(xmHeader.patternOrderTable))
		{
			fputs("Pattern order table is too long!\n", stderr);
			exit(1);
		}
		memcpy(xmHeader.patternOrderTable, this->patternOrder.data(), this->patternOrder.size());
	}
	write<xm_header_t>(fh, xmHeader);

	for (SharedPattern const& pattern : this->patterns)
	{
		std::vector<uint8_t> packedData;
		for (SharedRow const& row : pattern.rows)
		{
			for (SharedCell const& cell : row.cells)
			{
				uint8_t bits = 0;
				if (cell.note)   bits |= 0x01;
				if (cell.inst)   bits |= 0x02;
				if (cell.vol)    bits |= 0x04;
				if (cell.effect) bits |= 0x08;
				if (cell.param)  bits |= 0x10;

				if (bits == 0x1f)
				{
					packedData.push_back(cell.note);
					packedData.push_back(cell.inst);
					packedData.push_back(cell.vol);
					packedData.push_back(cell.effect);
					packedData.push_back(cell.param);
				}
				else
				{
					bits |= 0x80;
					packedData.push_back(bits);
					if (cell.note)   packedData.push_back(cell.note);
					if (cell.inst)   packedData.push_back(cell.inst);
					if (cell.vol)    packedData.push_back(cell.vol);
					if (cell.effect) packedData.push_back(cell.effect);
					if (cell.param)  packedData.push_back(cell.param);
				}
			}
		}

		xm_pattern_header_t patternHeader;
		patternHeader.headerSize = 0x9;
		patternHeader.packingType = 0;
		patternHeader.rowCount = pattern.rows.size();
		patternHeader.packedDataSize = packedData.size();

		write<xm_pattern_header_t>(fh, patternHeader);
		writeArray<uint8_t>(fh, packedData);
	}

	for (XMInstrument const& inst : this->instruments)
	{
		xm_instrument_header_t instHeader;
		{
			instHeader.headerSize = sizeof(xm_instrument_header_t);
			if (inst.samples.size() > 0)
			{
				instHeader.headerSize += sizeof(xm_instrument_extended_header_t);
			}

			if (inst.name.length() > sizeof(instHeader.name))
			{
				fmt::print(stderr, "Instrument name '{}' is too long!\n", inst.name);
				exit(1);
			}
			memset(instHeader.name, 0, sizeof(instHeader.name));
			memcpy(instHeader.name, inst.name.c_str(), inst.name.length());

			instHeader.type = inst.type;
			instHeader.sampleCount = inst.samples.size();
		}
		write<xm_instrument_header_t>(fh, instHeader);

		if (inst.samples.size() > 0)
		{
			xm_instrument_extended_header_t instHeaderExt;
			instHeaderExt = inst.extHeader;
			write<xm_instrument_extended_header_t>(fh, instHeaderExt);
		}

		for (XMSample const& sample : inst.samples)
		{
			xm_sample_header_t sampleHeader;
			{
				sampleHeader.sampleLength = sample.data.size();
				sampleHeader.loopStart    = sample.loopStart;
				sampleHeader.loopLength   = sample.loopLength;
				sampleHeader.volume       = sample.volume;
				sampleHeader.finetine     = sample.finetine;
				sampleHeader.typeFlags    = sample.typeFlags;
				sampleHeader.panning      = sample.panning;
				sampleHeader.relativeNoteNumber = sample.relativeNoteNumber;
				sampleHeader.reserved = 0;

				if (sample.name.length() > sizeof(sampleHeader.name))
				{
					fmt::print(stderr, "Sample name '{}' is too long!\n", sample.name);
					exit(1);
				}
				memset(sampleHeader.name, 0, sizeof(sampleHeader.name));
				memcpy(sampleHeader.name, sample.name.c_str(), sample.name.length());
			}
			write<xm_sample_header_t>(fh, sampleHeader);
		}

		for (XMSample const& sample : inst.samples)
		{
			writeArray(fh, sample.data);
		}
	}
}
