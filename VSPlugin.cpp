#include "Common.h"

//FilterCreate function declarations
void VS_CC remapCreate(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi);
void VS_CC remapSimpleCreate(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi);
void VS_CC replaceCreate(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi);

VS_EXTERNAL_API(void) VapourSynthPluginInit(VSConfigPlugin configFunc, VSRegisterFunction registerFunc, VSPlugin *plugin) {
	configFunc("blaze.plugin.remap", "remap", "Remaps frame indices based on a file/string", VAPOURSYNTH_API_VERSION, 1, plugin);
	registerFunc("RemapFrames", "baseclip:clip;filename:data:opt;mappings:data:opt;sourceclip:clip:opt;mismatch:int:opt;", remapCreate, nullptr, plugin);
	registerFunc("Remf", "baseclip:clip;filename:data:opt;mappings:data:opt;sourceclip:clip:opt;mismatch:int:opt;", remapCreate, nullptr, plugin);
	registerFunc("RemapFramesSimple", "clip:clip;filename:data:opt;mappings:data:opt;", remapSimpleCreate, nullptr, plugin);
	registerFunc("Remfs", "clip:clip;filename:data:opt;mappings:data:opt;", remapSimpleCreate, nullptr, plugin);
	registerFunc("ReplaceFramesSimple", "baseclip:clip;sourceclip:clip;filename:data:opt;mappings:data:opt;mismatch:int:opt;", replaceCreate, nullptr, plugin);
	registerFunc("Rfs", "baseclip:clip;sourceclip:clip;filename:data:opt;mappings:data:opt;mismatch:int:opt;", replaceCreate, nullptr, plugin);
}