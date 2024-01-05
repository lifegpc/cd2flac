#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include "info.h"
#include "libavcodec/avcodec.h"
#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libswresample/swresample.h"
#include "cddb/version.h"

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
