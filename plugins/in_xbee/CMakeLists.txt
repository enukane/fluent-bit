# Build dependency
add_subdirectory(lib/libxbee-v3)
include_directories(lib/libxbee-v3)

set(src
  in_xbee.c)

FLB_PLUGIN(in_xbee "${src}" "xbee")
