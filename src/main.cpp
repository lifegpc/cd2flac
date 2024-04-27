#if HAVE_CONFIG_H
#include "config.h"
#endif
#include "cd2flac_version.h"

#include <stdio.h>
#include <vector>
#include "getopt.h"
#include "cstr_util.h"
#include "fileop.h"
#include "str_util.h"
#include "wchar_util.h"

#if HAVE_INIH
#include "./INIReader.h"
#endif

#include "163.h"
#include "core.h"
#include "cue.h"
#include "info.h"
#include "music_brain_z.h"
#include "open.h"

#if _WIN32
#include <stdlib.h>
#include "Windows.h"
#endif

#if HAVE_PRINTF_S
#define printf printf_s
#endif

#if HAVE_SSCANF_S
#define sscanf sscanf_s
#endif

#if HAVE_SCANF_S
#define scanf scanf_s
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
#define COOKIES 138

#if HAVE_INIH
const std::string default_config_file() {
#if _WIN32
    wchar_t* appdata = nullptr;
    size_t len = 0;
    if (_wdupenv_s(&appdata, &len, "APPDATA")) return "";
    std::wstring appdata_w(appdata);
    free(appdata);
    std::string app;
    if (!wchar_util::wstr_to_str(app, appdata_w, CP_UTF8)) return "";
#else
    auto home = getenv("HOME");
    if (!home) return "";
    std::string app = fileop::join(home, ".config");
#endif
    return fileop::join(app, "cd2flac.ini");
}
#endif

std::string default_163_cookies_file() {
#if _WIN32
    wchar_t* appdata = nullptr;
    size_t len = 0;
    if (_wdupenv_s(&appdata, &len, "APPDATA")) return "";
    std::wstring appdata_w(appdata);
    free(appdata);
    std::string app;
    if (!wchar_util::wstr_to_str(app, appdata_w, CP_UTF8)) return "";
#else
    auto home = getenv("HOME");
    if (!home) return "";
    std::string app = fileop::join(home, ".config");
#endif
    return fileop::join(app, "cd2flac_163.txt");
}

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
#if HAVE_INIH
    printf("\
    -c, --config [PATH]     Config file location. Default: cd2flac.ini, %s.\n", default_config_file().c_str());
#endif
}

void print_163_help() {
    printf("Usage: cd2flac 163 [OPTIONS] ACTIONS\n\
\n\
Actions:\n\
    fetchSongDetail ID [ID [ID ...]]\n\
                            Fetch song detail.\n\
    fetchAlbum ID           Fetch album detail.\n\
    fetchLyric ID           Fetch lyric.\n\
    fetchLoginStatus        Fetch login status.\n\
    fetchSongUrl ID [ID [ID ...]]\n\
                            Fetch song URL.\n\
    search KEYWORDS [TYPE] [LIMIT] [OFFSET]\n\
                            Search keywords.\n\
    sentSms [+CONTRYCODE] PHONE\n\
                            Send CAPTCHA with SMS.\n\
    loginWithSms [+CONTRYCODE] PHONE [CAPTCHA]\n\
                            Login with SMS CAPTCHA.\n\
    loginRefresh            Refresh login.\n\
Options:\n\
    -h, --help              Print this help message.\n\
    -v, --verbose           Enable verbose logging.\n\
    -V, --version           Print version.\n\
    --debug                 Enable debug logging.\n\
    --trace                 Enable trace logging.\n\
    --print-level           Print log level.\n\
    --cookies [PATH]        Cookies file location. Default: %s.\n",
    default_163_cookies_file().c_str());
}

void print_mbz_help() {
    printf("Usage: cd2flac mbz [OPTIONS] ACTIONS\n\
\n\
Actions:\n\
    lookup ENTITY_TYPE MBID [INC ...]\n\
                            Lookup of an entity.\n\
Options:\n\
    -h, --help              Print this help message.\n\
    -v, --verbose           Enable verbose logging.\n\
    -V, --version           Print version.\n\
    --debug                 Enable debug logging.\n\
    --trace                 Enable trace logging.\n\
    --print-level           Print log level.\n");
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
        { "cookies", 1, nullptr, COOKIES },
#if HAVE_INIH
        { "config", 1, nullptr, 'c' },
#endif
        nullptr,
    };
    int c;
    std::string shortopts = "-hvdV";
#if HAVE_INIH
    shortopts += "c:";
#endif
    LOGLEVEL level = LL_INFO;
    bool version = false;
    bool dry_run = false;
    std::vector<std::string> args;
    std::string device;
    std::string output;
    bool no_cddb = false;
    std::string cddb_server = "gnudb.org";
    cddb_protocol_t cddb_protocol = PROTO_HTTP;
    unsigned int cddb_port = 80;
    bool cddb_no_track_artist = false;
    bool print_level = false;
    bool is_cue = false;
    bool n163 = false;
    bool nmbz = false;
    bool printh = false;
    std::string n163_cookies;
