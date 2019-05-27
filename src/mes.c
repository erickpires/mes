#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <alloca.h>

#include "mes.h"
#include "mes_utils.h"
#include "std_string.h"

typedef intptr_t isize;
typedef int8_t   int8;
typedef int16_t  int16;
typedef int32_t  int32;
typedef int64_t  int64;

typedef size_t       usize;
typedef uint8_t      uint8;
typedef uint16_t     uint16;
typedef uint32_t     uint32;
typedef uint64_t     uint64;
typedef unsigned int uint;

// TODO(erick):
//    Expressions resolution
//    Do something with line numbers of lines the came from a macro expansion (???)
//    Allocate variables in RAM
//    Better print labels on the .lst file
//    Help message
//    Header file
//    Intel hex file

//
// Globals
//

static bool output_intel_hex;
static bool output_ces_hex;

// NOTE(erick): Two binary files, one for lower bytes, one for higher bytes.
static bool output_separated_binaries;

// TODO(erick): Machine code should not be a global.
uint16 machine_code[MEMORY_SIZE];

static StringTokenPair* equ_dict = NULL;
static usize equ_dict_capacity = 0;
static usize equ_dict_count = 0;

static Macro* macro_dict = NULL;
static usize macro_dict_capacity = 0;
static usize macro_dict_count = 0;

static StringUintPair* label_dict = NULL;
static usize label_dict_capacity = 0;
static usize label_dict_count = 0;

// TODO(erick): These variables should not be globals.
OriginChange org_change_list[1024] = {};
usize org_change_list_count = 0;
uint last_address;

void print_help_and_exit(int status) {
    printf("HELP\n");
    exit(status);
}


void add_to_equ_dict(string_slice key, Token* data) {
    if(equ_dict_capacity == 0) {
        equ_dict_capacity = 16;
        equ_dict = (StringTokenPair*)
            malloc(equ_dict_capacity * sizeof(StringTokenPair));
    }

    if(equ_dict_count >= equ_dict_capacity) {
        equ_dict_capacity *= 2;
        equ_dict = (StringTokenPair*)
            realloc(equ_dict, equ_dict_capacity * sizeof(StringTokenPair));
    }

    StringTokenPair element = {key, data};
    equ_dict[equ_dict_count] = element;
    equ_dict_count++;
}

Token* search_equ_dict(string_slice to_search) {
    Token* result = NULL;

    for(int i = 0; i < equ_dict_count; i++) {
        if(string_slice_equals_icase(to_search, equ_dict[i].key)) {
            result = equ_dict[i].data;
            break;
        }
    }

    return result;
}

bool count_comma_separated_tokens(Token* token, uint* result) {
    string_slice comma_token = make_string_slice(",");

    if(!token) {
        *result = 0;
        return true;
    }

    uint count = 0;
    while(token) {
        count++;

        if(token->next_token && string_slice_equals(token->next_token->slice,
                                                  comma_token)) {
            Token* next_token = token->next_token->next_token;
            if(!next_token) { return false; }
            if(string_slice_equals(next_token->slice, comma_token)) { return false; }

            token = next_token;
        } else { break; }
    }

    *result = count;
    return true;
}

void add_to_macro_dict(Line* dec_line, string_slice name,
                       Token* params, Token* locals,
                       Line* begin, Line* end) {
    uint params_count;
    uint locals_count;
    if(!count_comma_separated_tokens(params, &params_count)) {
        fail(dec_line, MACRO_ERROR, "Invalid parameter list.");
    }

    if(!count_comma_separated_tokens(locals, &locals_count)) {
        fail(dec_line, MACRO_ERROR, "Invalid LOCAL list.");
    }

    Macro macro = {name, params, locals, begin, end, params_count, locals_count, 0};

    if(!begin || !end) {
        fail(dec_line, MACRO_ERROR, "Invalid Macro definition: %.*s\n",
             (int) name.len, name.begin);
    }

    if(macro_dict_capacity == 0) {
        macro_dict_capacity = 16;
        macro_dict = (Macro*)
            malloc(macro_dict_capacity * sizeof(Macro));
    }

    if(macro_dict_count >= macro_dict_capacity) {
        macro_dict_capacity *= 2;
        macro_dict = (Macro*)
            realloc(macro_dict, macro_dict_capacity * sizeof(Macro));
    }

    macro_dict[macro_dict_count] = macro;
    macro_dict_count++;
}

