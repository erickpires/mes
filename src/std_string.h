#ifndef STD_STRING_H
#define STD_STRING_H 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

#include "std_types.h"

typedef struct {
    const char* begin;
    usize len;
} string_slice;

string_slice make_string_slice(const char* str);
string_slice make_string_slice_with_len(const char* str, usize len);
char* string_slice_to_copy_string(string_slice slice);
bool string_slice_equals(string_slice a, string_slice b);
bool string_slice_equals_icase(string_slice a, string_slice b);


string_slice string_slice_concatenate(char* storage, ...);
string_slice trim_string_slice(string_slice slice);
// TODO(erick): We probably want a find_char_from_left.
// Also, it would be nice to implement a right-to-left KMP
// to be able to search a string from right-to-left.
string_slice find_char_from_left(string_slice haystack, char needle);
string_slice find_char_from_right(string_slice haystack, char needle);
bool advance_string(string_slice* slice, usize amount);
string_slice copy_and_advance_string(string_slice slice, usize amount);
string_slice uint_to_string(uint value);

#endif
