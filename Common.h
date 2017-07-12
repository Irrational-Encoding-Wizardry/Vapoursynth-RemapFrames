#ifndef COMMON_H
#define COMMON_H
#include "VapourSynth.h"
#include "VSHelper.h"
#include <vector>
#include <sstream>
#include <cctype>
#include <fstream>

struct Range {
	int start;
	int end;
};

enum class MismatchCauses {
	NO_MISMATCH,
	DIFFERENT_DIMENSIONS,
	DIFFERENT_FORMATS,
	DIFFERENT_FRAMERATES,
	DIFFERENT_LENGTHS
};

enum class Filter {
	REMAP_FRAMES,
	REMAP_FRAMES_SIMPLE,
	REPLACE_FRAMES_SIMPLE
};

void skipWhitespace(const std::string &str, int &col);
int getInt(const std::string &str, int &col, const int &line, const bool &file, int maxFrames, Filter filter);
char getChar(const std::string &str, const int &col);
void fillRange(const std::string &str, int &col, Range &range, const int &line, const bool &file, int maxFrames, Filter filter);
MismatchCauses findCommonVi(VSVideoInfo *outVi, VSNodeRef *node2, const VSAPI *vsapi);

#endif