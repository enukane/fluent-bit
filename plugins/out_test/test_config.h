#ifndef FLB_TEST_CONFIG_H
#define FLB_TEST_CONFIG_H

#include <mk_config/mk_config.h>

struct flb_out_test_config {
    int val0;
    int bool0;
    char *str0;
    struct mk_list *list0;
};

struct flb_out_test_config *test_config_init(struct mk_config *conf);

#endif /* FLB_TEST_CONFIG_H */
