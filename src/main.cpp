#if HAVE_CONFIG_H
#include "config.h"
#endif
#include "cd2flac_version.h"

#include <stdio.h>
#include "getopt.h"
#include "cstr_util.h"
#include "str_util.h"
#include "wchar_util.h"

#include "core.h"
#include "cue.h"
#include "info.h"
#include "open.h"

#if _WIN32
#include "Windows.h"
#endif

#if HAVE_PRINTF_S
#define printf printf_s
#endif

#if HAVE_SSCANF_S
#define sscanf sscanf_s
#endif

#define NO_CDDB 129
#define CDDB_SERVER 130
#define CDDB_PROTOCOL 131
#define CDDB_PORT 132
#define CDDB_NO_TRACK_ARTIST 133
#define LOG_DEBUG 134
#define LOG_TRACE 135
#define LOG_PRINT_LEVEL 136
#define CUE 137

void print_help() {
    printf("%s", "Usage: cd2flac [OPTIONS] DEVICE OUTPUT\n\
Convert Audio CD to FLAC files.\n\
\n\
Options:\n\
    -h, --help              Print this help message.\n\
    -v, --verbose           Enable verbose logging.\n\
    -d, --dry-run           Running without writing files to output.\n\
    -V, --version           Print version.\n\
    --no-cddb               Do not use CDDB.\n\
    --cddb-server [SERVER]  CDDB Server. Default: gnudb.org.\n\
    --cddb-protocol [PROTOCOL]\n\
                            CDDB protocol. Default: http. Supported value: cddbp,\n\
                            http, unknown.\n\
    --cddb-port [PORT]      CDDB Port. Default: 80.\n\
    --cddb-no-track-artist  Disable treat disc artist as track artist when using \n\
                            CDDB.\n\
    --debug                 Enable debug logging.\n\
    --trace                 Enable trace logging.\n\
    --print-level           Print log level.\n\
    --cue                   Treat input device as CUE file.\n");
}

void print_version(bool verbose) {
    printf("cd2flac v%s Copyright (C) 2024  lifegpc\n\
Source code: https://github.com/lifegpc/cd2flac \n\
This program comes with ABSOLUTELY NO WARRANTY;\n\
for details see <https://www.gnu.org/licenses/agpl-3.0.html>.\n\
This is free software, and you are welcome to redistribute it\n\
under certain conditions.\n", CD2FLAC_VERSION);
    print_ffmpeg_version();
    print_cddb_version();
    if (verbose) {
        print_ffmpeg_configuration();
        print_ffmpeg_license();
    } else {
        printf("Add \"-v\" to see more inforamtion about ffmpeg library.\n");
    }
}

void cddb_log_to_av_log(cddb_log_level_t level, const char* message) {
    int target_level = AV_LOG_INFO;
    switch (level) {
    case CDDB_LOG_INFO:
        target_level = AV_LOG_INFO;
        break;
    case CDDB_LOG_DEBUG:
        target_level = AV_LOG_DEBUG;
        break;
    case CDDB_LOG_ERROR:
        target_level = AV_LOG_ERROR;
        break;
    case CDDB_LOG_WARN:
        target_level = AV_LOG_WARNING;
        break;
    case CDDB_LOG_CRITICAL:
        target_level = AV_LOG_FATAL;
        break;
    }
    av_log(nullptr, target_level, "[CDDB] %s\n", message);
}

