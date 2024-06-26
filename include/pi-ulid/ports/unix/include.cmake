# File to be included in your CMakeLists.txt file through include() statement, not designed to be
# used as a subdir

set(pi-ulid-port-unix_SRCS ${PI_ULID_DIR}/include/pi-ulid/ports/unix/funcs.c)

add_library(pi-ulid-port-unix EXCLUDE_FROM_ALL ${pi-ulid-port-unix_SRCS})
target_include_directories(pi-ulid-port-unix PRIVATE ${PI_ULID_DIR}/include)
target_compile_options(pi-ulid-port-unix PRIVATE ${PI_ULID_CFLAGS})
