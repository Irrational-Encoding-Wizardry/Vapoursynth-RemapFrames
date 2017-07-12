#include "Common.h"

struct ReplaceData {
	VSNodeRef *node1;
	VSNodeRef *node2;
	VSVideoInfo vi;
	std::vector<unsigned int> frameMap;
};

static void VS_CC replaceInit(VSMap *in, VSMap *out, void **instanceData, VSNode *node, VSCore *core, const VSAPI *vsapi) {
	ReplaceData *d{ static_cast<ReplaceData*>(*instanceData) };
	vsapi->setVideoInfo(&d->vi, 1, node);
}

static const VSFrameRef *VS_CC replaceGetFrame(int n, int activationReason, void **instanceData, void **frameData, VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi) {
	ReplaceData *d{ static_cast<ReplaceData*>(*instanceData) };

	if (activationReason == arInitial) {
		//Check whether frameMap returns 0 or 1 for the current frame and return the corresponding clip.
		vsapi->requestFrameFilter(n, (d->frameMap[n] ? d->node2 : d->node1), frameCtx);
	}
	else if (activationReason == arAllFramesReady) {
		return vsapi->getFrameFilter(n, (d->frameMap[n] ? d->node2 : d->node1), frameCtx);
	}

	return nullptr;
}

static void VS_CC replaceFree(void *instanceData, VSCore *core, const VSAPI *vsapi) {
	ReplaceData *d{ static_cast<ReplaceData*>(instanceData) };
	vsapi->freeNode(d->node1);
	vsapi->freeNode(d->node2);
	delete d;
}

//Loops through the string/file checking for ranges or integers and sets frameMap values accordingly.
static void parse(std::string name, std::vector<unsigned int> &frameMap, void *stream, bool file, int maxFrames) {
	int line{ 0 };
	int col{ 0 };

	if (!file) {
		skipWhitespace(name, col);
		if (col == name.size())
			return;
	}

	std::string temp;
	while (file ? std::getline(*static_cast<std::ifstream*>(stream), temp) : std::getline(*static_cast<std::stringstream*>(stream), temp)) {
		col = 0;
		while (col < temp.size()) {
			skipWhitespace(temp, col);
			char ch{ getChar(temp, col) };
			if (ch != 0) {
				if (ch == '#')
					continue;
				else if (std::isdigit(ch) || ch == '-')
					frameMap[getInt(temp, col, line, file, maxFrames, Filter::REPLACE_FRAMES_SIMPLE)] = 1;
				else if (ch == '[') {
					++col;
					Range range;
					fillRange(temp, col, range, line, file, maxFrames, Filter::REPLACE_FRAMES_SIMPLE);
					for (int i = range.start; i <= range.end; i++) {
						frameMap[i] = 1;
					}
				}
				else {
					std::string location{ file ? " text file " : " mappings " };
					std::string error{ "ReplaceFramesSimple: Parse Error in" + location + "at line " + std::to_string(line + 1) + ", column " + std::to_string(col + 1) };
					throw std::runtime_error(error);
				}
			}
		}
		line++;
	}
}

void VS_CC replaceCreate(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi) {
	ReplaceData d;
	d.node1 = vsapi->propGetNode(in, "baseclip", 0, 0);
	d.node2 = vsapi->propGetNode(in, "sourceclip", 0, 0);
	d.vi = *vsapi->getVideoInfo(d.node1);
	int err;

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

	bool mismatch{ !!vsapi->propGetInt(in, "mismatch", 0, &err) };
	if (err)
		mismatch = false;

	MismatchCauses mismatchCause = findCommonVi(&d.vi, d.node2, vsapi);
	if (mismatchCause == MismatchCauses::DIFFERENT_LENGTHS) {
		vsapi->setError(out, "RemapFrames: Clip lengths don't match");
		vsapi->freeNode(d.node1);
		vsapi->freeNode(d.node2);
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
		vsapi->freeNode(d.node2);
		return;
	}

	d.frameMap.resize(d.vi.numFrames);

	//All frames map to baseclip by default
	//0 = baseclip
	//1 = sourceclip
	for (int i = 0; i < d.frameMap.size(); i++) {
		d.frameMap[i] = 0;
	}
	try {
		if (!filename.empty()) {
			std::ifstream file(filename);
			if (!file) {
				vsapi->setError(out, "RemapFrames: Failed to open the timecodes file.");
				vsapi->freeNode(d.node1);
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
		vsapi->freeNode(d.node2);
		return;
	}

	ReplaceData *data = new ReplaceData{ d };
	vsapi->createFilter(in, out, "Replace", replaceInit, replaceGetFrame, replaceFree, fmParallel, 0, data, core);
}