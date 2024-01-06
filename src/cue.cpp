#if HAVE_CONFIG_H
#include "config.h"
#endif
#include "cue.h"
#include "cue_track_list.h"
#include "info.h"

#include <fcntl.h>
#include <inttypes.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include "cpp2c.h"
#include "cstr_util.h"
#include "err.h"
#include "fileop.h"
#include "file_reader.h"
#include "str_util.h"

#ifdef _WIN32
#ifndef _O_BINARY
#define _O_BINARY 0x8000
#endif

#ifndef _SH_DENYWR
#define _SH_DENYWR 0x20
#endif

#ifndef _S_IREAD
#define _S_IREAD 0x100
#endif
#else
#define _O_BINARY 0
#define _SH_DENYWR 0
#define _S_IREAD 0
#endif

#if HAVE_SSCANF_S
#define sscanf sscanf_s
#endif

int open_cue(Context* ctx, const char* path) {
    int fd = 0;
    FILE* f = nullptr;
    file_reader_file* fr = nullptr;
    bool have_bom = false;
    char* line = nullptr;
    size_t line_len = 0;
    char* tmp = nullptr;
    AVDictionary** m = nullptr;
    LinkedList<CueTrack>* current_track = nullptr;
    auto base = fileop::dirname(path);
    ctx->cue = (CueData*)malloc(sizeof(CueData));
    if (!ctx->cue) {
        av_log(NULL, AV_LOG_FATAL, "Failed to open CUE file: Out of memory.\n");
        return 1;
    }
    memset(ctx->cue, 0, sizeof(CueData));
    int re = fileop::open(path, fd, O_RDONLY | _O_BINARY, _SH_DENYWR, _S_IREAD);
    if (re) {
        std::string errmsg;
        if (!err::get_errno_message(errmsg, re)) {
            errmsg = "Unknown error";
        }
        av_log(NULL, AV_LOG_FATAL, "Failed to open CUE file: %s (%i)\n", errmsg.c_str(), re);
        return 1;
    }
    f = fileop::fdopen(fd, "rb");
    if (!f) {
        fileop::close(fd);
        av_log(NULL, AV_LOG_FATAL, "Failed to open CUE file: fdopen() failed\n");
        return 1;
    }
    fr = create_file_reader(f, 0);
    if (!fr) {
        fileop::fclose(f);
        av_log(NULL, AV_LOG_FATAL, "Failed to open CUE file: create_file_reader() failed: Out of memory.\n");
        return 1;
    }
    unsigned char bom[3];
    memset(bom, 0, sizeof(bom));
    size_t readed = fread(bom, 1, 3, f);
    if (!readed) {
        re = errno;
        std::string errmsg;
        if (!err::get_errno_message(errmsg, re)) {
            errmsg = "Unknown error";
        }
        av_log(NULL, AV_LOG_FATAL, "Failed to parse CUE file: fread() failed: %s (%i)\n", errmsg.c_str(), re);
        goto end;
    }
    av_log(NULL, AV_LOG_DEBUG, "First three bytes: %02x %02x %02x\n", bom[0], bom[1], bom[2]);
    if (bom[0] == 0xEF && bom[1] == 0xBB && bom[2] == 0xBF) {
        have_bom = true;
        av_log(NULL, AV_LOG_DEBUG, "CUE file has BOM\n");
    }
    if (!have_bom) {
        re = fileop::fseek(f, 0, SEEK_SET);
        if (re) {
            re = errno;
            std::string errmsg;
            if (!err::get_errno_message(errmsg, re)) {
                errmsg = "Unknown error";
            }
            av_log(NULL, AV_LOG_FATAL, "Failed to parse CUE file: fseek() failed: %s (%i)\n", errmsg.c_str(), re);
            goto end;
        }
    }
    if (file_reader_read_line(fr, &line, &line_len)) {
        re = 1;
        av_log(NULL, AV_LOG_FATAL, "Failed to parse CUE file: file_reader_read_line() failed: No content in file.\n");
        goto end;
    }
    m = &ctx->cue->metadata;
    while (line) {
        av_log(NULL, AV_LOG_DEBUG, "CUE line: %s\n", line);
        tmp = line;
        while (*tmp && (*tmp == ' ' || *tmp == '\t')) tmp++;
        auto list = str_util::str_splitv(tmp, " ", 3, false);
        if (list.size()) {
            if (!cstr_stricmp(list[0].c_str(), "file")) {
                if (list.size() < 2) {
                    av_log(NULL, AV_LOG_FATAL, "Failed to parse CUE file: file line has no file name.\n");
                    re = 1;
                    goto end;
                }
                auto v = str_util::remove_quote(list[1]);
                if (list.size() > 2) {
                    if (!cstr_stricmp(list[2].c_str(), "WAVE")) {
                    } else if (!cstr_stricmp(list[2].c_str(), "MP3")) {
                    } else {
                        av_log(NULL, AV_LOG_FATAL, "Failed to parse CUE file: Unknown CUE file type: %s\n", list[2].c_str());
                        re = 1;
                        goto end;
                    }
                }
                v = fileop::join(base, v);
                char* path = NULL;
                if (!cpp2c::string2char(v, path)) {
                    av_log(NULL, AV_LOG_FATAL, "Failed to parse CUE file: Out of memory.\n");
                    re = 1;
                    goto end;
                }
                if (!linked_list_append(STR_CPP(ctx->cue->files), &path)) {
                    av_log(NULL, AV_LOG_FATAL, "Failed to parse CUE file: Out of memory.\n");
                    re = 1;
                    goto end;
                }
                av_log(NULL, AV_LOG_DEBUG, "Add FILE %s\n", v.c_str());
            } else if (!cstr_stricmp(list[0].c_str(), "rem")) {
                if (list.size() == 3) {
                    av_dict_set(m, list[1].c_str(), list[2].c_str(), 0);
                    av_log(NULL, AV_LOG_DEBUG, "Add metadata %s = %s\n", list[1].c_str(), list[2].c_str());
                }
            } else if (!cstr_stricmp(list[0].c_str(), "PERFORMER")) {
                auto v = str_util::remove_quote(list[1]);
                av_dict_set(m, "PERFORMER", v.c_str(), 0);
                av_log(NULL, AV_LOG_DEBUG, "Add metadata PERFORMER = %s\n", v.c_str());
            } else if (!cstr_stricmp(list[0].c_str(), "title")) {
                av_log(NULL, AV_LOG_DEBUG, "TITLE = %s\n", list[1].c_str());
                auto v = str_util::remove_quote(list[1]);
                av_dict_set(m, "TITLE", v.c_str(), 0);
                av_log(NULL, AV_LOG_DEBUG, "Add metadata TITLE = %s\n", v.c_str());
            } else if (!cstr_stricmp(list[0].c_str(), "track")) {
                auto type = str_util::remove_quote(list[2]);
                if (cstr_stricmp(type.c_str(), "audio")) {
                    av_log(NULL, AV_LOG_FATAL, "Failed to parse CUE file: Unsupported track type: %s\n", type.c_str());
                    re = 1;
                    goto end;
                }
                uint8_t index = 0;
                if (sscanf(list[1].c_str(), "%" SCNu8, &index) != 1) {
                    av_log(NULL, AV_LOG_FATAL, "Failed to parse CUE file: Invalid track number: %s\n", list[1].c_str());
                    re = 1;
                    goto end;
                }
                if (!linked_list_append(CTL_CPP(ctx->cue->tracks), &current_track)) {
                    av_log(NULL, AV_LOG_FATAL, "Failed to parse CUE file: Out of memory.\n");
                    re = 1;
                    goto end;
                }
                current_track->d.number = index;
                av_log(NULL, AV_LOG_DEBUG, "Add track %" PRIu8 "\n", index);
                m = &current_track->d.metadata;
            } else if (!cstr_stricmp(list[0].c_str(), "index")) {
                uint8_t index_number;
                int64_t index;
                uint8_t min;
                uint8_t sec;
                uint8_t frame;
                if (current_track == nullptr) {
                    av_log(NULL, AV_LOG_FATAL, "Failed to parse CUE file: No track for index.\n");
                    re = 1;
                    goto end;
                }
                if (sscanf(list[1].c_str(), "%" SCNu8, &index_number) != 1) {
                    av_log(NULL, AV_LOG_FATAL, "Failed to parse CUE file: Invalid index number: %s\n", list[1].c_str());
                    re = 1;
                    goto end;
                }
                if (sscanf(list[2].c_str(), "%" SCNu8 ":%" SCNu8 ":%" SCNu8, &min, &sec, &frame) != 3) {
                    av_log(NULL, AV_LOG_FATAL, "Failed to parse CUE file: Invalid index time: %s\n", list[2].c_str());
                    re = 1;
                    goto end;
                }
                index = min * 60 * 75 + sec * 75 + frame;
                current_track->d.index = index;
                current_track->d.index_number = index_number;
                av_log(NULL, AV_LOG_DEBUG, "Add index %" PRIu8 " = %" PRId64 "\n", index_number, index);
            } else if (!cstr_stricmp(list[0].c_str(), "pregap")) {
                int64_t pregap;
                uint8_t min;
                uint8_t sec;
                uint8_t frame;
                if (current_track == nullptr) {
                    av_log(NULL, AV_LOG_FATAL, "Failed to parse CUE file: No track for pregap.\n");
                    re = 1;
                    goto end;
                }
                if (sscanf(list[1].c_str(), "%" SCNu8 ":%" SCNu8 ":%" SCNu8, &min, &sec, &frame) != 3) {
                    av_log(NULL, AV_LOG_FATAL, "Failed to parse CUE file: Invalid pregap time: %s\n", list[1].c_str());
                    re = 1;
                    goto end;
                }
                pregap = min * 60 * 75 + sec * 75 + frame;
                current_track->d.pregap = pregap;
                av_log(NULL, AV_LOG_DEBUG, "Add pregap = %" PRId64 "\n", pregap);
            } else if (!cstr_stricmp(list[0].c_str(), "postgap")) {
                int64_t postgap;
                uint8_t min;
                uint8_t sec;
                uint8_t frame;
                if (current_track == nullptr) {
                    av_log(NULL, AV_LOG_FATAL, "Failed to parse CUE file: No track for postgap.\n");
                    re = 1;
                    goto end;
                }
                if (sscanf(list[1].c_str(), "%" SCNu8 ":%" SCNu8 ":%" SCNu8, &min, &sec, &frame) != 3) {
                    av_log(NULL, AV_LOG_FATAL, "Failed to parse CUE file: Invalid postgap time: %s\n", list[1].c_str());
                    re = 1;
                    goto end;
                }
                postgap = min * 60 * 75 + sec * 75 + frame;
                current_track->d.postgap = postgap;
                av_log(NULL, AV_LOG_DEBUG, "Add postgap = %" PRId64 "\n", postgap);
            }  else {
                av_log(NULL, AV_LOG_WARNING, "Unknown CUE line: %s\n", line);
            }
        }
        free(line);
        line = nullptr;
        if (file_reader_read_line(fr, &line, &line_len)) {
            break;
        }
    }
    av_log(NULL, AV_LOG_DEBUG, "Tracks count: %zu\n", linked_list_count(CTL_CPP(ctx->cue->tracks)));
    av_log(NULL, AV_LOG_INFO, "Infomations from CUE file:\n");
    print_cue(ctx);
end:
    fileop::fclose(f);
    free_file_reader(fr);
    if (line) free(line);
    return re;
}

void free_cue_track(CueTrack track) {
    av_dict_free(&track.metadata);
}

void free_char(char* path) {
    if (path) free(path);
}

void free_cue(Context* ctx) {
    if (!ctx) return;
    if (ctx->cue) {
        av_dict_free(&ctx->cue->metadata);
        linked_list_clear(CTL_CPP(ctx->cue->tracks), free_cue_track);
        linked_list_clear(STR_CPP(ctx->cue->files), free_char);
        free(ctx->cue);
        ctx->cue = nullptr;
    }
}
