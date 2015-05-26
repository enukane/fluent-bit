#include <stdio.h>
#include <stdlib.h>

#include <msgpack.h>
#include <fluent-bit/flb_input.h>
#include <fluent-bit/flb_config.h>

#include "in_test.h"


static int in_test_collect(struct flb_config *config, void *in_context);

static int in_test_init(struct flb_config *config)
{
    int ret;
    struct flb_in_test_config *ctx;

    printf("%u %s : do init (buffersize=%d)\n", __LINE__, __func__, config->flush);

    ctx = calloc(1, sizeof(struct flb_in_test_config));
    if (!ctx) {
        return -1;
    }

    /* prepare buffer */
    ctx->data_idx   = 0;
    ctx->data_size  = config->flush;
    ctx->data_array = malloc(sizeof(struct in_test_data) * ctx->data_size);
    if (!ctx->data_array) {
        flb_utils_error_c("Could not create data array for TEST input plugin");
    }

    /* Set the context */
    ret = flb_input_set_context("test", ctx, config);
    if (ret == -1) {
        flb_utils_error_c("Could not set configuration for CPU input plugin");
    }

    /* Set our collector based on time, every 1 sec */
    ret = flb_input_set_collector_time("test",
                                       in_test_collect,
                                       IN_TEST_COLLECT_SEC,
                                       IN_TEST_COLLECT_NSEC,
                                       config);
    if (ret == -1) {
        flb_utils_error_c("Could not set collector for TEST input plugin");
    }

    return 0;
}

static int in_test_pre_run(void *in_context, struct flb_config *config)
{
    struct flb_in_test_config *ctx = in_context;

    printf("%u %s : do pre run\n", __LINE__, __func__);

    return 0;
}

static int in_test_collect(struct flb_config *config, void *in_context)
{
    static int current = 0; /* dummy current data */
    struct flb_in_test_config *ctx = in_context;
    struct in_test_data *buf;

    printf("%u %s : do collect\n", __LINE__, __func__);

    if (ctx->data_idx == ctx->data_size) {
        ctx->data_idx = 0;
    }
    buf         = &ctx->data_array[ctx->data_idx];
    buf->time   = time(NULL);
    buf->value  = current;

    printf("%u %s : collect '%d' to %dth buffer\n", __LINE__, __func__,
           current, ctx->data_idx);

    ctx->data_idx++;
    flb_debug("[in_test] CURRENT %d (buffer = %i)", current, ctx->data_idx - 1);

    current++;

    return 0;
}

void *in_test_flush(void *in_context, int *size)
{
    int i;
    struct flb_in_test_config *ctx = in_context;
    struct in_test_data *data;

    char *buf;
    msgpack_packer pck;
    msgpack_sbuffer sbuf;

    printf("%u %s : do flush \n", __LINE__, __func__);

    /* initialize buffers */
    msgpack_sbuffer_init(&sbuf);
    msgpack_packer_init(&pck, &sbuf, msgpack_sbuffer_write);

    msgpack_pack_array(&pck, ctx->data_size);

    for (i = 0; i < ctx->data_idx; i++) {
        data = &ctx->data_array[i];
        msgpack_pack_uint64(&pck, data->value);
        printf("%u %s : packed[%d] = %d\n", __LINE__, __func__, i, data->value);
    }

    *size = sbuf.size;
    buf = malloc(sbuf.size);
    memcpy(buf, sbuf.data, sbuf.size);
    msgpack_sbuffer_destroy(&sbuf);

    printf("%u %s : buffer = [ ", __LINE__, __func__);
    for (i = 0; i < *size; i++) {
        printf("%02x ", buf[i]);
    }
    printf("]\n");

    printf("%u %s : flushed (size=%d)\n", __LINE__, __func__, *size);

    ctx->data_idx = 0;

    return buf;
}

struct flb_input_plugin in_test_plugin = {
    .name           = "test",
    .description    = "Test input",
    .cb_init        = in_test_init,
    .cb_pre_run     = in_test_pre_run,
    .cb_collect     = in_test_collect,
    .cb_flush_buf   = in_test_flush,
};
