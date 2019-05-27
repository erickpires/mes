#ifndef MES_H
#define MES_H 1

#include "std_string.h"

//
// Error Codes
//
#define ARGS_PARSING_ERROR 1
#define FILE_ERROR         2
#define INCLUDE_ERROR      3
#define TOKENIZE_ERROR     4
#define CLASSIFY_ERROR     5
#define EQUALITIES_ERROR   6
#define UNKNOWNS_ERROR     7
#define MACRO_ERROR        8
#define ADDRESS_ERROR      9
#define LABEL_ERROR        10
#define RESOLVE_EXPR_ERROR 11
#define MACHINE_CODE_ERROR 12

//
// Constants
//
#define MEMORY_SIZE (64 * 1024)
#define MAX_OPERAND 0x4000


//
// Data types
//
typedef enum {
    MACRO_DEF,
    MACRO_END,
    MACRO_INVOKE,
    MACRO_LOCALS,
    CODE,
    ALLOC,
    EQUALITY,
    ORIGIN,
    BLANK,
    LABEL,
    // NOTE(erick): Unknown can be a Macro Invoke or a 'equ' replacement.
    //  This can only be decided after the 'equ' application.
    UNKNOWN,
} LineType;

typedef struct __Token {
    struct __Token* next_token;
    string_slice slice;
} Token;

typedef struct __Line {
    struct __Line* next_line;

    Token* tokens;
    union {
        string_slice comment;
        string_slice line_content;
    };

    LineType type;

    bool occupies_space;
    bool has_label;
    bool belongs_to_macro_def;

    string_slice file_name;
    uint line_number;

    uint address;
    uint16 memory_data;
} Line;

typedef struct {
    string_slice key;
    Token* data;
} StringTokenPair;

typedef struct {
    string_slice key;
    uint data;
} StringUintPair;

typedef struct {
    StringTokenPair* data;
    usize count;
} MacroReplaceDict;

typedef struct {
    string_slice name;
    Token* params;
    Token* local_labels;

    Line* begin;
    Line* end;

    uint params_count;
    uint local_labels_count;

    uint n_macro_instantiation;
} Macro;

typedef struct {
    uint old_origin;
    uint new_origin;
} OriginChange;


void output_lst_file(char*, Line*);
void output_hex_file(char*);
void output_binary_file(char*);
void output_ces_hex_file(char*);
void output_separated_binary_files(char*);

#endif