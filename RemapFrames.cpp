#include "Common.h"

struct RemapData {
	VSNodeRef *node1;
	VSNodeRef *node2;
	VSVideoInfo vi;
	std::vector<unsigned int> frameMap;
};

static void VS_CC remapInit(VSMap *in, VSMap *out, void **instanceData, VSNode *node, VSCore *core, const VSAPI *vsapi) {
	RemapData *d{ static_cast<RemapData*>(*instanceData) };
	vsapi->setVideoInfo(&d->vi, 1, node);
}

static const VSFrameRef *VS_CC remapGetFrame(int n, int activationReason, void **instanceData, void **frameData, VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi) {
	RemapData *d{ static_cast<RemapData*>(*instanceData) };

	if (activationReason == arInitial) {
		//We check whether node2 is a null pointer. If it is, we use baseclip (node1). 
		//Else, sourceclip (node2) is used.
		vsapi->requestFrameFilter(d->frameMap[n], (d->node2 ? d->node2 : d->node1), frameCtx);
	}
	else if (activationReason == arAllFramesReady) {
		return vsapi->getFrameFilter(d->frameMap[n], (d->node2 ? d->node2 : d->node1), frameCtx);
	}

	return nullptr;
}

static void VS_CC remapFree(void *instanceData, VSCore *core, const VSAPI *vsapi) {
	RemapData *d{ static_cast<RemapData*>(instanceData) };
	vsapi->freeNode(d->node1);
	if (d->node2)
		vsapi->freeNode(d->node2);
	delete d;
}

//Map one frame to another. For the case: x y
static void matchIntToInt(const std::string &str, int &col, std::vector<unsigned int> &frameMap, const int &line, const bool &file, int maxFrames) {
	int initialFrame{ getInt(str, col, line, file, maxFrames, Filter::REMAP_FRAMES) }; //Original frame number
	skipWhitespace(str, col);
	int replaceFrame{ getInt(str, col, line, file, maxFrames, Filter::REMAP_FRAMES) }; //Frame to be remapped to
	frameMap[initialFrame] = replaceFrame;
}

//Maps a frame range to a single frame. For the case: [x y] z
static void matchRangeToInt(const std::string &str, int &col, std::vector<unsigned int> &frameMap, Range range, const int &line, const bool &file, int maxFrames) {
	int replaceFrame{ getInt(str, col, line, file, maxFrames, Filter::REMAP_FRAMES) };
	for (int i = range.start; i <= range.end; i++) {
		frameMap[i] = replaceFrame;
	}
}

//Maps one frame range to another. For the case: [x y] [z a]
//If the input and output ranges are not of equal size, the output
//range will be interpolated across the input range.
static void matchRangeToRange(const std::string &str, int &col, std::vector<unsigned int> &frameMap, Range rangeIn, const int &line, const bool &file, int maxFrames) {
	Range rangeOut;
	fillRange(str, col, rangeOut, line, file, maxFrames, Filter::REMAP_FRAMES);

	int rangeInSize = rangeIn.end - rangeIn.start + 1;
	double rangeOutSize = rangeOut.end - rangeOut.start;
	rangeOutSize += (rangeOutSize < 0) ? -1 : 1;

	rangeOutSize /= rangeInSize;
	for (int i = 0; i < rangeInSize; i++)
	{
		frameMap[rangeIn.start + i] = int(rangeOut.start + rangeOutSize * i);
	}
}

//The backbone of the filter. bool file indicates whether we are reading an input file or not.
//If we aren't reading an input file, then we are reading the mappings string.
//This checks for the following conditions:
//x y
//[x y] z
//[x y] [z a]
//
//If it isn't able to parse the string as any of the above three patterns, it throws a runtime error (Parse Error).
//Runtime errors (Index out of Bounds, Overflow, or Parse errors) can also be thrown from inside getInt and fillRange.
static void parse(std::string name, std::vector<unsigned int> &frameMap, void *stream, bool file, int maxFrames) {

	int line{ 0 }; //Current line. This is simply for throwing detailed errors and has no other use.
	int col{ 0 }; //Current column. This is used to track the current column position in the string we are at.

	if (!file) {
		skipWhitespace(name, col);
		//If mappings string is empty.
		if (col == name.size())
			return;
	}

	std::string temp;
	while (file ? std::getline(*static_cast<std::ifstream*>(stream), temp) : std::getline(*static_cast<std::stringstream*>(stream), temp)) {
		col = 0;
		skipWhitespace(temp, col);
		char ch{ getChar(temp, col) };
		if (ch != 0) {
			if (ch == '#')
				continue;
			else if (std::isdigit(ch) || ch == '-')
				matchIntToInt(temp, col, frameMap, line, file, maxFrames);
			else if (ch == '[') {
				++col;
				Range rangeIn;
				fillRange(temp, col, rangeIn, line, file, maxFrames, Filter::REMAP_FRAMES);
				if (rangeIn.start > rangeIn.end) {
					std::string location{ file ? " text file " : " mappings " };
					std::string error{ "RemapFrames: Index out of bounds in" + location + "at line " + std::to_string(line + 1) + ", column " + std::to_string(col + 1) };
					throw std::runtime_error(error);
				}
				skipWhitespace(temp, col);
				ch = getChar(temp, col);
				if (ch != 0) {
					if (std::isdigit(ch) || ch == '-')
						matchRangeToInt(temp, col, frameMap, rangeIn, line, file, maxFrames);
					else if (ch == '[')
						++col;
					matchRangeToRange(temp, col, frameMap, rangeIn, line, file, maxFrames);
				}
			}
			else {
				std::string location{ file ? " text file " : " mappings " };
				std::string error{ "RemapFrames: Parse Error in" + location + "at line " + std::to_string(line + 1) + ", column " + std::to_string(col + 1) };
				throw std::runtime_error(error);
			}
			skipWhitespace(temp, col);
			if (col != temp.size()) {
				std::string location{ file ? " text file " : " mappings " };
				std::string error{ "RemapFrames: Parse Error in" + location + "at line " + std::to_string(line + 1) + ", column " + std::to_string(col + 1) };
				throw std::runtime_error(error);
			}
		}
		line++;
	}
}

