#pragma once

#include "core.h"

#ifdef __cplusplus
extern "C" {
#endif

int open_cue(Context* ctx, const char* path);
void free_cue(Context* ctx);

#ifdef __cplusplus
}
#endif