Macro* search_macro_dict(string_slice to_search) {
    Macro* result = NULL;

    for(int i = 0; i < macro_dict_count; i++) {
        if(string_slice_equals_icase(to_search, macro_dict[i].name)) {
            result = macro_dict + i;
            break;
        }
    }

    return result;
}

void add_to_label_dict(string_slice key, uint data) {
    if(label_dict_capacity == 0) {
        label_dict_capacity = 16;
        label_dict = (StringUintPair*)
            malloc(label_dict_capacity * sizeof(StringUintPair));
    }

    if(label_dict_count >= label_dict_capacity) {
        label_dict_capacity *= 2;
        label_dict = (StringUintPair*)
            realloc(label_dict, label_dict_capacity * sizeof(StringUintPair));
    }

    StringUintPair element = {key, data};
    label_dict[label_dict_count] = element;
    label_dict_count++;
}

bool search_label_dict(string_slice to_search, uint* result) {
    for(int i = 0; i < label_dict_count; i++) {
        if(string_slice_equals_icase(to_search, label_dict[i].key)) {
            *result = label_dict[i].data;
            return true;
        }
    }

    return false;
}

Line* divide_in_lines(char* data, string_slice filename) {
    Line* result = (Line*) malloc(sizeof(Line));
    Line* current_line = result;

    uint current_line_number = 1;
    if(*data == '\n') {
        current_line_number = 2;
    }

    while(true) {
        if(*data == '\n') { data++; }

        char* start = data;
        while(*data != '\n' && *data != '\0') {
            data++;
        }

        char* end = data;

        string_slice slice = make_string_slice_with_len(start, end - start);

        current_line->line_content = slice;
        current_line->line_number  = current_line_number;
        current_line->file_name = filename;

        if(*data == '\0') { break; }

        current_line->next_line = (Line*) malloc(sizeof(Line));
        current_line = current_line->next_line;
        current_line_number++;
    }

    return result;
}

string_slice get_number(string_slice slice) {
    string_slice result;
    result.begin = slice.begin;
    result.len = 0;

    const char* str_ptr = slice.begin;
    while(is_number(*str_ptr) ||
          (*str_ptr >= 'a' && *str_ptr <= 'f') ||
          (*str_ptr >= 'A' && *str_ptr <= 'F') ||
          *str_ptr == 'h' ||
          *str_ptr == 'b') {

        result.len++;
        str_ptr++;
    }

    return result;
}

string_slice get_ident(string_slice slice) {
    string_slice result;
    result.begin = slice.begin;
    result.len = 0;

    const char* str_ptr = slice.begin;
    while(is_alpha(*str_ptr) || is_number(*str_ptr)) {
        result.len++;
        str_ptr++;
    }

    return result;
}

Token* new_token(string_slice slice) {
    Token* result = (Token*) malloc(sizeof(Token));

    result->slice = slice;
    result->next_token = NULL;

    return result;
}

void tokenize_lines(Line* lines) {
    Line* current_line = lines;
    while(current_line) {
        string_slice full_line = current_line->line_content;

        string_slice code = find_char_from_left(full_line, ';');
        if(code.len > 0 && code.begin[code.len - 1] == ';') {
            code.len--;
        }

        current_line->comment.begin = full_line.begin + code.len;
        current_line->comment.len = full_line.len - code.len;

        code = trim_string_slice(code);

        if(code.len == 0) {
            current_line->tokens = NULL;
            goto LOOP_END;
        }

        current_line->tokens = (Token*) malloc(sizeof(Token));
        Token* current_token = current_line->tokens;

        while(true) {
            string_slice token;

            if(is_number(*code.begin)) {
                token = get_number(code);

            } else if(is_alpha(*code.begin)) {
                token = get_ident(code);

            } else if(*code.begin == ':' ||
                      *code.begin == '$' ||
                      *code.begin == ',' ||
                      *code.begin == '(' ||
                      *code.begin == ')' ||
                      *code.begin == '+' ||
                      *code.begin == '-' ||
                      *code.begin == '*' ||
                      *code.begin == '/') {
                token.begin = code.begin;
                token.len = 1;
            } else {
                fail(current_line, TOKENIZE_ERROR, "Invalid character (%c) [0x%x]",
                     *code.begin, *code.begin);
            }

            code.begin += token.len;
            code.len -= token.len;
            code = trim_string_slice(code);

            current_token->slice = token;

            if(code.len == 0) { break; }

            current_token->next_token = (Token*) malloc(sizeof(Token));
            current_token = current_token->next_token;
        }

    LOOP_END:
        current_line = current_line->next_line;
    }
}

