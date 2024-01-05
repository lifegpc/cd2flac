#include "core.h"

#include <malloc.h>
#include <string.h>

Context* context_new() {
    Context* ctx = (Context*)malloc(sizeof(Context));
    memset(ctx, 0, sizeof(Context));
    return ctx;
}

void context_free(Context* ctx) {
    if (!ctx) return;
    if (ctx->cddb_site) {
        cddb_site_destroy(ctx->cddb_site);
    }
    if (ctx->fmt) avformat_close_input(&ctx->fmt);
    free(ctx);
}

cddb_error_t init_cddb_site(Context* ctx, const char* host, unsigned int port, cddb_protocol_t protocol) {
    if (!ctx || !host) return CDDB_ERR_INVALID;
    cddb_error_t re = CDDB_ERR_OK;
    ctx->cddb_site = cddb_site_new();
    if (!ctx->cddb_site) return CDDB_ERR_OUT_OF_MEMORY;
    if ((re = cddb_site_set_address(ctx->cddb_site, host, port)) != CDDB_ERR_OK) {
        goto err;
    }
    if ((re = cddb_site_set_protocol(ctx->cddb_site, protocol)) != CDDB_ERR_OK) {
        goto err;
    }
    return re;
err:
    cddb_site_destroy(ctx->cddb_site);
    ctx->cddb_site = nullptr;
    return re;
}
