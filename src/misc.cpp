#include <cstring>
#include "misc.h"

void falign(FILE* fh, size_t align)
{
	size_t const pos = ftell(fh);
	size_t const offset = pos % align;
	if (offset != 0)
	{
		fseek(fh, align-offset, SEEK_CUR);
	}
}

bool tryParseHex(char const* str, int digits, int64_t* result)
{
	*result = 0;

	for (int i = 0; i < digits; ++i)
	{
		char const c = str[i];
		*result *= 16;
		if (c >= '0' && c <= '9')
		{
			*result += (c-'0');
		}
		else if (c >= 'A' && c <= 'F')
		{
			*result += 0xA+(c-'A');
		}
		else if (c >= 'a' && c <= 'f')
		{
			*result += 0xA+(c-'a');
		}
		else
		{
			return false;
		}
	}
	return true;
}

bool tryParseDecimal(char const* str, int digits, int64_t* result)
{
	*result = 0;

	for (int i = 0; i < digits; ++i)
	{
		char const c = str[i];
		*result *= 10;
		if (c >= '0' && c <= '9')
		{
			*result += (c-'0');
		}
		else
		{
			return false;
		}
	}
	return true;
}

bool tryParseNumber(char const* str, int64_t* result)
{
	*result = 0;

	if (strlen(str)>2 && str[0] == '0' && str[1] == 'x')
	{
		return tryParseHex(str+2, strlen(str)-2, result);
	}
	else
	{
		return tryParseDecimal(str, strlen(str), result);
	}
}