void VS_CC remapCreate(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi) {
	RemapData d;
	d.node1 = vsapi->propGetNode(in, "baseclip", 0, 0);
	d.vi = *vsapi->getVideoInfo(d.node1);
	int err;

	//We use a const char* to store the value from propGetData as std::string crashes or has undefined behaviour when fed NULL.
	std::string filename;
	const char* fn{ vsapi->propGetData(in, "filename", 0, &err) };
	if (err)
		filename = "";
	else
		filename = fn;
	
	std::string mappings;
	const char* mp{ vsapi->propGetData(in, "mappings", 0, &err) };
	if (err)
		mappings = "";
	else
		mappings = mp;

	//If sourceclip is not provided, we set node2 to a null pointer.
	d.node2 = vsapi->propGetNode(in, "sourceclip", 0, &err);
	if (err)
		d.node2 = nullptr;

	bool mismatch{ !!vsapi->propGetInt(in, "mismatch", 0, &err) };
	if (err)
		mismatch = false;

	//We do not accept variable clip lengths regardless of mismatch's value.
	MismatchCauses mismatchCause = findCommonVi(&d.vi, d.node2, vsapi);
	if (mismatchCause == MismatchCauses::DIFFERENT_LENGTHS) {
		vsapi->setError(out, "RemapFrames: Clip lengths don't match");
		vsapi->freeNode(d.node1);
		if (d.node2)
			vsapi->freeNode(d.node2); //We check whether node2 is a null pointer. If it isn't, we free the node.
		return;
	}

	if (static_cast<bool>(mismatchCause) && (!mismatch)) {
		if (mismatchCause == MismatchCauses::DIFFERENT_DIMENSIONS)
			vsapi->setError(out, "RemapFrames: Clip dimensions don't match");
		else if (mismatchCause == MismatchCauses::DIFFERENT_FORMATS)
			vsapi->setError(out, "RemapFrames: Clip formats don't match");
		else if (mismatchCause == MismatchCauses::DIFFERENT_FRAMERATES)
			vsapi->setError(out, "RemapFrames: Clip frame rates don't match");
		vsapi->freeNode(d.node1);
		if (d.node2)
			vsapi->freeNode(d.node2);
		return;
	}

	//We use a vector to store frame mappings. Each index represents a frame number,
	//and each value at that index represents which frame it going to be replaced with.
	d.frameMap.resize(d.vi.numFrames);
	
	//All frames map to themselves by default.
	for (int i = 0; i < d.frameMap.size(); i++) {
		d.frameMap[i] = i;
	}

	//Enclosed in a try catch block to catch any runtime errors.
	//Frame mappings in the mappings string have higher precedence than
	//the ones in the text file (they can override frame mappings in
	//the text file).
	try {
		if (!filename.empty()) {
			std::ifstream file(filename);
			if (!file) {
				vsapi->setError(out, "RemapFrames: Failed to open the timecodes file.");
				vsapi->freeNode(d.node1);
				if (d.node2)
					vsapi->freeNode(d.node2);
				return;
			}
			parse(filename, d.frameMap, &file, true, d.vi.numFrames);
		}
		if (!mappings.empty()) {
			std::stringstream stream(mappings);
			parse(mappings, d.frameMap, &stream, false, d.vi.numFrames);
		}
	}
	catch (const std::exception &ex) {
		vsapi->setError(out, ex.what());
		vsapi->freeNode(d.node1);
		if (d.node2)
			vsapi->freeNode(d.node2);
		return;
	}

	RemapData *data = new RemapData{ d };
	vsapi->createFilter(in, out, "Remap", remapInit, remapGetFrame, remapFree, fmParallel, 0, data, core);
}