# engine-software-gba-tools

[![CircleCI](https://circleci.com/gh/lunasorcery/engine-software-gba-tools.svg?style=svg)](https://circleci.com/gh/lunasorcery/engine-software-gba-tools)

A suite of tools for interacting with the XM-like music data found in certain Game Boy Advance games - specifically those that use the Engine Software replayer middleware.

Exporting music from games using these tools allows for much clearer audio than the original hardware was capable of producing. However, **I do not want to see music exported with these tools described as 'accurate' or 'faithful'**, at least until such time as certain assumptions in the code have been verified.

_Disclaimer: This project is not affiliated with Engine Software and is purely a reverse-engineering effort._

## Overview of tools

### xmprint

A basic debugging tool for viewing the contents of an XM file and verifying my understanding of the format.

Usage:
```
xmprint path/to/xm/file.xm
```

### gbaprint

Prints the contents of a GBA music bank as text.

Usage:
```
gbaprint path/to/gba/rom.gba 0x123456
```

(where `0x123456` is the address of the music bank inside the rom)

### gbafind

Searches for a GBA music bank inside a GBA rom.

Usage:
```
gbafind path/to/gba/rom.gba
```

### gba2xm

Exports a GBA music bank as a series of XM files.

Usage:
```
gba2xm path/to/gba/rom.gba 0x123456
```

(where `0x123456` is the address of the music bank inside the rom)

## Building

These tools are built using the [Meson](https://mesonbuild.com/) build system, which itself depends on [Ninja](https://ninja-build.org/).  
Once you have both installed, the recommended commands are as follows:

```
meson setup build
cd build
meson compile
```

## Music Bank structure

This is based primarily on static reverse-engineering of the binary storage format, along with some very light disassembly of the replayer code. I'm confident in the majority of it, but I am uncertain about some fields in the instrument structures.

_Note: all offsets are relative to the start of the music bank._

```
pseudostruct envelope_point
{
	u16 x; // point position in time
	u16 y; // point value
}

pseudostruct envelope
{
	u8 pointCount;
	u8 maybeSustainPoint;
	u8 maybeLoopStartPoint;
	u8 maybeLoopEndPoint;

	envelope_point points[12];
}

pseudostruct instrument
{
	u32 sampleLength;
	u32 sampleLoopStart;
	u32 sampleLoopLength;
	u8 sampleVolume;
	u8 samplePanning;
	s8 sampleFinetune;
	s8 sampleRelativeNoteNumber;
	u16 volumeFadeout;
	u8 unknownBytes[2]; // probably just alignment padding
	envelope volumeEnvelope;
	envelope panningEnvelope;

	// signed 8-bit PCM, *not* delta-encoded
	s8 sampleData[sampleLength];

	// - align to 4-byte boundary -
}

pseudostruct row
{
	u8 bitmask[ceil((channelCount*5)/8)];
	u8 data[countSetBits(bitmask)];
}

pseudostruct pattern
{
	u16 rowCount;

	// - align to 4-byte boundary -

	u32 rowOffsets[rowCount];
}

pseudostruct song
{
	u8 channelCount;
	u8 songLength;
	u8 loopPoint;
	u8 patternCount;
	u8 tickrate;
	u8 tempo;

	// - align to 4-byte boundary -

	u8 patternOrder[songLength];

	// - align to 4-byte boundary -

	pattern patterns[patternCount];
}

pseudostruct musicbank
{
	// seems to always be 0x0121
	// possibly a version number of 1.21?
	u16 id;
	
	u8 instrumentCount;
	u8 songCount;

	u32 songOffsets[songCount];

	instrument instruments[instrumentCount];

	// after this point, parse data by seeking to offsets,
	// as the row data's length is unpredictable.

	u8 rowdata[???];

	song songs[songCount];
}
```

### Row data compression

Row data follows the same 5-byte `{note,inst,vol,effect,param}` layout as XM, but uses a different packing structure in the ROM.

Each bit of `bitmask` corresponds to a column of the tracker.
The first five bits correspond to the fields of the first channel,
the next five bits correspond to the next channel, and so on.
As such, the length of `bitmask` is `channelCount * 5` bits, rounded up to the next whole byte.

Starting at `bitmask[0]` and reading from MSB to LSB, if the bit is set, read a byte from `data` and assign it to the corresponding field of the channel.

For example, here's a row from a 3-channel song.

```
bitmask: [0xc0, 0x30]
data: [0x2a, 0x04, 0x31, 0x0d]
```

Reading the bitmask a bit at a time, we get the following binary. The lone last bit is just padding and goes unused.

```
11000 00000 11000 0
```

This means `data` has note/inst data for the first and last channel.
In a pseudo-tracker interface, that row might look like this:
```
| 2a 04 -- -- -- | -- -- -- -- -- | 31 0d -- -- -- |
```
