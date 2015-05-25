#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <fluent-bit/flb_output.h>
#include <fluent-bit/flb_utils.h>
#include <fluent-bit/flb_network.h>

#include <mk_config/mk_config.h>
#include <mk_config/mk_string.h>

#include "test.h"
#include "test_config.h"

struct flb_output_plugin out_test_plugin;

int cb_test_init(struct flb_config *config)
{
    int ret;
    struct flb_out_test_config *ctx;
    struct mk_list *head;
    struct mk_string_line *entry;

    printf("%u %s : do init (host=%s, port=%d)\n", __LINE__, __func__,
           out_test_plugin.host, out_test_plugin.port);

    if (!config->file) {
        flb_utils_error_c("TEST output requires a configuration file");
    }

    ctx = test_config_init(config->file);
    if (!ctx) {
        perror("calloc");
        return -1;
    }

    printf("Config is: \n");
    printf("  val0  = %d\n",  ctx->val0);
    printf("  bool0 = %d\n", ctx->bool0);
    printf("  str0  = %s\n", ctx->str0);
    printf("  list0 = ");
    if (ctx->list0) {
        mk_list_foreach(head, ctx->list0) {
            entry = mk_list_entry(head, struct mk_string_line, _head);
            printf("'%s' ", entry->val);
        }
        printf("\n");
    }

    ret = flb_output_set_context("test", ctx, config);
    if (ret == -1) {
        flb_utils_error_c("Could not set configuration for test output plugin");
        return -1;
    }

    return 0;
}

int cb_test_pre_run(void *out_context, struct flb_config *config)
{
    struct flb_out_test_config *ctx = (struct flb_out_test_config *)out_context;

    printf("%u %s : do pre run (host=%s, port=%d)\n", __LINE__, __func__,
           out_test_plugin.host, out_test_plugin.port);

    return 0;
}

int cb_test_flush(void *data, size_t bytes, void *out_context)
{
    printf("%u %s : do flush (%zd bytes)\n", __LINE__, __func__, bytes);
    return bytes;
}

/* Plugin reference */
struct flb_output_plugin out_test_plugin = {
    .name           = "test",
    .description    = "Test Output Plugin",
    .cb_init        = cb_test_init,
    .cb_pre_run     = cb_test_pre_run,
    .cb_flush       = cb_test_flush,
    .flags          = FLB_OUTPUT_TCP,
};