bool is_alloc(Line* line) {
    string_slice dw_slice  = make_string_slice("DW");

    Token* first_token = line->tokens;
    Token* maybe_dw_token;
    if(line->has_label) {
        Token* third_token = first_token->next_token->next_token;
        maybe_dw_token = third_token;
    } else {
        maybe_dw_token = first_token;
    }

    if(!maybe_dw_token) { return false; }

    return (string_slice_equals_icase(maybe_dw_token->slice, dw_slice));
}

bool is_code(Line* line) {
    string_slice lm_slice  = make_string_slice("LM");
    string_slice em_slice  = make_string_slice("EM");
    string_slice sb_slice  = make_string_slice("SB");
    string_slice dnp_slice = make_string_slice("DNP");

    Token* maybe_code_token;
    Token* first_token = line->tokens;

    if(line->has_label) {
        maybe_code_token = first_token->next_token->next_token;
    } else {
        maybe_code_token = first_token;
    }

    if(!maybe_code_token) { return false; }

    if(string_slice_equals_icase(maybe_code_token->slice, lm_slice) ||
       string_slice_equals_icase(maybe_code_token->slice, em_slice) ||
       string_slice_equals_icase(maybe_code_token->slice, sb_slice) ||
       string_slice_equals_icase(maybe_code_token->slice, dnp_slice)) {

        return true;
    } else {
        return false;
    }
}

void classify_lines(Line* lines) {
    Line* current_line = lines;

    string_slice org_slice    = make_string_slice("ORG");
    string_slice endm_slice   = make_string_slice("ENDM");
    string_slice local_slice   = make_string_slice("LOCAL");
    string_slice macro_slice  = make_string_slice("MACRO");
    string_slice column_slice = make_string_slice(":");

    string_slice equ_slice = make_string_slice("equ");

    while(current_line) {

        Token* first_token = current_line->tokens;

        if(first_token == NULL || first_token->slice.len == 0) {
            current_line->type = BLANK;
            current_line->has_label = false;
            current_line->occupies_space = false;

            goto LOOP_END;
        }

        if(string_slice_equals_icase(first_token->slice, org_slice)) {
            current_line->type = ORIGIN;
            current_line->has_label = false;
            current_line->occupies_space = false;

            goto LOOP_END;
        } else if(string_slice_equals_icase(first_token->slice, endm_slice)) {
            current_line->type = MACRO_END;
            current_line->has_label = false;
            current_line->occupies_space = false;

            goto LOOP_END;
        } else if(string_slice_equals_icase(first_token->slice, local_slice)) {
            current_line->type = MACRO_LOCALS;
            current_line->has_label = false;
            current_line->occupies_space = false;

            goto LOOP_END;
        }

        Token* second_token = first_token->next_token;

        if(second_token == NULL) {
            current_line->type = UNKNOWN;

            goto LOOP_END;
        }

        if(string_slice_equals(second_token->slice, macro_slice)) {
            current_line->type = MACRO_DEF;
            current_line->has_label = false;
            current_line->occupies_space = false;

            goto LOOP_END;
        } else if(string_slice_equals(second_token->slice, equ_slice)) {
            current_line->type = EQUALITY;
            current_line->has_label = false;
            current_line->occupies_space = false;

            goto LOOP_END;
        }

        if(string_slice_equals(second_token->slice, column_slice)) {
            current_line->has_label = true;
        } else {
            current_line->has_label = false;
        }

        if(is_alloc(current_line)) {
            current_line->type = ALLOC;
            current_line->occupies_space = true;
        } else if(is_code(current_line)) {
            current_line->type = CODE;
            current_line->occupies_space = true;
        }else if(current_line->has_label && second_token->next_token == NULL) {
            current_line->type = LABEL;
        } else {
            current_line->type = UNKNOWN;
        }

    LOOP_END:
        current_line = current_line->next_line;
    }
}

