#pragma once

#include <cstdint>
#include <vector>


struct SharedCell {
	uint8_t note   = 0;
	uint8_t inst   = 0;
	uint8_t vol    = 0;
	uint8_t effect = 0;
	uint8_t param  = 0;
};


struct SharedRow {
	std::vector<SharedCell> cells;
};


struct SharedPattern {
	std::vector<SharedRow> rows;
};
