#pragma once

#include "core.h"

#ifdef __cplusplus
extern "C" {
#endif

void print_ffmpeg_version();
void print_cddb_version();
void print_ffmpeg_configuration();
void print_ffmpeg_license();

void print_avdict(const AVDictionary* dict, int indent);
void print_cue(Context* ctx);

#ifdef __cplusplus
}
#endif