Token* replace_token(Token* to_replace, Token* replacement) {
    Token* tail = to_replace->next_token;

    to_replace->slice = replacement->slice;

    while(replacement->next_token) {
        Token* new_token = (Token*) malloc(sizeof(Token));
        to_replace->next_token = new_token;

        replacement = replacement->next_token;
        to_replace = to_replace->next_token;
        to_replace->slice = replacement->slice;
    }

    to_replace->next_token = tail;
    return to_replace;
}

void apply_equalities(Line* lines) {
    Line* current_line = lines;

    while(current_line) {
        if(current_line->type == EQUALITY) {
            Token* first_token  = current_line->tokens;
            Token* second_token = first_token->next_token;
            Token* third_token  = second_token->next_token;

            if(!third_token) {
                fail(current_line, EQUALITIES_ERROR, "Invalid 'equ' (%.*s)",
                    (int) first_token->slice.len,
                     first_token->slice.begin);
            }

            if(search_equ_dict(first_token->slice) != NULL) {
                fail(current_line, EQUALITIES_ERROR,
                     "EQU (%.*s) has already been defined",
                     (int) first_token->slice.len,
                     first_token->slice.begin);
            }

            add_to_equ_dict(first_token->slice, third_token);
        }

        current_line = current_line->next_line;
    }

    current_line = lines;
    while(current_line) {
        if(current_line->type != EQUALITY) {
            Token* current_token = current_line->tokens;
            while(current_token) {
                Token* search = search_equ_dict(current_token->slice);
                if(search) {
                    replace_token(current_token, search);
                } else {
                    // NOTE(erick): This guarantees that nested qualities
                    // are going to be replaced.
                    current_token = current_token->next_token;
                }
            }
        }

        current_line = current_line->next_line;
    }

}

void resolve_unknowns(Line* lines) {
    Line* current_line = lines;

    while(current_line) {
        if(current_line->type == UNKNOWN) {
            if(is_code(current_line)) {
                current_line->type = CODE;
                current_line->occupies_space = true;
            } else {
                current_line->type = MACRO_INVOKE;
            }
        }

        current_line = current_line->next_line;
    }
}

Token* copy_or_replace_tokens(Token* to_copy, MacroReplaceDict replace_dict) {
    if(!to_copy) { return NULL;  }

    Token* result = (Token*) malloc(sizeof(Token));

    Token* current_token = result;
    while(true) {
        current_token->slice = to_copy->slice;
        for(int i = 0; i < replace_dict.count; i++) {
            if(string_slice_equals_icase(current_token->slice,
                                         replace_dict.data[i].key)) {
                current_token = replace_token(current_token, replace_dict.data[i].data);
                break;
            }
        }

        if(!to_copy->next_token) { break; }

        current_token->next_token = (Token*) malloc(sizeof(Token));
        current_token = current_token->next_token;
        to_copy = to_copy->next_token;
    }

    return result;
}

string_slice allocate_and_concatenate(string_slice a, string_slice b) {
    char* data = (char*) malloc(a.len + b.len);

    memcpy(data,         a.begin, a.len);
    memcpy(data + a.len, b.begin, b.len);

    string_slice result = { data, a.len + b.len };
    return result;
}

