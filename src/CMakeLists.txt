add_executable(vtun base64.c client.c codec.c conf.c ioctl.c server.c sig.c vtun.c xfer.c)
cmake_minimum_required(VERSION 3.15.5)
include_directories(../inc/)
project(vtun)
set_target_properties(vtun PROPERTIES RUNTIME_OUTPUT_DIRECTORY ../bin/)
