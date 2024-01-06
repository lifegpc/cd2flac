#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <inttypes.h>
#include <stdio.h>
#include "info.h"

#include "libavutil/timestamp.h"

#if HAVE_PRTINF_S
#define printf printf_s
#endif

void print_ffmpeg_version() {
    unsigned int version = avcodec_version();
    printf("libavcodec %u.%u.%u\n", version >> 16, (version >> 8) & 0xff, version & 0xff);
    version = avdevice_version();
    printf("libavdevice %u.%u.%u\n", version >> 16, (version >> 8) & 0xff, version & 0xff);
    version = avformat_version();
    printf("libavformat %u.%u.%u\n", version >> 16, (version >> 8) & 0xff, version & 0xff);
    version = avutil_version();
    printf("libavutil %u.%u.%u\n", version >> 16, (version >> 8) & 0xff, version & 0xff);
    version = swresample_version();
    printf("libswresample %u.%u.%u\n", version >> 16, (version >> 8) & 0xff, version & 0xff);
}

void print_cddb_version() {
    printf("libcddb %s\n", CDDB_VERSION);
}

void print_ffmpeg_configuration() {
    printf("configuration: %s\n", avcodec_configuration());
}

void print_ffmpeg_license() {
    printf("license: %s\n", avcodec_license());
}

void print_avdict(const AVDictionary* dict, int indent) {
    if (!dict) {
        av_log(NULL, AV_LOG_INFO, "Empty dict\n");
        return;
    }
    AVDictionaryEntry* entry = NULL;
    while ((entry = av_dict_get(dict, "", entry, AV_DICT_IGNORE_SUFFIX))) {
        for (int i = 0; i < indent; i++) av_log(NULL, AV_LOG_INFO, " ");
        av_log(NULL, AV_LOG_INFO, "%s: %s\n", entry->key, entry->value);
    }
}

void print_track_info(CueTrack track, int indent) {
    for (int i = 0; i < indent; i++) av_log(NULL, AV_LOG_INFO, " ");
    av_log(NULL, AV_LOG_INFO, "Track %02d:\n", track.number);
    for (int i = 0; i < indent + 2; i++) av_log(NULL, AV_LOG_INFO, " ");
    AVRational tb = {1, 75};
    av_log(NULL, AV_LOG_INFO, "Index: %s\n", av_ts2timestr(track.index, &tb));
    for (int i = 0; i < indent + 2; i++) av_log(NULL, AV_LOG_INFO, " ");
    av_log(NULL, AV_LOG_INFO, "index number: %" PRIu8 "\n", track.index_number);
    for (int i = 0; i < indent + 2; i++) av_log(NULL, AV_LOG_INFO, " ");
    av_log(NULL, AV_LOG_INFO, "Pre gap: %s\n", av_ts2timestr(track.pregap, &tb));
    for (int i = 0; i < indent + 2; i++) av_log(NULL, AV_LOG_INFO, " ");
    av_log(NULL, AV_LOG_INFO, "Post gap: %s\n", av_ts2timestr(track.postgap, &tb));
    for (int i = 0; i < indent + 2; i++) av_log(NULL, AV_LOG_INFO, " ");
    av_log(NULL, AV_LOG_INFO, "Metadata: \n");
    print_avdict(track.metadata, indent + 4);
}

void print_cue(Context* ctx) {
    if (!ctx->cue) return;
    av_log(NULL, AV_LOG_INFO, "Disc metadata:\n");
    print_avdict(ctx->cue->metadata, 2);
    av_log(NULL, AV_LOG_INFO, "Tracks:\n");
    CueTrackList* cur = ctx->cue->tracks;
    while (cur) {
        print_track_info(cur->d, 2);
        cur = cur->next;
    }
}
