#include <strings.h>

#include "std_types.h"
#include "std_string.h"

string_slice make_string_slice(const char* str) {
    usize len = strlen(str);
    return make_string_slice_with_len(str, len);
}

string_slice make_string_slice_with_len(const char* str, usize len){
    string_slice result;

    result.begin = str;
    result.len = len;

    return result;
}

char* string_slice_to_copy_string(string_slice slice) {
    char* result = (char*) malloc(slice.len + 1);
    if(!result) { return NULL; }

    memcpy(result, slice.begin, slice.len);
    result[slice.len] = '\0';

    return result;
}

string_slice string_slice_concatenate(char* storage, ...) {
    va_list var_args;
    va_start(var_args, storage);

    string_slice result;
    result.begin = storage;
    result.len = 0;

    string_slice slice;
    do {
        slice = va_arg(var_args, string_slice);

        memcpy(storage, slice.begin, slice.len);

        storage += slice.len;
        result.len += slice.len;

        // TODO(erick): This halting criteria will
        // work for now, but we need something better.
    } while(slice.len != 0);

    // Null terminating the string.
    *storage = '\0';

    va_end(var_args);
    return result;
}

bool string_slice_equals(string_slice a, string_slice b) {
    if(a.len != b.len) { return false; }

    bool result = memcmp(a.begin, b.begin, a.len) == 0;
    return result;
}

bool string_slice_equals_icase(string_slice a, string_slice b) {
    if(a.len != b.len) { return false; }

    bool result = strncasecmp(a.begin, b.begin, a.len) == 0;
    return result;
}

string_slice trim_string_slice(string_slice slice) {
    string_slice result = slice;

    while(result.len &&
          (result.begin[result.len - 1] == ' ' ||
           result.begin[result.len - 1] == '\t')) {
        result.len--;
    }

    while(result.len &&
          (*result.begin == ' ' ||
           *result.begin == '\t')) {
        result.begin++;
        result.len--;
    }

    return result;
}

string_slice find_char_from_left(string_slice haystack, char needle) {
    string_slice result = {haystack.begin, haystack.len};

    for(int i = 0; i < haystack.len; i++) {
        if(haystack.begin[i] == needle) {
            result.len = i + 1;
            break;
        }
    }

    return result;
}

string_slice find_char_from_right(string_slice haystack, char needle) {
    const char* searching = haystack.begin + haystack.len - 1;

    string_slice result = {0};

    usize bytes_read = 0;
    //NOTE(erick): This can be faster with a sentinel.
    while(searching >= haystack.begin) {
        if(*searching == needle) {
            result.begin = searching;
            result.len = bytes_read + 1;
            break;
        }

        bytes_read++;
        searching--;
    }

    return result;
}

bool advance_string(string_slice* slice, usize amount) {
    if(slice->len < amount) {
        slice->begin += slice->len;
        slice->len = 0;
    } else {
        slice->begin += amount;
        slice->len -= amount;
    }

    return slice->len > 0;
}

string_slice copy_and_advance_string(string_slice slice, usize amount) {
    string_slice result = slice;
    advance_string(&result, amount);

    return result;
}

string_slice uint_to_string(uint value) {
    char* mem = (char*) malloc(8);
    sprintf(mem, "%04xh", value);
    return make_string_slice(mem);
}