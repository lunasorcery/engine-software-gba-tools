#pragma once

#include <cstdio>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include "common.h"

struct xm_envelope_point_t {
	uint16_t x, y;
};
static_assert(sizeof(xm_envelope_point_t) == 4);


struct xm_header_t {
	char idText[17];
	char moduleName[20];
	uint8_t always1a;
	char trackerName[20];
	uint16_t versionNumber;
	uint32_t headerSize;
	uint16_t songLength;
	uint16_t songRestartPos;
	uint16_t channelCount;
	uint16_t patternCount;
	uint16_t instrumentCount;
	uint16_t frequencyTableFlags;
	uint16_t defaultTickrate;
	uint16_t defaultTempo;
	uint8_t patternOrderTable[256];
};
static_assert(sizeof(xm_header_t) == 336);


#pragma pack(1)
struct xm_pattern_header_t {
	uint32_t headerSize;
	uint8_t packingType;
	uint16_t rowCount;
	uint16_t packedDataSize;
};
static_assert(sizeof(xm_pattern_header_t) == 9);
#pragma pack()


#pragma pack(1)
struct xm_instrument_header_t {
	uint32_t headerSize;
	char name[22];
	uint8_t type;
	uint16_t sampleCount;
};
static_assert(sizeof(xm_instrument_header_t) == 29);
#pragma pack()


#pragma pack(1)
struct xm_instrument_extended_header_t {
	uint32_t sampleHeaderSize;
	uint8_t sampleNumberForAllNotes[96];
	xm_envelope_point_t volumeEnvelopePoints[12];
	xm_envelope_point_t panningEnvelopePoints[12];
	uint8_t volumePointCount;
	uint8_t panningPointCount;
	uint8_t volumeSustainPoint;
	uint8_t volumeLoopStartPoint;
	uint8_t volumeLoopEndPoint;
	uint8_t panningSustainPoint;
	uint8_t panningLoopStartPoint;
	uint8_t panningLoopEndPoint;
	uint8_t volumeType;
	uint8_t panningType;
	uint8_t vibratoType;
	uint8_t vibratoSweep;
	uint8_t vibratoDepth;
	uint8_t vibratoRate;
	uint16_t volumeFadeout;
	uint16_t reserved;
};
static_assert(sizeof(xm_instrument_extended_header_t) == 214);
#pragma pack()


#pragma pack(1)
struct xm_sample_header_t {
	uint32_t sampleLength;
	uint32_t loopStart;
	uint32_t loopLength;
	uint8_t volume;
	int8_t finetine;
	uint8_t typeFlags;
	uint8_t panning;
	int8_t relativeNoteNumber;
	uint8_t reserved;
	char name[22];
};
static_assert(sizeof(xm_sample_header_t) == 40);
#pragma pack()


struct XMSample {
	uint32_t loopStart;
	uint32_t loopLength;
	uint8_t volume;
	int8_t finetine;
	uint8_t typeFlags;
	uint8_t panning;
	int8_t relativeNoteNumber;
	std::string name;
	std::vector<int8_t> data;
};


struct XMInstrument {
	std::string name;
	uint8_t type;
	xm_instrument_extended_header_t extHeader;
	std::vector<XMSample> samples;
};


struct XMFile {
	std::string moduleName;
	std::string trackerName;
	uint16_t songRestartPos;
	uint16_t channelCount;
	uint16_t frequencyTableFlags;
	uint16_t defaultTickrate;
	uint16_t defaultTempo;
	std::vector<uint8_t> patternOrder;
	std::vector<SharedPattern> patterns;
	std::vector<XMInstrument> instruments;

	XMFile();
	XMFile(FILE* fh);
	void save(FILE* fh) const;
};
