/**
 * @file ez8_common.c
 * @brief Common utilities for other modules.
 * @author Lucas Jadilo
 */

/**************************************************************************************************/
/*  Private Includes                                                                              */
/**************************************************************************************************/

#include "ez8_common.h"
#include <string.h>

/**************************************************************************************************/
/*  Public Variables                                                                              */
/**************************************************************************************************/

const ez8_instruction_t instructions[EZ8_INSTRUCTIONS] = {
    [EZ8_OPCODE_LDA] = {"LDA", true},    [EZ8_OPCODE_LDI] = {"LDI", true},
    [EZ8_OPCODE_STA] = {"STA", true},    [EZ8_OPCODE_LIA] = {"LIA", false},
    [EZ8_OPCODE_NOT] = {"NOT", false},   [EZ8_OPCODE_AND] = {"AND", true},
    [EZ8_OPCODE_OR] = {"OR", true},      [EZ8_OPCODE_XOR] = {"XOR", true},
    [EZ8_OPCODE_SHL] = {"SHL", false},   [EZ8_OPCODE_SHR] = {"SHR", false},
    [EZ8_OPCODE_ADD] = {"ADD", true},    [EZ8_OPCODE_SUB] = {"SUB", true},
    [EZ8_OPCODE_CMP] = {"CMP", true},    [EZ8_OPCODE_JMP] = {"JMP", true},
    [EZ8_OPCODE_JPE] = {"JPE", true},    [EZ8_OPCODE_JPA] = {"JPA", true},
    [EZ8_OPCODE_JPB] = {"JPB", true},    [EZ8_OPCODE_CALL] = {"CALL", true},
    [EZ8_OPCODE_RET] = {"RET", false},   [EZ8_OPCODE_LSP] = {"LSP", false},
    [EZ8_OPCODE_PUSH] = {"PUSH", false}, [EZ8_OPCODE_POP] = {"POP", false},
    [EZ8_OPCODE_IN] = {"IN", true},      [EZ8_OPCODE_OUT] = {"OUT", true},
    [EZ8_OPCODE_NOP] = {"NOP", false},   [EZ8_OPCODE_HALT] = {"HALT", false}};

/**************************************************************************************************/
/*  Public Function Definitions                                                                   */
/**************************************************************************************************/

const char *ez8_base_file_name(const char *full_path)
{
    const char *last_slash;

    if ((NULL != (last_slash = strrchr(full_path, '/'))) ||
        (NULL != (last_slash = strrchr(full_path, '\\')))) {
        return last_slash + 1;
    }

    return full_path;
}

void ez8_concat_file_path(char *full_path, const char *dir_path, const char *base_name)
{
    do {
        *full_path++ = *dir_path;
    } while (*dir_path++);
    full_path--;

    if ((*(full_path - 1) != '/') && (*(full_path - 1) != '\\')) {
        *full_path++ = '/';
    }

    do {
        *full_path++ = *base_name;
    } while (*base_name++);
}

void ez8_insert_file_extension(char *result, const char *file_name, const char *extension)
{
    char *start = result;

    do {
        *result++ = *file_name;
    } while (*file_name++);

    char *end = result - 1;

    while ((*--result != '.') && (result != start)) {}

    if (result == start) {
        result = end;
    }

    do {
        *result++ = *extension;
    } while (*extension++);
}

/****************************************** END OF FILE *******************************************/