MacroReplaceDict build_macro_replace_dict(Line* current_line,
                                          Macro* macro, Token* args) {
    string_slice comma_slice = make_string_slice(",");
    MacroReplaceDict result = {0};

    result.data = (StringTokenPair*)
        malloc((macro->params_count + macro->local_labels_count) *
               sizeof(StringTokenPair));

    result.count = macro->params_count + macro->local_labels_count;


    StringTokenPair* locals = result.data;
    StringTokenPair* params = result.data + macro->local_labels_count;

    //
    // Filling LOCALs
    //

    char buffer[128];
    sprintf(buffer, "%03d_at_%.*s", macro->n_macro_instantiation,
            (int) macro->name.len, macro->name.begin);
    string_slice inst_number = make_string_slice(buffer);

    Token* current_local = macro->local_labels;
    for(int i = 0; i < macro->local_labels_count; i++) {
        string_slice label = allocate_and_concatenate(current_local->slice,
                                                      inst_number);

        locals[i].key = current_local->slice;
        locals[i].data = new_token(label);

        if(i != macro->local_labels_count - 1) {
            current_local = current_local->next_token->next_token;
        }
    }

    //
    // Filling parameters.
    //
    // NOTE(erick): The parameter list is known to be correct.
    // TODO(erick): This can be build once per macro and baked into the macro struct
    if(macro->params == 0) { return result; }

    Token* current_param = macro->params;
    for(int i = 0; i < macro->params_count - 1; i++) {
        params[i].key = current_param->slice;
        current_param = current_param->next_token->next_token;
    }

    // The last param
    params[macro->params_count - 1].key = current_param->slice;

    //
    // Filling arguments
    //
    uint current_arg_index = 0;
    Token* current_arg_token = args;
    while(args) {
        if(args->next_token && string_slice_equals_icase(args->next_token->slice,
                                                       comma_slice)) {
            Token* _arg = args;
            // Add to dict
            params[current_arg_index].data = current_arg_token;
            // Update variables
            current_arg_token = args->next_token->next_token;
            args = args->next_token->next_token;
            current_arg_index++;

            // Finish token list here.
            _arg->next_token = NULL;

            if(current_arg_index == macro->params_count) {
                fail(current_line, MACRO_ERROR, "Too many args for macro (%.*s)",
                     (int) macro->name.len, macro->name.begin);
            }
        } else {
            args = args->next_token;
        }
    }

    if(current_arg_index == macro->params_count) {
        fail(current_line, MACRO_ERROR, "Too many args for macro (%.*s)",
             (int) macro->name.len, macro->name.begin);
    }

    //
    // Last param
    //
    params[current_arg_index].data = current_arg_token;
    current_arg_index++;

    if(current_arg_index != macro->params_count) {
        fail(current_line, MACRO_ERROR, "Too few args for macro (%.*s)",
             (int) macro->name.len, macro->name.begin);
    }

    return result;
}

Line* instanciate_macro_code(Macro* macro, MacroReplaceDict replace_dict) {
    Line* result = (Line*) malloc(sizeof(Line));
    Line* current_line = result;
    Line* macro_line = macro->begin;

    while(true) {
        current_line->comment        = macro_line->comment;
        current_line->has_label      = macro_line->has_label;
        current_line->occupies_space = macro_line->occupies_space;
        current_line->type           = macro_line->type;
        current_line->belongs_to_macro_def = false;

        current_line->tokens = copy_or_replace_tokens(macro_line->tokens, replace_dict);

        if(!macro_line->next_line || macro_line == macro->end) {
            current_line->next_line = NULL;
            break;
        }

        current_line->next_line = (Line*) malloc(sizeof(Line));
        current_line = current_line->next_line;
        macro_line = macro_line->next_line;
    }

    return result;
}

