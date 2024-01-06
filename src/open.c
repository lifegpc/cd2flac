#include "open.h"

#include <string.h>

const AVInputFormat* find_libcdio() {
    const AVInputFormat* f = NULL;
    f = av_input_audio_device_next(f);
    while (f) {
        if (f && !strcmp(f->name, "libcdio")) return f;
        f = av_input_audio_device_next(f);
    }
    return NULL;
}

int open_cd_device(Context* ctx, const char* device) {
    if (!ctx || !device) {
        av_log(NULL, AV_LOG_FATAL, "Invalid arguments.\n");
        return 1;
    }
    avdevice_register_all();
    const AVInputFormat* f = find_libcdio();
    if (!f) {
        av_log(NULL, AV_LOG_FATAL, "libcdio not found.\n");
        return 1;
    }
    int re = 0;
    AVDictionary* opts = NULL;
    av_dict_set(&opts, "paranoia_mode", "verify", 0);
    if ((re = avformat_open_input(&ctx->fmt, device, f, &opts)) < 0) {
        av_log(NULL, AV_LOG_FATAL, "Failed to open device: %s\n", av_err2str(re));
        av_dict_free(&opts);
        return 1;
    }
    av_dict_free(&opts);
    if ((re = avformat_find_stream_info(ctx->fmt, NULL)) < 0) {
        av_log(NULL, AV_LOG_FATAL, "Failed to find stream info: %s\n", av_err2str(re));
        return 1;
    }
    av_dump_format(ctx->fmt, 0, device, 0);
    return 0;
}
