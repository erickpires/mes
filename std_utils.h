#ifndef STD_UTILS_H
#define STD_UTILS_H 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

#include <stdint.h>
#include <stdbool.h>

typedef intptr_t isize;
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef size_t   usize;
typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef unsigned int uint;

typedef struct {
    const char* begin;
    usize len;
} string_slice;

typedef struct {
    string_slice file_stem;
    string_slice file_extension;
    string_slice parent;
} fs_path;

// TODO(erick): A way to disable assertions in Release.
#define assert(expr) ((expr) ? \
                      (void) 0 : \
                      __fail_assert(#expr, __FILE__, __LINE__, __func__))

#define sizeof_array(array) (sizeof(array) / sizeof(array[0]))

#ifdef _WIN32
#define fs_separator '\\'
#else
#define fs_separator '/'
#endif

// TODO(erick): There sould be a way of passing a file
// to log warnings and errors. Or the concept of
// a logger.
void __fail_assert(const char* expr, const char* file_name, const uint32 line,
                   const char* function);
void warn(const char* format, ...);
void error(const char* format, ...);

char* get_current_directory_name();
char* read_all_data(int file_descriptor, usize* len_read);
void write_all_data(char* data, usize len, int dest);

string_slice make_string_slice(const char* str);
string_slice make_string_slice_with_len(const char* str, usize len);
char* string_slice_to_copy_string(string_slice slice);
bool string_slice_equals(string_slice a, string_slice b);

string_slice string_slice_concatenate(char* storage, ...);
string_slice trim_string_slice(string_slice slice);
// TODO(erick): We probably want a find_char_from_left.
// Also, it would be nice to implement a right-to-left KMP
// to be able to search a string from right-to-left.
string_slice find_char_from_left(string_slice haystack, char needle);
string_slice find_char_from_right(string_slice haystack, char needle);
bool advance_string(string_slice* slice, usize amount);
string_slice copy_and_advance_string(string_slice slice, usize amount);

fs_path make_fs_path_from_str(string_slice path_str);
fs_path make_fs_path_from_dir_and_file(string_slice dir, string_slice file);
char* fs_path_to_string(const fs_path* path);
fs_path fs_path_go_up(fs_path* path);
void fs_remove_extension(fs_path* path);
fs_path fs_path_copy(fs_path* to_copy);



// NOTE(erick): It would be nice to have 'fs_path_append'
// but it either requires memory allocation (to be sure
// that parent is continuous) or to represent parent
// with a linked list (which also requires allocation).
// That said, this behaviour is already accomplished
// by calling 'fs_path_to_string' and 'make_fs_from_dir_and_file'
#endif