void read_macros(Line* lines) {
    Line* current_line = lines;

    bool is_reading_macro = false;
    string_slice current_macro_name = {0};

    Line* current_macro_begin   = NULL;
    Line* current_macro_end     = NULL;
    Token* current_macro_params = NULL;
    Token* current_macro_locals = NULL;


    while(current_line) {

        if(is_reading_macro) {
            current_line->belongs_to_macro_def = true;

            if(!current_line->next_line) {
                fail(NULL, MACRO_ERROR, "Macro with no END: %.*s\n",
                     (int) current_macro_name.len,
                     current_macro_name.begin);
            }

            if(current_line->next_line->type == MACRO_END) {
                current_line->next_line->belongs_to_macro_def = true;
                is_reading_macro = false;
                current_macro_end = current_line;
                if(search_macro_dict(current_macro_name) != NULL) {
                    fail(current_macro_begin, MACRO_ERROR,
                         "Double macro definition (%.*s)",
                         (int) current_macro_name.len,
                         current_macro_name.begin);
                }

                add_to_macro_dict(current_line,
                                  current_macro_name, current_macro_params,
                                  current_macro_locals, current_macro_begin,
                                  current_macro_end);
            }
        } else {
            if(current_line->type == MACRO_DEF) {
                current_line->belongs_to_macro_def = true;
                is_reading_macro = true;

                current_macro_name   = current_line->tokens->slice;
                current_macro_params = current_line->tokens->next_token->next_token;

                if(current_line->next_line->type == MACRO_LOCALS) {
                    current_macro_locals = current_line->next_line->tokens->next_token;
                    current_macro_begin  = current_line->next_line->next_line;
                } else {
                    current_macro_locals = NULL;
                    current_macro_begin  = current_line->next_line;
                }

                if(current_line->next_line &&
                   current_line->next_line->type == MACRO_END) {
                    fail(current_line, MACRO_ERROR, "Empty Macro definition (%.*s)",
                         (int) current_macro_name.len,
                         current_macro_name.begin);
                }
            }
        }


        current_line = current_line->next_line;
    }

    // NOTE(erick): This should never be reached.
    if(is_reading_macro) {
        fail(NULL, MACRO_ERROR, "Macro with no END: %.*s\n",
             (int) current_macro_name.len,
             current_macro_name.begin);
    }
}

string_slice concat_tokens(char* _prefix, Token* tokens, string_slice suffix) {
    string_slice prefix = make_string_slice(_prefix);
    usize mem_needed = prefix.len + suffix.len;

    Token* current_token = tokens;
    while(current_token) {
        mem_needed += current_token->slice.len + 1;
        current_token = current_token->next_token;
    }

    char* data = (char*) malloc(mem_needed);
    string_slice result = {data, mem_needed};

    memcpy(data, prefix.begin, prefix.len);
    data += prefix.len;

    current_token = tokens;
    while(current_token) {
        *data++ = ' ';
        memcpy(data, current_token->slice.begin, current_token->slice.len);
        data += current_token->slice.len;
        current_token = current_token->next_token;
    }

    memcpy(data, suffix.begin, suffix.len);

    return result;
}

void apply_macros(Line* lines) {
    Line* current_line = lines;
    while(current_line) {
        if(current_line->type == MACRO_INVOKE && !current_line->belongs_to_macro_def) {
            Token* macro_name_token;

            if(current_line->has_label) {
                macro_name_token = current_line->tokens->next_token->next_token;
            } else {
                macro_name_token = current_line->tokens;
            }

            string_slice macro_name = macro_name_token->slice;
            Macro* macro = search_macro_dict(macro_name);
            if(!macro) {
                fail(current_line, MACRO_ERROR, "Invoke non-defined Macro (%.*s)",
                     (int) macro_name.len, macro_name.begin);
            }

            string_slice new_comment = concat_tokens(";",
                                                     macro_name_token,
                                                     current_line->comment);

            Token* macro_args = macro_name_token->next_token;
            // NOTE(erick): build_macro_replace_dict destroys the token argument list.
            MacroReplaceDict replace_dict = build_macro_replace_dict(current_line,
                                                                     macro,
                                                                     macro_args);

            Line* macro_code = instanciate_macro_code(macro, replace_dict);

            Line* old_next_line = current_line->next_line;

            current_line->occupies_space = false;
            current_line->type = current_line->has_label ? LABEL : BLANK;
            current_line->next_line = macro_code;
            current_line->comment = new_comment;

            while(macro_code->next_line) {
                macro_code = macro_code->next_line;
            }

            macro_code->next_line = old_next_line;
            macro->n_macro_instantiation++;

            if(replace_dict.data) { free(replace_dict.data); }
        }

        current_line = current_line->next_line;
    }
}

void remove_macros_space_properties(Line* lines) {
    Line* current_line = lines;
    bool is_reading_macro = false;

    while(current_line) {
        if(is_reading_macro) {
            current_line->occupies_space = false;

            if(current_line->type == MACRO_END) {
                is_reading_macro = false;
            }
        } else {
            current_line->belongs_to_macro_def = false;

            if(current_line->type == MACRO_END) {
                fail(current_line, MACRO_ERROR,
                     "ENDM found with no matching Macro definition");
            }

            if(current_line->type == MACRO_DEF) {
                current_line->belongs_to_macro_def = true;

                is_reading_macro = true;
            }
        }

        current_line = current_line->next_line;
    }
}

