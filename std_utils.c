#include "std_utils.h"

void __fail_assert(const char* expr, const char* file_name, const uint32 line,
                   const char* function) {
    error("%s:%d: In function '%s': Assertion Failed! (%s).",
          file_name, line, function, expr);
    exit(1);
}

void warn(const char* format, ...) {
    va_list var_args;
    va_start(var_args, format);

    fprintf(stderr, "WARNING: ");
    vfprintf(stderr, format, var_args);
    fprintf(stderr, "\n");

    va_end(var_args);
}

void error(const char* format, ...) {
    va_list var_args;
    va_start(var_args, format);

    fprintf(stderr, "ERROR: ");
    vfprintf(stderr, format, var_args);
    fprintf(stderr, "\n");

    va_end(var_args);
    exit(1);
}


char* get_current_directory_name() {
    // TODO(erick): Do something different for Windows

    usize len = 128;
    char* result = (char*) malloc(len);
    if(!result) { error("Out of memory!"); }

    while(!getcwd(result, len)) {
        len *= 2;
        result = (char*) realloc(result, len);
        if(!result) { error("Out of memory!"); }
    }

    return result;
}

char* read_all_data(int file_descriptor, usize* len_read) {
    usize capacity = 1024;
    usize total_read = 0;
    char* result = (char*) malloc(capacity);
    if(!result) { error("Out of memory!"); }

    isize bytes_read;
    while(true) {
        char* write_ptr = result + total_read;
        usize len_to_read = capacity - total_read;

        bytes_read = read(file_descriptor, write_ptr, len_to_read);
        if(bytes_read < 0) { error("Error while reading file."); }

        total_read += bytes_read;
        if(bytes_read < len_to_read) { break; }

        // NOTE(erick): We were able to fill the buffer.
        // It is possible that there is some data remaining
        // in the 'file'. So we reallocate and try to read
        // one more time.
        capacity *= 2;

        result = (char*) realloc(result, capacity);
        if(!result) { error("Out of memory!"); }
    }

    if(len_read) {
        *len_read = total_read;
    }

    return result;
}

void write_all_data(char* data, usize len, int dest) {
    do {
        isize written = write(dest, data, len);

        if(written < 0) { error("Error while writing to file."); }

        len -= written;
        data += written;
    } while(len > 0);
}

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

static void fs_file_stem_and_extension(string_slice* stem,
                                       string_slice* extension,
                                       string_slice  file) {
    // README(erick): Should we really support empty files?
    if(file.len == 0) { return; }

    string_slice last_dot = find_char_from_right(file, '.');

    // We don't have an extension.
    if(last_dot.len == 0) {
        *stem = file;
        // File starts with dot. We still don't have an extension
    } else if(last_dot.begin == file.begin) {
        *stem = file;
    } else {
        *extension = copy_and_advance_string(last_dot, 1);
        stem->begin = file.begin;
        stem->len   = file.len - last_dot.len;
    }
}

// TODO(erick): What happens when a string ending in '/' is passed here?
// Right now it is considered a path with no file and the whole string is
// the parent. This doesn't seems right, but what happens when the root
// (a.k.a. "/") is passed? Or when "/bin" is passed? Is the parent empty
// in these cases?
fs_path make_fs_path_from_str(string_slice path_str) {
    assert(path_str.len > 0);

    fs_path result = {};
    // TODO(erick): Is this all right? What if the user
    // wants to pass a separator? What separator does the Windows API use?
    string_slice last_separator = find_char_from_right(path_str, fs_separator);

    string_slice file;
    // If this is true the path corresponds to a single file
    // and we don't have a parent.
    if(last_separator.begin == path_str.begin) {
        file = path_str;
    } else {
        file = copy_and_advance_string(last_separator, 1);
        result.parent.begin = path_str.begin;
        // NOTE(erick): parent doesn't contain the trailing fs_separator
        result.parent.len = path_str.len - file.len;

    }

    fs_file_stem_and_extension(&result.file_stem, &result.file_extension, file);
    return result;
}

fs_path make_fs_path_from_dir_and_file(string_slice dir, string_slice file) {
    // README(erick): clang doesn't like initialization with {0}.
    // I don't know whether msvc accepts it or not.
    fs_path result = {};

    // NOTE(erick): parent doesn't contain the trailing fs_separator
    if(dir.begin[dir.len - 1] == fs_separator) {
        dir.len--;
    }

    result.parent = dir;
    fs_file_stem_and_extension(&result.file_stem, &result.file_extension, file);

    return result;
}

// TODO(erick): We need to think about what happens when path points to the root.
char* fs_path_to_string(const fs_path* path) {
    // TODO(erick): We probably can't assert this due to the special case
    // of pointing to the root.
    assert(path->file_stem.len > 0);

    usize len =
        path->parent.len + 1 + // fs_separator
        path->file_stem.len + 1 + // '.'
        path->file_extension.len + 1; // '\0'

    char* result = (char*) malloc(len);
    if(!result) {
        warn("Unable to allocate memory!");
        return NULL;
    }

    char* write_ptr = result;
    if(path->parent.len > 0) {
        memcpy(write_ptr, path->parent.begin, path->parent.len);

        write_ptr += path->parent.len;
    }

    *write_ptr = '/';

    write_ptr += 1;
    memcpy(write_ptr, path->file_stem.begin, path->file_stem.len);

    write_ptr += path->file_stem.len;
    if(path->file_extension.len > 0) {
        *write_ptr = '.';

        write_ptr += 1;
        memcpy(write_ptr, path->file_extension.begin, path->file_extension.len);

        write_ptr += path->file_extension.len;
    }

    *write_ptr = '\0';
    return result;
}

fs_path fs_path_go_up(fs_path* path) {
    string_slice last_separator = find_char_from_right(path->parent, fs_separator);
    // We can't go up!!!!
    if(last_separator.begin == path->parent.begin) {
        return *path;
    }

    fs_path result;

    // NOTE(erick): We can only go up to a directory. So the
    // file extension should be empty.
    result.file_stem = copy_and_advance_string(last_separator, 1);
    result.file_extension.len = 0;

    result.parent.begin = path->parent.begin;
    result.parent.len = path->parent.len - last_separator.len;

    return result;
}

void fs_remove_extension(fs_path* path) {
    path->file_extension.len = 0;
}

fs_path fs_path_copy(fs_path* to_copy) {
    fs_path result = *to_copy;
    return result;
}
