/**
 * @file ez8_common.h
 * @brief Common utilities for other modules.
 * @author Lucas Jadilo
 */

#ifndef EZ8_COMMON_H
#define EZ8_COMMON_H

/**************************************************************************************************/
/*  Public Includes                                                                               */
/**************************************************************************************************/

#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************/
/*  Public Macros                                                                                 */
/**************************************************************************************************/

#define EZ8_MEMORY_SIZE 256U

#define EZ8_FONT_BOLD_RED   "\x1B[1;31m"
#define EZ8_FONT_BOLD_GREEN "\x1B[1;32m"
#define EZ8_FONT_BOLD_CYAN  "\x1B[1;36m"
#define EZ8_FONT_RESET      "\x1B[0m"

#define EZ8_ERROR(...)                                                                             \
    do {                                                                                           \
        printf(EZ8_FONT_BOLD_RED "ERROR: " __VA_ARGS__);                                           \
        puts(EZ8_FONT_RESET);                                                                      \
    } while (0)

#define OPEN_FILE(file, file_name, mode)                                                           \
    FILE *file = fopen((file_name), (mode));                                                       \
    do {                                                                                           \
        if ((file) == NULL) {                                                                      \
            EZ8_ERROR("Not able to open file `%s`", (file_name));                                  \
            return -1;                                                                             \
        }                                                                                          \
    } while (0)

#define PRINT_FILE(file, file_name, ...)                                                           \
    do {                                                                                           \
        if (fprintf((file), __VA_ARGS__) < 0) {                                                    \
            EZ8_ERROR("Unable to write file `%s`", (file_name));                                   \
            return -1;                                                                             \
        }                                                                                          \
    } while (0)

#ifdef DEBUG
#define EZ8_DEBUG(...)  fprintf(stderr, EZ8_FONT_BOLD_CYAN "[DEBUG] " EZ8_FONT_RESET __VA_ARGS__)
#define EZ8_DEBUG_(...) fprintf(stderr, __VA_ARGS__)
#else
#define EZ8_DEBUG(...)
#define EZ8_DEBUG_(...)
#endif

/**************************************************************************************************/
/*  Public Types                                                                                  */
/**************************************************************************************************/

typedef enum ez8_opcode {
    EZ8_OPCODE_LDA = 0,
    EZ8_OPCODE_LDI,
    EZ8_OPCODE_STA,
    EZ8_OPCODE_LIA,
    EZ8_OPCODE_NOT,
    EZ8_OPCODE_AND,
    EZ8_OPCODE_OR,
    EZ8_OPCODE_XOR,
    EZ8_OPCODE_SHL,
    EZ8_OPCODE_SHR,
    EZ8_OPCODE_ADD,
    EZ8_OPCODE_SUB,
    EZ8_OPCODE_CMP,
    EZ8_OPCODE_JMP,
    EZ8_OPCODE_JPE,
    EZ8_OPCODE_JPA,
    EZ8_OPCODE_JPB,
    EZ8_OPCODE_CALL,
    EZ8_OPCODE_RET,
    EZ8_OPCODE_LSP,
    EZ8_OPCODE_PUSH,
    EZ8_OPCODE_POP,
    EZ8_OPCODE_IN,
    EZ8_OPCODE_OUT,
    EZ8_OPCODE_NOP,
    EZ8_OPCODE_HALT,
    EZ8_INSTRUCTIONS
} ez8_opcode_t;

typedef struct ez8_instruction {
    const char *mnemonic;
    bool need_operand;
} ez8_instruction_t;

/**************************************************************************************************/
/*  Public Variables                                                                              */
/**************************************************************************************************/

extern const ez8_instruction_t instructions[];

/**************************************************************************************************/
/*  Public Function Declarations                                                                  */
/**************************************************************************************************/

/**
 * @brief Extract the base name from a file path (name after the last slash).
 * @param[in] full_path Full file path string.
 * @return Pointer to the first character of the base name.
 */
const char *ez8_base_file_name(const char *full_path);

/**
 * @brief Concatenate a base file name to a directory path.
 * @param[out] full_path Array that will store the resulting file path.
 * @param[in] dir_path Directory path string.
 * @param[in] base_name Base file name string.
 */
void ez8_concat_file_path(char *full_path, const char *dir_path, const char *base_name);

/**
 * @brief Insert or overwrite a file extension in a file name.
 * @param[out] result Pointer to the array that will store the resulting string.
 * @param[in] file_name Current file name with or without an extension.
 * @param[in] extension Extension string to be inserted.
 */
void ez8_insert_file_extension(char *result, const char *file_name, const char *extension);

#ifdef __cplusplus
}
#endif

#endif // EZ8_COMMON_H

/****************************************** END OF FILE *******************************************/
