#include <stdio.h>
#include <stdlib.h>
#include <mk_config/mk_config.h>
#include <mk_config/mk_string.h>
#include <fluent-bit/flb_utils.h>

#include "test_config.h"

struct flb_out_test_config *test_config_init(struct mk_config *conf)
{
    int val0, val1;
    int bool0, bool1;
    char *str0, *str1;
    struct mk_list *list0, *list1, *head;
    struct mk_string_line *entry;

    struct mk_config_section *section;
    struct flb_out_test_config *config;

    section = mk_config_section_get(conf, "TEST");
    if (!section) {
        return NULL;
    }

    val0 = (int)mk_config_section_getval(section, "val0", MK_CONFIG_VAL_NUM);
    bool0 = (int)mk_config_section_getval(section, "bool0", MK_CONFIG_VAL_BOOL);
    str0 = mk_config_section_getval(section, "str0", MK_CONFIG_VAL_STR);
    list0 = mk_config_section_getval(section, "list0", MK_CONFIG_VAL_LIST);

    printf("val0 => %d, bool0 => %d, str0 => %s\n", val0, bool0, str0);
    printf("List: ");
    if (list0 != NULL) {
        mk_list_foreach(head, list0) {
            entry = mk_list_entry(head, struct mk_string_line, _head);
            printf("'%s' ", (char *)entry->val);
        }
        printf("\n");
    }

    /* doesn't exists */
    val1 = (int)mk_config_section_getval(section, "val1", MK_CONFIG_VAL_NUM);
    bool1 = (int)mk_config_section_getval(section, "bool1", MK_CONFIG_VAL_BOOL);
    str1 = mk_config_section_getval(section, "str1", MK_CONFIG_VAL_STR);
    list1 = mk_config_section_getval(section, "list1", MK_CONFIG_VAL_LIST);

    printf("val1 => %d, bool1 => %d, str1 => %s\n", val1, bool1, str1);

    if (list1 != NULL) {
        printf("List: ");
        mk_list_foreach(head, list1) {
            entry = mk_list_entry(head, struct mk_string_line, _head);
            printf("'%s' ", entry->val);
        }
        printf("\n");
    }

    config = calloc(1, sizeof(struct flb_out_test_config));
    config->val0        = val0;
    config->bool0       = bool0;
    config->str0        = str0;
    config->list0       = list0;

    return config;
}
