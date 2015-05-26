#ifndef FLB_IN_TEST_H
#define FLB_IN_TEST_H

#include <fluent-bit/flb_config.h>
#include <fluent-bit/flb_input.h>
#include <fluent-bit/flb_utils.h>

/* Collection time: every 1 sec */
#define IN_TEST_COLLECT_SEC     1
#define IN_TEST_COLLECT_NSEC    0

struct in_test_data {
    time_t      time;
    int         value;
};

/* configuration & context */
struct flb_in_test_config {

    /* BUffered dta */
    int data_idx;
    int data_size;

    /* the data (ring buffer) */
    struct in_test_data *data_array;
};

#endif
