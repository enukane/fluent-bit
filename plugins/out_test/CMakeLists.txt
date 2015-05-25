set(src
  test.c
  test_config.c)

FLB_PLUGIN(out_test "${src}" "mk_config")