int main(int argc, char* argv[]) {
#if _WIN32
    SetConsoleOutputCP(CP_UTF8);
    bool have_wargv = false;
    int wargc;
    char** wargv;
    if (wchar_util::getArgv(wargv, wargc)) {
        have_wargv = true;
        argc = wargc;
        argv = wargv;
    }
#endif
    if (argc == 1) {
        print_help();
#if _WIN32
        if (have_wargv) wchar_util::freeArgv(wargv, wargc);
#endif
        return 0;
    }
    struct option opts[] = {
        { "help", 0, nullptr, 'h' },
        { "verbose", 0, nullptr, 'v' },
        { "dry-run", 0, nullptr, 'd' },
        { "version", 0, nullptr, 'V' },
        { "no-cddb", 0, nullptr, NO_CDDB },
        { "cddb-server", 1, nullptr, CDDB_SERVER },
        { "cddb-protocol", 1, nullptr, CDDB_PROTOCOL },
        { "cddb-port", 1, nullptr, CDDB_PORT },
        { "cddb-no-track-artist", 0, nullptr, CDDB_NO_TRACK_ARTIST },
        { "debug", 0, nullptr, LOG_DEBUG },
        { "trace", 0, nullptr, LOG_TRACE },
        { "print-level", 0, nullptr, LOG_PRINT_LEVEL },
        { "cue", 0, nullptr, CUE },
        nullptr,
    };
    int c;
    const char* shortopts = "-hvdV";
    LOGLEVEL level = LL_INFO;
    bool version = false;
    bool dry_run = false;
    std::string device;
    std::string output;
    bool no_cddb = false;
    std::string cddb_server = "gnudb.org";
    cddb_protocol_t cddb_protocol = PROTO_HTTP;
    unsigned int cddb_port = 80;
    bool cddb_no_track_artist = false;
    bool print_level = false;
    bool is_cue = false;
    while ((c = getopt_long(argc, argv, shortopts, opts, nullptr)) != -1) {
        switch (c) {
        case 'h':
            print_help();
#if _WIN32
            if (have_wargv) wchar_util::freeArgv(wargv, wargc);
#endif
            return 0;
        case 'v':
            level = LL_VERBOSE;
            break;
        case 'V':
            version = true;
            break;
        case 'd':
            dry_run = true;
            break;
        case NO_CDDB:
            no_cddb = true;
            break;
        case CDDB_SERVER:
            cddb_server = optarg;
            break;
        case CDDB_PROTOCOL:
            if (cstr_stricmp(optarg, "cddbp") == 0) {
                cddb_protocol = PROTO_CDDBP;
            } else if (cstr_stricmp(optarg, "http") == 0) {
                cddb_protocol = PROTO_HTTP;
            } else if (cstr_stricmp(optarg, "unknown") == 0) {
                cddb_protocol = PROTO_UNKNOWN;
            } else {
                printf("Unknown CDDB protocol: %s\n", optarg);
#if _WIN32
                if (have_wargv) wchar_util::freeArgv(wargv, wargc);
#endif
                return 1;
            }
            break;
        case CDDB_PORT:
            if (sscanf(optarg, "%u", &cddb_port) != 1) {
                av_log(NULL, AV_LOG_FATAL, "Invalid CDDB port: %s\n", optarg);
#if _WIN32
                if (have_wargv) wchar_util::freeArgv(wargv, wargc);
#endif
                return 1;
            }
            break;
        case CDDB_NO_TRACK_ARTIST:
            cddb_no_track_artist = true;
            break;
        case LOG_DEBUG:
            level = LL_DEBUG;
            break;
        case LOG_TRACE:
            level = LL_TRACE;
            break;
        case LOG_PRINT_LEVEL:
            print_level = true;
            break;
        case CUE:
            is_cue = true;
            break;
        case 1:
            if (device.empty()) {
                device = optarg;
            } else if (output.empty()) {
                output = optarg;
            } else {
                av_log(NULL, AV_LOG_FATAL, "Too many arguments.\n");
#if _WIN32
                if (have_wargv) wchar_util::freeArgv(wargv, wargc);
#endif
                return 1;
            }
            break;
        case '?':
        default:
#if _WIN32
            if (have_wargv) wchar_util::freeArgv(wargv, wargc);
#endif
            return 1;
        }
    }
#if _WIN32
    if (have_wargv) wchar_util::freeArgv(wargv, wargc);
#endif
    if (version) {
        print_version(level >= LL_VERBOSE);
        return 0;
    }
    if (device.empty()) {
        av_log(NULL, AV_LOG_FATAL, "Device not specified.\n");
        return 1;
    }
    if (output.empty()) {
        av_log(NULL, AV_LOG_FATAL, "Output not specified.\n");
        return 1;
    }
    switch (level) {
    case LL_INFO:
        av_log_set_level(AV_LOG_INFO);
        break;
    case LL_VERBOSE:
        av_log_set_level(AV_LOG_VERBOSE);
        break;
    case LL_DEBUG:
        av_log_set_level(AV_LOG_DEBUG);
        break;
    case LL_TRACE:
        av_log_set_level(AV_LOG_TRACE);
        break;
    }
    if (print_level) {
        av_log_set_flags(AV_LOG_PRINT_LEVEL);
    }
    if (!no_cddb) {
        cddb_log_set_handler(cddb_log_to_av_log);
    }
    if (!is_cue) {
        if (str_util::str_endswith(device, ".cue")) {
            av_log(NULL, AV_LOG_DEBUG, "Treat input device as CUE file because device ends with .cue.\n");
            is_cue = true;
        }
    }
    Context* ctx = context_new();
    ctx->use_cddb = no_cddb ? 0 : 1;
    ctx->is_cue = is_cue ? 1 : 0;
    if (ctx->use_cddb) {
        cddb_error_t re = init_cddb_site(ctx, cddb_server.c_str(), cddb_port, cddb_protocol);
        if (re != CDDB_ERR_OK) {
            av_log(NULL, AV_LOG_FATAL, "Failed to initialize CDDB site: %s\n", cddb_error_str(re));
            goto end;
        }
    }
    if (ctx->is_cue) {
        if (open_cue(ctx, device.c_str())) {
            av_log(NULL, AV_LOG_FATAL, "Failed to open CUE file: %s\n", device.c_str());
            goto end;
        }
    } else {
        if (open_cd_device(ctx, device.c_str())) {
            av_log(NULL, AV_LOG_FATAL, "Failed to open device: %s\n", device.c_str());
            goto end;
        }
    }
    if (ctx->fmt) av_dump_format(ctx->fmt, 0, device.c_str(), 0);
end:
    context_free(ctx);
    return 0;
}
