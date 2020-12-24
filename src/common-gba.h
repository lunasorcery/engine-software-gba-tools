#pragma once

#include <cstdio>
#include <cstdint>
#include <optional>
#include <vector>
#include "common.h"

struct gba_musicbank_header_t {
	uint16_t version;
	uint8_t instrumentCount;
	uint8_t songCount;
};
static_assert(sizeof(gba_musicbank_header_t) == 4);


struct gba_envelope_point_t {
	uint16_t x, y;
};
static_assert(sizeof(gba_envelope_point_t) == 4);


struct gba_envelope_t {
	uint8_t pointCount;
	uint8_t maybeSustainPoint;
	uint8_t maybeLoopStartPoint;
	uint8_t maybeLoopEndPoint;

	gba_envelope_point_t points[12];
};
static_assert(sizeof(gba_envelope_t) == 52);


struct gba_instrument_header_t {
	uint32_t sampleLength;
	uint32_t sampleLoopStart;
	uint32_t sampleLoopLength;
	uint8_t sampleVolume;
	uint8_t samplePanning;
	int8_t sampleFinetune;
	int8_t sampleRelativeNoteNumber;
	uint16_t volumeFadeout;
	uint8_t unknownBytes[2]; // probably just alignment padding
	gba_envelope_t volumeEnvelope;
	gba_envelope_t panningEnvelope;
};
static_assert(sizeof(gba_instrument_header_t) == 124);


struct gba_song_header_t {
	uint8_t channelCount;
	uint8_t songLength;
	uint8_t loopPoint;
	uint8_t patternCount;
	uint8_t tickrate;
	uint8_t tempo;
};
static_assert(sizeof(gba_song_header_t) == 6);


struct GBAInstrument {
	gba_instrument_header_t header;
	std::vector<int8_t> sample;
};


struct GBASong {
	gba_song_header_t header;
	std::vector<uint8_t> patternOrder;
	std::vector<SharedPattern> patterns;
};


struct GBAMusicBank {
	std::vector<GBAInstrument> instruments;
	std::vector<GBASong> songs;

	GBAMusicBank(FILE* fh, size_t baseAddr);
};
