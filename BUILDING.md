# Building sources

Building system for this project is based on CMake's `include()` statements and some variable definitions to work.

This project doesn't support adding this project as a subdir through `add_subdirectory()` statement, the main reason being letting the user to decide which ports/modules to include in his project.

For instance if you wanted to include the core and also the "unix" port, you would add something like this to your `CMakeLists.txt` file:
```cmake
set(PI_ULID_DIR "/path/to/ulid")

include("${PI_ULID_DIR}/include/pi-ulid/ports/unix/include.cmake") # this exports pi-ulid-port-unix
include("${PI_ULID_DIR}/include.cmake") # this exports pi-ulid-core

add_executable(main main.c) # your main application
target_link_libraries(main pi-ulid-core pi-ulid-port-unix) # your main application now is linked to ulid and unix port
```

## Variables to be defined
* #### `PI_ULID_DIR`
  Absolute path to root directory where pi-ulid is.

* #### `PI_ULID_CFLAGS` (optional)
  Compiler flags to pass to pi-ulid during compilation.