void compute_addresses(Line* lines) {
    Line* current_line = lines;

    uint current_address = 0;
    while(current_line) {
        assert(current_line->type != UNKNOWN);


        if(current_line->type == ORIGIN) {
            Token* value_token = current_line->tokens->next_token;
            if(!value_token) {
                fail(current_line, ADDRESS_ERROR, "ORG with no value");
            }

            uint16 org;
            if(!parse_value(value_token, &org)) {
                fail(current_line, ADDRESS_ERROR, "Value could not be parsed (%.*s)",
                     (int) value_token->slice.len,
                     value_token->slice.begin);
            }

            OriginChange change = {current_address, org};
            org_change_list[org_change_list_count] = change;
            org_change_list_count++;

            current_address = org;
        }

        //
        // By now we only have blank lines and lines that generate code.
        //

        current_line->address = current_address;
        if(current_line->has_label && !current_line->belongs_to_macro_def) {
            uint ignored;
            Token* label_token = current_line->tokens;

            if(search_label_dict(label_token->slice, &ignored)) {
                fail(current_line, ADDRESS_ERROR, "Double label definition (%.*s)",
                     (int) label_token->slice.len,
                     label_token->slice.begin);
            }

            add_to_label_dict(label_token->slice, current_address);
        }

        if(current_line->occupies_space) {
            current_address++;
        }

        current_line = current_line->next_line;
    }

    last_address = current_address;
}

void propagate_labels(Line* lines) {
    Line* current_line = lines;

    while(current_line) {
        if(!current_line->belongs_to_macro_def) {
            Token* current_token;
            if(current_line->has_label) {
                current_token = current_line->tokens->next_token->next_token;
            } else {
                current_token = current_line->tokens;
            }

            while(current_token) {
                uint value;
                if(search_label_dict(current_token->slice, &value)) {
                    string_slice value_str = uint_to_string(value);
                    current_token->slice = value_str;
                } else if(string_slice_equals(current_token->slice,
                                              make_string_slice("$"))) {
                    string_slice value_str = uint_to_string(current_line->address);
                    current_token->slice = value_str;
                }

                current_token = current_token->next_token;
            }
        }

        current_line = current_line->next_line;
    }
}

void generate_machine_code(Line* lines) {
    string_slice dw_slice  = make_string_slice("DW");

    Line* current_line = lines;

    while(current_line) {
        if(current_line->occupies_space) {
            uint address = current_line->address;
            if(address >= MEMORY_SIZE) {
                fail(current_line, MACHINE_CODE_ERROR,
                     "Address of line is greater than the memory");
            }

            Token* instruction;
            if(current_line->has_label) {
                instruction = current_line->tokens->next_token->next_token;
            } else {
                instruction = current_line->tokens;
            }

            if(!instruction) {
                fail(current_line, MACHINE_CODE_ERROR,
                     "Line does not contain an instruction");
            }

            Token* operand = instruction->next_token;
            uint16 instruction_code;
            uint16 operand_code;

            if(!operand) {
                fail(current_line, MACHINE_CODE_ERROR,
                     "Line does not contain an operand");
            }

            if(!parse_instruction(instruction, &instruction_code)) {
                fail(current_line, MACHINE_CODE_ERROR,
                     "Line with no valid instruction (%.*s)",
                     (int) instruction->slice.len,
                     instruction->slice.begin);
            }

            if(!parse_value(operand, &operand_code)) {
                fail(current_line, MACHINE_CODE_ERROR,
                     "No valid operand (%.*s)",
                     (int) operand->slice.len,
                     operand->slice.begin);
            }

            if((!string_slice_equals_icase(instruction->slice, dw_slice)) &&
               operand_code >= MAX_OPERAND) {
                fail(current_line, MACHINE_CODE_ERROR,
                     "operand (%.*s) exceeds 14 bits",
                     (int) operand->slice.len,
                     operand->slice.begin);
            }

            current_line->memory_data = instruction_code | operand_code;
            machine_code[address] = current_line->memory_data;
        }

        current_line = current_line->next_line;
    }
}

