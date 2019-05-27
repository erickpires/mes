#ifndef MES_UTILS_H
#define MES_UTILS_H 1

#include "mes.h"
#include "std_types.h"

// TODO(erick): A way to disable assertions in Release.
#define assert(expr) ((expr) ? \
                      (void) 0 : \
                      __fail_assert(#expr, __FILE__, __LINE__, __func__))

void __fail_assert(const char* expr, const char* file_name, const uint32 line,
                   const char* function);

void warn(const char* format, ...);
void error(const char* format, ...);
void fail(Line* line, uint exit_code, const char* fmt, ...);

char* read_entire_file(FILE* file);

bool is_number(char c);
bool is_alpha(char c);

bool is_number_token(Token* token);
uint16 parse_decimal(string_slice slice);
uint16 parse_hex(string_slice slice);
uint16 parse_binary(string_slice slice);
bool parse_value(Token* operand, uint16* result);

bool parse_instruction(Token* instruction, uint16* result);

#endif