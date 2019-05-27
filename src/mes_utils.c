#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

#include "mes_utils.h"

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

char* read_entire_file(FILE* file) {
    usize capacity = 4;
    usize used = 0;

    char* result = (char*) malloc(capacity);
    while(true) {
        int read = fgetc(file);
        if(read == EOF) {
            result[used] = '\0';
            break;
        }

        if(used >= capacity - 2) {
            capacity *= 2;
            result = (char*) realloc(result, capacity);
        }

        result[used] = (char) read;
        used++;
    }

    return result;
}

void fail(Line* line, uint exit_code, const char* fmt, ...) {
    va_list var_args;
    va_start(var_args, fmt);

    fprintf(stderr, "\n\n");

    // TODO(erick): Add some colors
    if(line) {
        fprintf(stderr, "[ERROR] %.*s:%d: ",
                (int) line->file_name.len,
                line->file_name.begin,
                line->line_number);
    } else {
        fprintf(stderr, "[ERROR]: ");
    }

    vfprintf(stderr, fmt, var_args);
    fprintf(stderr, "\n");

    va_end(var_args);
    exit(exit_code);
}

bool is_number(char c) {
    return (c >= '0' && c <= '9');
}

bool is_alpha(char c) {
    return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
}

bool is_number_token(Token* token) {
    string_slice slice = token->slice;

    for(int i = 0; i < slice.len - 1; i++) {
        if((slice.begin[i] < '0' || slice.begin[i] > '9') &&
           (slice.begin[i] < 'a' || slice.begin[i] > 'f') &&
           (slice.begin[i] < 'A' || slice.begin[i] > 'F')) {
            return false;
        }
    }

    char last_char = slice.begin[slice.len - 1];
    if(last_char >= '0' && last_char <= '9') {
        return true;
    }

    if(last_char == 'h' || last_char == 'b') {
        return true;
    }

    return false;
}

uint16 parse_decimal(string_slice slice) {
    char buffer[16];
    // NOTE(erick): slice.begin is probably not null-terminated;
    snprintf(buffer, 16, "%.*s", (int) slice.len, slice.begin);

    return atoi(buffer);
}

uint16 parse_hex(string_slice slice) {
    uint16 result = 0;
    for(int i = 0; i < slice.len; i++) {
        uint8 current_digit;
        if(slice.begin[i] >= '0' && slice.begin[i] <= '9') {
            current_digit = slice.begin[i] - '0';
        } else if(slice.begin[i] >= 'a' && slice.begin[i] <= 'f') {
            current_digit = 10 + slice.begin[i] - 'a';
        } else {
            current_digit = 10 + slice.begin[i] - 'A';
        }

        result = 16 * result + current_digit;
    }

    return result;
}

uint16 parse_binary(string_slice slice) {
    uint16 result = 0;
    for(int i = 0; i < slice.len; i++) {
        uint current_digit = 1;
        if(slice.begin[i] == '0') { current_digit = 0; }

        result = 2 * result + current_digit;
    }

    return result;
}

bool parse_value(Token* operand, uint16* result) {
    if(!is_number_token(operand)) {
        return false;
    }

    string_slice operand_slice = operand->slice;
    bool is_hex    = operand_slice.begin[operand_slice.len - 1] == 'h';
    bool is_binary = operand_slice.begin[operand_slice.len - 1] == 'b';

    if(is_hex) {
        operand_slice.len -= 1;
        *result = parse_hex(operand_slice);
    } else if(is_binary) {
        operand_slice.len -= 1;
        *result = parse_binary(operand_slice);
    } else {
        *result = parse_decimal(operand_slice);
    }

    return true;
}

bool parse_instruction(Token* instruction, uint16* result) {
    string_slice dw_slice  = make_string_slice("DW");
    string_slice lm_slice  = make_string_slice("LM");
    string_slice em_slice  = make_string_slice("EM");
    string_slice sb_slice  = make_string_slice("SB");
    string_slice dnp_slice = make_string_slice("DNP");

    if(string_slice_equals_icase(instruction->slice, dw_slice)) {
        *result = 0;
        return true;
    }

    if(string_slice_equals_icase(instruction->slice, lm_slice)) {
        *result = 0x0000;
        return true;
    }

    if(string_slice_equals_icase(instruction->slice, em_slice)) {
        *result = 0x4000;
        return true;
    }

    if(string_slice_equals_icase(instruction->slice, sb_slice)) {
        *result = 0x8000;
        return true;
    }

    if(string_slice_equals_icase(instruction->slice, dnp_slice)) {
        *result = 0xC000;
        return true;
    }

    return false;
}
