#pragma once

#include <cstdio>
#include <cstdint>
#include <tuple>
#include <vector>

void falign(FILE* fh, size_t align);

template<typename T> T read(FILE* fh)
{
	T value;
	fread(&value, sizeof(T), 1, fh);
	return value;
}

template<typename T> void write(FILE* fh, T const& value)
{
	fwrite(&value, sizeof(T), 1, fh);
}

template<typename T> std::vector<T> readArray(FILE* fh, size_t count)
{
	std::vector<T> vec;
	for (size_t i = 0; i < count; ++i)
	{
		vec.push_back(read<T>(fh));
	}
	return vec;
}

template<typename T> void writeArray(FILE* fh, std::vector<T> const& vec)
{
	for (auto const& item : vec)
	{
		write<T>(fh, item);
	}
}

inline uint8_t readU8(FILE* fh) { return read<uint8_t>(fh); }
inline uint16_t readU16(FILE* fh) { return read<uint16_t>(fh); }
inline uint32_t readU32(FILE* fh) { return read<uint32_t>(fh); }

bool tryParseHex(char const* str, int digits, int64_t* result);
bool tryParseDecimal(char const* str, int digits, int64_t* result);
bool tryParseNumber(char const* str, int64_t* result);
