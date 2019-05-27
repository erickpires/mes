#include "std_types.h"
#include "mes.h"

extern uint16 machine_code[];
extern OriginChange org_change_list[];
extern usize org_change_list_count;
extern uint last_address;

void output_lst_file(char* file_stem, Line* lines) {
    char* filename = (char*) malloc(strlen(file_stem) + strlen(".lst") + 1);
    strcpy(filename, file_stem);
    strcat(filename, ".lst");

    FILE* file = fopen(filename, "w");
    fprintf(file, "LST file\n\n");

    while(lines) {
        if(lines->type != BLANK) {
            fprintf(file, "%04x: ", lines->address);
            if(lines->occupies_space) {
                fprintf(file, "%04x ", lines->memory_data);
            } else {
                fprintf(file, "     ");
            }

            Token* token = lines->tokens;
            while(token) {
                fprintf(file, "%.*s ", (int) token->slice.len, token->slice.begin);
                token = token->next_token;
            }
        }

        fprintf(file, "%.*s\n", (int) lines->comment.len, lines->comment.begin);

        lines = lines->next_line;
    }

    fclose(file);
}

void output_hex_file(char* file_stem) {
    char* filename = (char*) malloc(strlen(file_stem) + strlen(".hex") + 1);
    strcpy(filename, file_stem);
    strcat(filename, ".hex");

    FILE* file = fopen(filename, "w");
    fprintf(file, "HEX file\n");

    fclose(file);
}

void output_binary_file(char* file_stem) {
    char* filename = (char*) malloc(strlen(file_stem) + strlen(".bin") + 1);
    strcpy(filename, file_stem);
    strcat(filename, ".bin");

    FILE* file = fopen(filename, "wb");

    for(usize mem_index = 0; mem_index < MEMORY_SIZE; mem_index++) {
        uint8 lower_byte = machine_code[mem_index] & 0xFF;
        uint8 upper_byte = machine_code[mem_index] >> 8;

        fwrite(&upper_byte, 1, 1, file);
        fwrite(&lower_byte, 1, 1, file);
    }

    fclose(file);
}

void output_ces_hex_file(char* file_stem) {
    char* filename = (char*) malloc(strlen(file_stem) + strlen(".ces.hex") + 1);
    strcpy(filename, file_stem);
    strcat(filename, ".ces.hex");

    FILE* file = fopen(filename, "w");

    uint current_begin_address = 0;
    for (usize org_change_index = 0;
         org_change_index < org_change_list_count;
         org_change_index++) {
        OriginChange current_change = org_change_list[org_change_index];

        uint current_end_address = current_change.old_origin;

        if(current_end_address > current_begin_address) {
            fprintf(file, "%04X:", current_begin_address);
            for(uint i = current_begin_address; i < current_end_address; i++) {
                fprintf(file, " %04X", machine_code[i]);
            }

            fprintf(file, "\n");
        }

        current_begin_address = current_change.new_origin;
    }

    uint current_end_address = last_address;
    if(current_end_address > current_begin_address) {
        fprintf(file, "%04X:", current_begin_address);
        for(uint i = current_begin_address; i < current_end_address; i++) {
            fprintf(file, " %04X", machine_code[i]);
        }

        fprintf(file, "\n");
    }

    fclose(file);
}

void output_separated_binary_files(char* file_stem) {
    char* filename = (char*) malloc(strlen(file_stem)
                                    + strlen(".1.bin") + 1);
    strcpy(filename, file_stem);
    strcat(filename, ".1.bin");

    FILE* file1 = fopen(filename, "wb");

    strcpy(filename, file_stem);
    strcat(filename, ".2.bin");

    FILE* file2 = fopen(filename, "wb");

    for(usize mem_index = 0; mem_index < MEMORY_SIZE; mem_index++) {
        uint8 lower_byte = machine_code[mem_index] & 0xFF;
        uint8 upper_byte = machine_code[mem_index] >> 8;

        fwrite(&lower_byte, 1, 1, file1);
        fwrite(&upper_byte, 1, 1, file2);
    }

    fclose(file1);
    fclose(file2);
}
