# File to be included in your CMakeLists.txt file through include() statement, not designed to be
# used as a subdir

set(pi-ulidcore_SRCS ${PI_ULID_DIR}/pi-ulid.c)

add_library(pi-ulid-core EXCLUDE_FROM_ALL ${pi-ulidcore_SRCS})
target_compile_options(pi-ulid-core PRIVATE ${PI_ULID_CFLAGS})
target_include_directories(pi-ulid-core ${PI_ULID_DIR}/include)
