cmake_minimum_required(VERSION 4.1.1)
project(jhook)

SET (CMAKE_C_COMPILER "/usr/bin/clang")
set(JAVA_INCLUDE_PATH /usr/lib/jvm/java-25-openjdk/include/)
set(SOURCE_FILES src/library/jhook.c src/library/jhook.h)

find_package(JNI REQUIRED)
include_directories(${JNI_INCLUDE_DIRS})
add_compile_options(-Wall -ggdb -Wextra -Wpedantic -Werror -Wno-missing-field-initializers -Wno-strict-prototypes -O0)
add_compile_options(-fPIC)


add_library(jhook STATIC ${SOURCE_FILES})
add_library(jhook_example SHARED src/example/main.c)
target_include_directories(jhook PUBLIC src/library)
target_link_libraries(jhook_example jhook -Lsrc/library/jhook.h)