void handle_includes(Line* lines) {
    string_slice semicolumn_slice = make_string_slice("; ");

    Line* current_line = lines;
    while(current_line) {
        const usize inc_size = strlen("#include");
        if(strncmp(current_line->line_content.begin, "#include", inc_size) == 0) {
            string_slice include = current_line->line_content;
            include.begin += inc_size;
            include.len   -= inc_size;

            string_slice filename = trim_string_slice(include);

            // TODO(erick): Search file in std files directory.
            char* buffer = (char*) alloca(filename.len + 1);
            snprintf(buffer,filename.len + 1, "%s", filename.begin);
            fprintf(stderr, "[NOTE] Including: %s\n", buffer);

            FILE* included = fopen(buffer, "r");
            if(!included) {
                fail(current_line, FILE_ERROR,
                     "Could not found included file: %s\n", buffer);
            }

            char* included_data  = read_entire_file(included);
            Line* included_lines = divide_in_lines(included_data, filename);

            Line* tmp = included_lines;
            if(!tmp) {
                fail(current_line, INCLUDE_ERROR,
                     "Cannot include empty file: %s\n", buffer);
            }

            while(tmp->next_line) { tmp = tmp->next_line; }

            //
            // Inserting the included lines at the include place
            //
            tmp->next_line = current_line->next_line;
            current_line->next_line = included_lines;

            // Current line is replaced with a blank line with a comment.
            current_line->line_content =
                allocate_and_concatenate(semicolumn_slice,
                                         current_line->line_content);
        }

        current_line = current_line->next_line;
    }
}

int main(int args_count, char** args_values) {

    char* output_filename = NULL;
    char* input_filename = NULL;

    char** next_assignment = NULL;
    for(int arg_index = 1; arg_index < args_count; arg_index++) {
        char* current_arg = args_values[arg_index];

        if(next_assignment) {
            *next_assignment = current_arg;
            next_assignment = NULL;
            continue;
        }

        if(strcmp(current_arg, "-h") == 0) {
            print_help_and_exit(0);

        } else if(strcmp(current_arg, "-H") == 0) {
            output_intel_hex = true;

        } else if(strcmp(current_arg, "-c") == 0) {
            output_ces_hex = true;

        } else if(strcmp(current_arg, "-B") == 0) {
            output_separated_binaries = true;

        } else if(strcmp(current_arg, "-o") == 0) {
            next_assignment = &output_filename;

        } else {
            input_filename = current_arg;
        }
    }

    if(next_assignment) {
        perror("Invalid parameters.\n");
        print_help_and_exit(ARGS_PARSING_ERROR);
    }

    if(!input_filename) {
        perror("No input file specified.\n");
        print_help_and_exit(ARGS_PARSING_ERROR);
    }

    if(!output_filename) {
        string_slice input_slice = make_string_slice(input_filename);

        string_slice output_slice = input_slice;
        output_slice.len -= find_char_from_right(input_slice, '.').len;

        output_filename = (char*) malloc(output_slice.len + 1);
        sprintf(output_filename, "%.*s", (int) output_slice.len, output_slice.begin);
    }

    FILE* input_file = fopen(input_filename, "r");
    if(!input_file) {
        perror("Could not open input file.\n");
        exit(FILE_ERROR);
    }

    char* data = read_entire_file(input_file);

    Line* lines = divide_in_lines(data, make_string_slice(input_filename));

    handle_includes(lines);

    tokenize_lines(lines);

    classify_lines(lines);

    apply_equalities(lines);

    resolve_unknowns(lines);

    read_macros(lines);
    apply_macros(lines);
    remove_macros_space_properties(lines);

    compute_addresses(lines);
    propagate_labels(lines);
    // resolve_expressions(lines);
    generate_machine_code(lines);

    output_binary_file(output_filename);
    output_lst_file(output_filename, lines);
    if(output_intel_hex) {
        output_hex_file(output_filename);
    }

    if(output_ces_hex) {
        output_ces_hex_file(output_filename);
    }

    if(output_separated_binaries) {
        output_separated_binary_files(output_filename);
    }

    return 0;
}