#if HAVE_INIH
    std::string config;
    bool cddb_server_set = false;
    bool cddb_protocol_set = false;
    bool cddb_port_set = false;
    bool n163_cookies_set = false;
#endif
    while ((c = getopt_long(argc, argv, shortopts.c_str(), opts, nullptr)) != -1) {
        switch (c) {
        case 'h':
            printh = true;
            break;
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
#if HAVE_INIH
            cddb_server_set = true;
#endif
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
#if HAVE_INIH
            cddb_protocol_set = true;
#endif
            break;
        case CDDB_PORT:
            if (sscanf(optarg, "%u", &cddb_port) != 1) {
                av_log(NULL, AV_LOG_FATAL, "Invalid CDDB port: %s\n", optarg);
#if _WIN32
                if (have_wargv) wchar_util::freeArgv(wargv, wargc);
#endif
                return 1;
            }
#if HAVE_INIH
            cddb_port_set = true;
#endif
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
#if HAVE_INIH
        case 'c':
            config = optarg;
            break;
#endif
        case COOKIES:
            n163_cookies = optarg;
#if HAVE_INIH
            n163_cookies_set = true;
#endif
            break;
        case 1:
            args.push_back(optarg);
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
    if (!args.size()) {
        if (printh) {
            print_help();
            return 0;
        }
        av_log(NULL, AV_LOG_FATAL, "Device not specified.\n");
        return 1;
    }
    if (args[0] == "163") n163 = true;
    else if (args[0] == "mbz") nmbz = true;
    else device = args[0];
    if (printh) {
        if (n163) print_163_help();
        else if (nmbz) print_mbz_help();
        else print_help();
        return 0;
    }
    if (!device.empty()) {
        if (args.size() > 1) {
            output = args[1];
        } else {
            av_log(NULL, AV_LOG_FATAL, "Output not specified.\n");
            return 1;
        }
    }
    if (!is_cue && !n163) {
        if (str_util::str_endswith(device, ".cue")) {
            av_log(NULL, AV_LOG_DEBUG, "Treat input device as CUE file because device ends with .cue.\n");
            is_cue = true;
        }
    }
#if HAVE_INIH
    if (!config.empty() && !fileop::exists(config)) {
        av_log(NULL, AV_LOG_FATAL, "Config file not exists: %s\n", config.c_str());
        return 1;
    } else if (config.empty()) {
        if (fileop::exists("cd2flac.ini")) {
            config = "cd2flac.ini";
        } else {
            auto tmp = default_config_file();
            if (!tmp.empty() && fileop::exists(tmp)) {
                config = tmp;
            }
        }
    }
    if (!config.empty()) {
        av_log(NULL, AV_LOG_DEBUG, "Using config file: %s\n", config.c_str());
        INIReader reader(config);
        auto error = reader.ParseError();
        if (error == -1) {
            av_log(NULL, AV_LOG_FATAL, "Failed to open config file: %s\n", config.c_str());
            return 1;
        } else if (error > 0) {
            av_log(NULL, AV_LOG_FATAL, "Failed to parse config file %s: Syntax error on line %i\n", config.c_str(), error);
            return 1;
        }
        if (!no_cddb) {
            no_cddb = !reader.GetBoolean("cddb", "enabled", true);
        }
        if (!cddb_server_set) {
            cddb_server = reader.Get("cddb", "server", "gnudb.org");
        }
        if (!cddb_protocol_set) {
            auto protocol = reader.Get("cddb", "protocol", "http");
            if (cstr_stricmp(protocol.c_str(), "cddbp") == 0) {
                cddb_protocol = PROTO_CDDBP;
            } else if (cstr_stricmp(protocol.c_str(), "http") == 0) {
                cddb_protocol = PROTO_HTTP;
            } else if (cstr_stricmp(protocol.c_str(), "unknown") == 0) {
                cddb_protocol = PROTO_UNKNOWN;
            } else {
                av_log(NULL, AV_LOG_FATAL, "Unknown CDDB protocol: %s\n", protocol.c_str());
                return 1;
            }
        }
        if (!cddb_port_set) {
            cddb_port = reader.GetInteger("cddb", "port", 80);
        }
        if (!cddb_no_track_artist) {
            cddb_no_track_artist = reader.GetBoolean("cddb", "no_track_artist", false);
        }
        if (!n163_cookies_set) {
            n163_cookies = reader.Get("163", "cookies", default_163_cookies_file());
        }
    }
#endif
    if (n163) {
        NeteaseMusicApi api(n163_cookies);
        if (args.size() == 1) {
            print_163_help();
            return 0;
        }
        std::string act = args[1];
        if (!cstr_strcasecmp(act.c_str(), "fetchSongDetail")) {
            if (args.size() == 2) {
                av_log(NULL, AV_LOG_FATAL, "At least one song id needed.\n");
                return 1;
            }
            std::list<std::uint64_t> ids;
            uint64_t t;
            for (size_t i = 2; i < args.size(); i++) {
                if (sscanf(args[i].c_str(), "%" SCNu64, &t) != 1) {
                    av_log(NULL, AV_LOG_FATAL, "Invalid song id: %s\n", args[i].c_str());
                    return 1;
                }
                ids.push_back(t);
            }
            try {
                auto re = api.fetchSongDetails(ids);
                printf("%s\n", api.toJson(re).c_str());
            } catch (std::exception& e) {
                av_log(NULL, AV_LOG_FATAL, "Failed to fetch song details: %s\n", e.what());
                return 1;
            }
        } else if (!cstr_stricmp(act.c_str(), "fetchAlbum")) {
            if (args.size() < 3) {
                av_log(NULL, AV_LOG_FATAL, "Album id needed.\n");
                return 1;
            }
            uint64_t id;
            if (sscanf(args[2].c_str(), "%" SCNu64, &id) != 1) {
                av_log(NULL, AV_LOG_FATAL, "Invalid album id: %s\n", args[2].c_str());
                return 1;
            }
            try {
                auto re = api.fetchAlbum(id);
                printf("%s\n", api.toJson(re).c_str());
            } catch (std::exception& e) {
                av_log(NULL, AV_LOG_FATAL, "Failed to fetch album: %s\n", e.what());
                return 1;
            }
        } else if (!cstr_stricmp(act.c_str(), "fetchLyric")) {
            if (args.size() < 3) {
                av_log(NULL, AV_LOG_FATAL, "Song id needed.\n");
                return 1;
            }
            uint64_t id;
            if (sscanf(args[2].c_str(), "%" SCNu64, &id) != 1) {
                av_log(NULL, AV_LOG_FATAL, "Invalid song id: %s\n", args[2].c_str());
                return 1;
            }
            try {
                auto re = api.fetchLyric(id);
                printf("%s\n", api.toJson(re).c_str());
            } catch (std::exception& e) {
                av_log(NULL, AV_LOG_FATAL, "Failed to fetch lyric: %s\n", e.what());
                return 1;
            }
        } else if (!cstr_stricmp(act.c_str(), "fetchLoginStatus")) {
            try {
                auto re = api.fetchLoginStatus();
                printf("%s\n", api.toJson(re).c_str());
            } catch (std::exception& e) {
                av_log(NULL, AV_LOG_FATAL, "Failed to fetch login status: %s\n", e.what());
                return 1;
            }
        } else if (!cstr_stricmp(act.c_str(), "sentSms")) {
            if (args.size() < 3) {
                av_log(NULL, AV_LOG_FATAL, "Phone number needed.\n");
                return 1;
            }
            std::string phone;
            std::string country_code;
            if (args.size() > 3) {
                phone = args[3];
                country_code = args[2];
            } else {
                phone = args[2];
                country_code = "+86";
            }
            try {
                auto re = api.sentSms(phone, country_code);
                auto code = re["code"].GetInt();
                if (code != 200) {
                    av_log(NULL, AV_LOG_FATAL, "Failed to sent SMS: %s\n", re["msg"].GetString());
                    return 1;
                }
                av_log(NULL, AV_LOG_INFO, "Sent successfully.\n");
            } catch (std::exception& e) {
                av_log(NULL, AV_LOG_FATAL, "Failed to sent SMS: %s\n", e.what());
                return 1;
            }
        } else if (!cstr_stricmp(act.c_str(), "loginWithSms")) {
            if (args.size() < 3) {
                av_log(NULL, AV_LOG_FATAL, "Phone number needed.\n");
                return 1;
            }
            std::string phone;
            std::string country_code = "+86";
            std::string captcha;
            if (args.size() >= 5) {
                country_code = args[2];
                phone = args[3];
                captcha = args[4];
            } else if (args.size() == 4) {
                if (args[2].find("+") == 0) {
                    country_code = args[2];
                    phone = args[3];
                } else {
                    phone = args[2];
                    captcha = args[3];
                }
            } else {
                phone = args[2];
            }
            if (captcha.empty()) {
                try {
                    auto re = api.sentSms(phone, country_code);
                    auto code = re["code"].GetInt();
                    if (code != 200) {
                        av_log(NULL, AV_LOG_FATAL, "Failed to sent SMS: %s\n", re["msg"].GetString());
                        return 1;
                    }
                } catch (std::exception& e) {
                    av_log(NULL, AV_LOG_FATAL, "Failed to sent SMS: %s\n", e.what());
                    return 1;
                }
                printf("Please input captcha: ");
                char buf[1024];
                if (scanf("%1023s", buf) != 1) {
                    av_log(NULL, AV_LOG_FATAL, "Failed to read captcha.\n");
                    return 1;
                }
                captcha = buf;
            }
            try {
                auto re = api.loginWithSms(phone, captcha, country_code);
                auto code = re["code"].GetInt();
                if (code != 200) {
                    av_log(NULL, AV_LOG_FATAL, "Failed to login: %s\n", re["msg"].GetString());
                    return 1;
                }
                av_log(NULL, AV_LOG_INFO, "Login successfully.\n");
            } catch (std::exception& e) {
                av_log(NULL, AV_LOG_FATAL, "Failed to login: %s\n", e.what());
                return 1;
            }
        } else if (!cstr_stricmp(act.c_str(), "loginRefresh")) {
            try {
                auto re = api.loginRefresh();
                auto code = re["code"].GetInt();
                if (code != 200) {
                    av_log(NULL, AV_LOG_FATAL, "Failed to refresh login: %s\n", re["msg"].GetString());
                    return 1;
                }
                av_log(NULL, AV_LOG_INFO, "Refreshed successfully.\n");
            } catch (std::exception& e) {
                av_log(NULL, AV_LOG_FATAL, "Failed to refresh login: %s\n", e.what());
                return 1;
            }
        } else if (!cstr_stricmp(act.c_str(), "fetchSongUrl")) {
            if (args.size() == 2) {
                av_log(NULL, AV_LOG_FATAL, "At least one song id needed.\n");
                return 1;
            }
            std::list<std::uint64_t> ids;
            uint64_t t;
            for (size_t i = 2; i < args.size(); i++) {
                if (sscanf(args[i].c_str(), "%" SCNu64, &t) != 1) {
                    av_log(NULL, AV_LOG_FATAL, "Invalid song id: %s\n", args[i].c_str());
                    return 1;
                }
                ids.push_back(t);
            }
            try {
                auto re = api.fetchSongUrl(ids);
                printf("%s\n", api.toJson(re).c_str());
            } catch (std::exception& e) {
                av_log(NULL, AV_LOG_FATAL, "Failed to fetch song URL: %s\n", e.what());
                return 1;
            }
        } else if (!cstr_stricmp(act.c_str(), "search")) {
            if (args.size() < 3) {
                av_log(NULL, AV_LOG_FATAL, "Keywords needed.\n");
                return 1;
            }
            std::string keywords = args[2];
            int64_t type = 1;
            int64_t limit = 30;
            int64_t offset = 0;
            if (args.size() > 3) {
                if (sscanf(args[3].c_str(), "%" SCNd64, &type) != 1) {
                    av_log(NULL, AV_LOG_FATAL, "Invalid type: %s\n", args[3].c_str());
                    return 1;
                }
            }
            if (args.size() > 4) {
                if (sscanf(args[4].c_str(), "%" SCNd64, &limit) != 1) {
                    av_log(NULL, AV_LOG_FATAL, "Invalid limit: %s\n", args[4].c_str());
                    return 1;
                }
            }
            if (args.size() > 5) {
                if (sscanf(args[5].c_str(), "%" SCNd64, &offset) != 1) {
                    av_log(NULL, AV_LOG_FATAL, "Invalid offset: %s\n", args[5].c_str());
                    return 1;
                }
            }
            try {
                auto re = api.search(keywords, type, limit, offset);
                printf("%s\n", api.toJson(re).c_str());
            } catch (std::exception& e) {
                av_log(NULL, AV_LOG_FATAL, "Failed to search: %s\n", e.what());
                return 1;
            }
        } else {
            av_log(NULL, AV_LOG_FATAL, "Unknown action: %s\n", act.c_str());
            return 1;
        }
        return 0;
    }
    if (nmbz) {
        MusicBrainZApi api;
        if (args.size() == 1) {
            print_mbz_help();
            return 0;
        }
        std::string act = args[1];
        if (!cstr_stricmp(act.c_str(), "lookup")) {
            if (args.size() < 4) {
                av_log(NULL, AV_LOG_FATAL, "Entity type and MBID needed.\n");
                return 1;
            }
            std::string entity = args[2];
            std::string mbid = args[3];
            std::list<std::string> inc;
            for (size_t i = 4; i < args.size(); i++) {
                inc.push_back(args[i]);
            }
            try {
                auto re = api.lookup(entity, mbid, inc);
                printf("%s\n", api.toJson(re).c_str());
            } catch (std::exception& e) {
                av_log(NULL, AV_LOG_FATAL, "Failed to lookup: %s\n", e.what());
                return 1;
            }
        }
        return 0;
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
        if (cddb_no_track_artist) {
            libcddb_set_flags(CDDB_F_NO_TRACK_ARTIST);
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
end:
    context_free(ctx);
    return 0;
}
