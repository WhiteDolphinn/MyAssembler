#include <string.h>
#include <stdlib.h>
#include "text.h"
#include "my_assembler.h"

#define GET_CMD_CODE(COMMAND, CODE)         \
    do{                                     \
        if(!stricmp(COMMAND, TO_STR(PUSH))) \
            CODE = PUSH;                    \
        if(!stricmp(COMMAND, TO_STR(HLT)))  \
            CODE = HLT;                     \
        if(!stricmp(COMMAND, TO_STR(OUT)))  \
            CODE = OUT;                     \
        if(!stricmp(COMMAND, TO_STR(POP)))  \
            CODE = POP;                     \
        if(!stricmp(COMMAND, TO_STR(ADD)))  \
            CODE = ADD;                     \
        if(!stricmp(COMMAND, TO_STR(SUB)))  \
            CODE = SUB;                     \
        if(!stricmp(COMMAND, TO_STR(MUL)))  \
            CODE = MUL;                     \
        if(!stricmp(COMMAND, TO_STR(DIV)))  \
            CODE = DIV;                     \
        if(!stricmp(COMMAND, TO_STR(JMP)))  \
            CODE = JMP;                     \
        if(!stricmp(COMMAND, TO_STR(JB)))   \
            CODE = JB;                      \
        if(!stricmp(COMMAND, TO_STR(JBE)))  \
            CODE = JBE;                     \
        if(!stricmp(COMMAND, TO_STR(JA)))   \
            CODE = JA;                      \
        if(!stricmp(COMMAND, TO_STR(JAE)))  \
            CODE = JAE;                     \
        if(!stricmp(COMMAND, TO_STR(JE)))   \
            CODE = JE;                      \
        if(!stricmp(COMMAND, TO_STR(JNE)))  \
            CODE = JNE;                     \
    }while(0)                               \

#define GET_REGIST_CODE(COMMAND, CODE)      \
    do{                                     \
        if(!stricmp(COMMAND, TO_STR(AX)))   \
            CODE = AX;                      \
        if(!stricmp(COMMAND, TO_STR(BX)))   \
            CODE = BX;                      \
        if(!stricmp(COMMAND, TO_STR(CX)))   \
            CODE = CX;                      \
        if(!stricmp(COMMAND, TO_STR(DX)))   \
            CODE = DX;                      \
        if(!stricmp(COMMAND, TO_STR(EX)))   \
            CODE = EX;                      \
    }while(0)                               \

static bool is_empty_string(const char* str);
static bool is_link_string(const char* str);
static void add_link(int link_num, int cur_data_position, int* link_positions);
static bool check_regist_command(const char* str, int* code_buffer, int* code_reg);
static int reg_cmd(const char* buf_reg);

bool convertor(
               FILE* file_txt,
               FILE* file_bin,
               struct string* strings,
               int num_of_lines,
               int* uncorrect_line,
               int* link_positions,
               int mode
               )
{
    int data_bin[num_of_lines * 2] = {};/////
    int cur_data_position = 0;

    for (int i = 0; i < num_of_lines - 1; i++)
    {
        *uncorrect_line = i + 1;
        char cmd_buffer[MAX_STR_LENGTH] = "";

        sscanf(strings[i].position, " %14s", cmd_buffer);

        if(is_empty_string(cmd_buffer))
        {
            fprintf(file_txt, "\n");
            continue;
        }

        if(is_link_string(cmd_buffer))
        {
            int link_num = 0;
            sscanf(cmd_buffer, " :%d", &link_num);
            add_link(link_num, cur_data_position, link_positions);
            fprintf(file_txt, "\n");
            continue;
        }

        int code_buffer = ERROR;
        GET_CMD_CODE(cmd_buffer, code_buffer);

        int arg_reg = ERROR;
        if(code_buffer == PUSH || code_buffer == POP)
            if(check_regist_command(strings[i].position, &code_buffer, &arg_reg))
            {
                fprintf(file_txt, "%d", code_buffer);
                data_bin[cur_data_position++] = code_buffer;
                data_bin[cur_data_position++] = arg_reg;
                fprintf(file_txt, " %d\n", arg_reg);
                continue;
            }

        switch(code_buffer)
        {
            case PUSH:  //1 argument command
            {
                double arg_d = 0;
                if(!sscanf(strings[i].position + strlen(TO_STR(PUSH)), " %lf", &arg_d))
                    return false;

                int arg_i = (int)(arg_d * 100 + 0.5);

                fprintf(file_txt, "%d", PUSH);
                data_bin[cur_data_position++] = PUSH;
                data_bin[cur_data_position++] = arg_i;
                fprintf(file_txt, " %d", arg_i);
            }
            break;

            case JMP: case JB: case JBE: case JA: case JAE: case JE: case JNE:
            {
                int position = 0;
                bool is_link = false;
                char first_symbol = '\0';

                sscanf(strings[i].position + strlen(cmd_buffer), " %c", &first_symbol);

                if(first_symbol == ':')
                    is_link = true;

                if(sscanf(strings[i].position + strlen(cmd_buffer), " %c%d", &first_symbol, &position) != 2)
                    return false;

                fprintf(file_txt, "%d", code_buffer);
                data_bin[cur_data_position++] = code_buffer;

                if(is_link)
                {
                    position = link_positions[position];

                    if(mode == 2)
                    {
                        if(position == -1)
                            return false;
                    }
                }

                data_bin[cur_data_position++] = position;
                fprintf(file_txt, " %d", position);
            }
            break;

            case HLT: case OUT: case POP: case ADD: case SUB: case MUL: case DIV: //0 argument command
            {
                fprintf(file_txt, "%d", code_buffer);
                char symbol = '\0';
                sscanf(strings[i].position + strlen(cmd_buffer), " %c", &symbol);

                if(symbol)
                    return false;

                data_bin[cur_data_position++] = code_buffer;
            }
            break;

            default:
                return false;
        }

        fprintf(file_txt, "\n");
    }

    fwrite(data_bin, sizeof(int), cur_data_position, file_bin);

    return true;
}

static bool is_empty_string(const char* str)
{
    if(*str == '\0')
        return true;
    else
        return false;
}

static bool is_link_string(const char* str)
{
    if(*str == ':')
        return true;
    else
        return false;
}

static void add_link(int link_num, int cur_data_position, int* link_positions)
{
    link_positions[link_num] = cur_data_position;
}

static bool check_regist_command(const char* str, int* code_buffer, int* code_reg)
{
    char buf_cmd[MAX_STR_LENGTH] = {};
    char buf_reg[MAX_STR_LENGTH] = {};

    if(sscanf(str, "%s %s", buf_cmd, buf_reg) == 2)
    {
        *code_reg = reg_cmd(buf_reg);
        if(*code_reg != -1)
        {
            switch(*code_buffer)
            {
                case PUSH:
                *code_buffer = PUSH_R;
                break;

                case POP:
                *code_buffer = POP_R;
                break;
            }
            return true;
        }
    }

    return false;
}

static int reg_cmd(const char* buf_reg)
{
    int cmd_code = -1;
    GET_REGIST_CODE(buf_reg, cmd_code);
    return cmd_code;
}
