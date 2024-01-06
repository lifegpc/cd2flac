#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "libavcodec/avcodec.h"
#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libswresample/swresample.h"
#include "cddb/cddb.h"

typedef enum LOGLEVEL {
    LL_INFO,
    LL_VERBOSE,
    LL_DEBUG,
    LL_TRACE,
} LOGLEVEL;

typedef struct CueTrack {
    AVDictionary* metadata;
    uint8_t number;
    uint8_t index_number;
    int64_t index;
    int64_t pregap;
    int64_t postgap;
} CueTrack;

typedef struct CueTrackList {
    CueTrack d;
    struct CueTrackList* prev;
    struct CueTrackList* next;
} CueTrackList;

typedef struct FileList {
    char* path;
    struct FileList* prev;
    struct FileList* next;
} FileList;

typedef struct CueData {
    AVDictionary* metadata;
    CueTrackList* tracks;
    FileList* files;
} CueData;

typedef struct Context {
    cddb_site_t* cddb_site;
    AVFormatContext* fmt;
    CueData* cue;
    unsigned char use_cddb: 1;
    unsigned char is_cue: 1;
} Context;

Context* context_new();
void context_free(Context* ctx);
cddb_error_t init_cddb_site(Context* ctx, const char* host, unsigned int port, cddb_protocol_t protocol);

#ifdef __cplusplus
}
#endif
