/**
 * @file ez8_assembler.c
 * @brief This module contains functions to compile assembly programs.
 * @author Lucas Jadilo
 */

/**************************************************************************************************/
/*  Private Includes                                                                              */
/**************************************************************************************************/

#include "ez8_assembler.h"
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**************************************************************************************************/
/*  Private Macros                                                                                */
/**************************************************************************************************/

#define EZ8_ASM_MAX_LABELS      100U
#define EZ8_ASM_LABEL_MAX_SIZE  100U
#define EZ8_ASM_LINE_MAX_SIZE   1000U
#define EZ8_ASM_LINE_MAX_TOKENS 10U

/**************************************************************************************************/
/*  Private Types                                                                                 */
/**************************************************************************************************/

typedef enum ez8_asm_directive {
    EZ8_ASM_CODE = 0,
    EZ8_ASM_DATA,
    EZ8_ASM_END,
    EZ8_ASM_DIRECTIVES
} ez8_asm_directive_t;

typedef struct ez8_asm_line {
    const char *label;
    const char *op_label;
    uint8_t data[EZ8_ASM_LINE_MAX_SIZE];
    uint8_t data_size;
    int16_t opcode;
    int16_t operand;
    int8_t directive;
} ez8_asm_line_t;

typedef struct ez8_asm_label {
    char str[EZ8_ASM_LABEL_MAX_SIZE];
    uint8_t addr;
} ez8_asm_label_t;

typedef struct ez8_asm_labels {
    ez8_asm_label_t labels[EZ8_ASM_MAX_LABELS];
    uint16_t size;
} ez8_asm_labels_t;

/**************************************************************************************************/
/*  Private Variables                                                                             */
/**************************************************************************************************/

static const char *directives[EZ8_ASM_DIRECTIVES] = {
    [EZ8_ASM_CODE] = ".CODE", [EZ8_ASM_DATA] = ".DATA", [EZ8_ASM_END] = "END"};

/**************************************************************************************************/
/*  Private Function Declarations                                                                 */
/**************************************************************************************************/

static int8_t ez8_asm_parse_line(char *line_str, ez8_asm_line_t *line, bool code_section);

static int8_t ez8_asm_alloc_line(const ez8_asm_line_t *line, uint8_t *compiled_bytes,
                                 uint16_t *address, ez8_asm_labels_t *def_labels,
                                 ez8_asm_labels_t *used_labels);

static int16_t ez8_asm_parse_uint8(const char *str);

static int16_t ez8_asm_parse_label(const char *str, bool define);

static int16_t ez8_asm_parse_mnemonic(const char *str);

static int8_t ez8_asm_parse_directive(const char *str);

/**************************************************************************************************/
/*  Public Function Definitions                                                                   */
/**************************************************************************************************/

int8_t ez8_asm_preprocess(const char *asm_file_name, const char *pp_file_name)
{
    if ((asm_file_name == NULL) || (pp_file_name == NULL)) {
        EZ8_ERROR("Invalid pointer");
        return -1;
    }

    printf("Preprocessing assembly file `%s`\n", asm_file_name);

    OPEN_FILE(asm_file, asm_file_name, "r");
    OPEN_FILE(pp_file, pp_file_name, "w");

    char line[EZ8_ASM_LINE_MAX_SIZE];

    while (fgets(line, sizeof(line), asm_file) != NULL) {
        unsigned int ri = 0, wi = 0;

        while (isspace(line[ri])) { // Remove leading space
            ri++;
        }

        if ((line[ri] == '\0') || (line[ri] == ';')) { // Remove empty or comment lines
            continue;
        }

        bool space = false;
        for (; (line[ri] != '\0') && (line[ri] != ';'); ri++) {
            if (isspace(line[ri])) {
                space = true;
            } else {
                if (space) { // Convert a contiguous white space to 1 space character
                    line[wi++] = ' ';
                }
                line[wi++] = line[ri];
                space = false;
            }
        }

        line[wi++] = '\n';
        line[wi] = '\0';
        if (fputs(line, pp_file) == EOF) {
            EZ8_ERROR("Failed to write file `%s`", pp_file_name);
            return -1;
        }
    }

    fclose(asm_file);
    fclose(pp_file);
    printf("Preprocessed file `%s` created successfully\n", pp_file_name);
    return 0;
}

int8_t ez8_asm_compile(const char *pp_file_name, uint8_t compiled_bytes[EZ8_MEMORY_SIZE])
{
    if ((pp_file_name == NULL) || (compiled_bytes == NULL)) {
        EZ8_ERROR("Invalid pointer");
        return -1;
    }

    printf("Compiling preprocessed file `%s`\n", pp_file_name);

    OPEN_FILE(file, pp_file_name, "r");

    bool code_section = true;
    bool end_found = false;
    uint16_t address = 0;
    ez8_asm_labels_t def_labels = {.size = 0};
    ez8_asm_labels_t used_labels = {.size = 0};
    char line_str[EZ8_ASM_LINE_MAX_SIZE];

    for (unsigned int line_num = 1; fgets(line_str, EZ8_ASM_LINE_MAX_SIZE, file) != NULL;
         line_num++) {
        EZ8_DEBUG("Line %u:", line_num);
        ez8_asm_line_t line;
        if (ez8_asm_parse_line(line_str, &line, code_section)) {
            EZ8_ERROR("invalid line %u in file `%s`", line_num, pp_file_name);
            return -1;
        }

        if (line.directive == EZ8_ASM_CODE) {
            code_section = true;
            EZ8_DEBUG("\tdirective: code section = 0x%02X\n", address);
            continue;
        } else if (line.directive == EZ8_ASM_DATA) {
            code_section = false;
            EZ8_DEBUG("\tdirective: data section = 0x%02X\n", address);
            continue;
        } else if (line.directive == EZ8_ASM_END) {
            EZ8_DEBUG("\tdirective: end program = 0x%02X\n", address);
            end_found = true;
            break;
        }

        if (ez8_asm_alloc_line(&line, compiled_bytes, &address, &def_labels, &used_labels)) {
            EZ8_ERROR("invalid line %u in file `%s`", line_num, pp_file_name);
            return -1;
        }
    }

    fclose(file);

    if (!end_found) {
        EZ8_ERROR("Directive `END` not found");
        return -1;
    }

    EZ8_DEBUG("Defined Labels:\n");
    for (unsigned int i = 0; i < def_labels.size; i++) {
        EZ8_DEBUG("\t%s = 0x%02X\n", def_labels.labels[i].str, def_labels.labels[i].addr);
    }

    EZ8_DEBUG("Used Labels:\n");
    for (unsigned int i = 0; i < used_labels.size; i++) {
        EZ8_DEBUG("\t%s => *0x%02X\n", used_labels.labels[i].str, used_labels.labels[i].addr);
        bool label_defined = false;
        for (unsigned int j = 0; j < def_labels.size; j++) {
            if (strcmp(used_labels.labels[i].str, def_labels.labels[j].str) == 0) {
                compiled_bytes[used_labels.labels[i].addr] = def_labels.labels[j].addr;
                label_defined = true;
                break;
            }
        }
        if (!label_defined) {
            EZ8_ERROR("Undefined label `%s`", used_labels.labels[i].str);
            return -1;
        }
    }

    printf("Preprocessed file `%s` compiled successfully\n", pp_file_name);
    return 0;
}

int8_t ez8_asm_create_hex_file(const char *hex_file_name,
                               const uint8_t compiled_bytes[EZ8_MEMORY_SIZE],
                               const bool fmt_compact)
{
    if ((hex_file_name == NULL) || (compiled_bytes == NULL)) {
        EZ8_ERROR("Invalid pointer");
        return -1;
    }

    printf("Creating hexadecimal file `%s`\n", hex_file_name);

    OPEN_FILE(file, hex_file_name, "w");

    PRINT_FILE(file, hex_file_name,
               "     00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n"
               "     -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --\n");

    for (unsigned int addr = 0; addr < EZ8_MEMORY_SIZE; addr += 16) {
        PRINT_FILE(file, hex_file_name, "%02X :", addr);
        const unsigned int max_addr = addr + 16;
        for (unsigned int i = addr; i < max_addr; i++) {
            PRINT_FILE(file, hex_file_name, " %02X", compiled_bytes[i]);
        }

        if (fmt_compact) {
            PRINT_FILE(file, hex_file_name, "  ");
            for (unsigned int i = addr; i < max_addr; i++) {
                PRINT_FILE(file, hex_file_name, "%c",
                           isprint(compiled_bytes[i]) ? compiled_bytes[i] : '.');
            }
        } else {
            PRINT_FILE(file, hex_file_name, "\n    ");
            for (unsigned int i = addr; i < max_addr; i++) {
                PRINT_FILE(file, hex_file_name, " %2c",
                           isprint(compiled_bytes[i]) ? compiled_bytes[i] : ' ');
            }
        }

        PRINT_FILE(file, hex_file_name, "\n");
    }

    fclose(file);
    printf("File `%s` created successfully\n", hex_file_name);
    return 0;
}

int8_t ez8_asm_create_bin_file(const char *bin_file_name,
                               const uint8_t compiled_bytes[EZ8_MEMORY_SIZE])
{
    if ((bin_file_name == NULL) || (compiled_bytes == NULL)) {
        EZ8_ERROR("Invalid pointer");
        return -1;
    }

    printf("Creating binary file `%s`\n", bin_file_name);

    OPEN_FILE(file, bin_file_name, "wb");

    if (fwrite(compiled_bytes, sizeof(compiled_bytes[0]), EZ8_MEMORY_SIZE, file) !=
        EZ8_MEMORY_SIZE) {
        EZ8_ERROR("Unable to write file `%s`", bin_file_name);
        return -1;
    }

    fclose(file);
    printf("File `%s` created successfully\n", bin_file_name);
    return 0;
}

/**************************************************************************************************/
/*  Private Function Definitions                                                                  */
/**************************************************************************************************/

/**
 * @brief Validate and convert an Assembly line.
 * @param[in,out] line_str Array of characters from the line.
 * @param[out] line Struct that stores the parsed line data.
 * @param[in] code_section `true` if the line is within a code section, or `false` if the line is
 * within a data section.
 * @retval 0: Success.
 * @retval -1: Error.
 */
static int8_t ez8_asm_parse_line(char *line_str, ez8_asm_line_t *line, const bool code_section)
{
    char *_tokens[EZ8_ASM_LINE_MAX_TOKENS] = {0};
    char **tokens = _tokens;
    unsigned int n_tokens = 0;

    for (unsigned int i = 0, ti = 0; line_str[i] != '\0'; i++) {
        if (isspace(line_str[i])) {
            line_str[i] = '\0';
            if (n_tokens >= EZ8_ASM_LINE_MAX_TOKENS) {
                EZ8_DEBUG_("\n");
                EZ8_ERROR("Exceeded the maximum number of tokens per line (max = %u)",
                          EZ8_ASM_LINE_MAX_TOKENS);
                return -1;
            }
            tokens[n_tokens++] = &line_str[i - ti];
            ti = 0;
            EZ8_DEBUG_(" %s,", tokens[n_tokens - 1]);
        } else {
            ti++;
            continue;
        }
    }
    EZ8_DEBUG_("\n");

    line->label = NULL;
    line->op_label = NULL;
    line->data_size = 0;
    line->opcode = -1;
    line->operand = -1;
    line->directive = -1;

    line->directive = ez8_asm_parse_directive(tokens[0]);
    if (line->directive >= 0) {
        if (n_tokens == 1) {
            return 0;
        }
        EZ8_ERROR("Directive lines must have only 1 token");
        return -1;
    }

    const int16_t label_size = ez8_asm_parse_label(tokens[0], true);
    if (label_size >= 0) {
        tokens[0][label_size] = '\0';
        line->label = tokens[0];
        if (n_tokens == 1) {
            return 0;
        }
        tokens++;
        n_tokens--;
    }

    if (code_section) {
        EZ8_DEBUG("\tcode line\n");
        if (n_tokens == 1) {
            line->opcode = ez8_asm_parse_mnemonic(tokens[0]);
            if (line->opcode < 0) {
                EZ8_ERROR("`%s` is not a valid label definition nor an instruction mnemonic",
                          tokens[0]);
                return -1;
            }
            if (instructions[line->opcode].need_operand) {
                EZ8_ERROR("instruction `%s` requires an operand",
                          instructions[line->opcode].mnemonic);
                return -1;
            }
        } else if (n_tokens == 2) {
            line->opcode = ez8_asm_parse_mnemonic(tokens[0]);
            if (line->opcode < 0) {
                EZ8_ERROR("`%s` is not a valid label definition nor an instruction mnemonic",
                          tokens[0]);
                return -1;
            }
            if (!instructions[line->opcode].need_operand) {
                EZ8_ERROR("instruction `%s` does not require an operand",
                          instructions[line->opcode].mnemonic);
                return -1;
            }
            line->operand = ez8_asm_parse_uint8(tokens[1]);
            if ((line->operand < 0) && (ez8_asm_parse_label(tokens[1], false) < 0)) {
                EZ8_ERROR("invalid operand `%s`", tokens[1]);
                return -1;
            }
            if (line->operand < 0) {
                line->op_label = tokens[1];
            }
        } else {
            EZ8_ERROR("Exceeded the maximum number of tokens per code section line (max = 3)");
            return -1;
        }
    } else {
        EZ8_DEBUG("\tdata line\n");
        for (unsigned int i = 0; i < n_tokens; i++) {
            const int16_t data_int = ez8_asm_parse_uint8(tokens[i]);
            if (data_int >= 0) {
                const uint8_t data_u8 = (uint8_t)data_int;
                EZ8_DEBUG("\tint: 0x%02X (%u)\n", data_u8, data_u8);
                line->data[line->data_size++] = data_u8;
            } else {
                if (tokens[i][0] != '\"') {
                    EZ8_ERROR("invalid data token `%s`", tokens[i]);
                    return -1;
                }
                EZ8_DEBUG("\tstr: `");
                unsigned int j = 1;
                for (; (tokens[i][j] != '\0') && (tokens[i][j] != '\"'); j++) {
                    line->data[line->data_size++] = (uint8_t)tokens[i][j];
                    EZ8_DEBUG_("%c", line->data[line->data_size - 1]);
                }
                EZ8_DEBUG_("`\n");
                if (tokens[i][j] != '\"') {
                    EZ8_ERROR("invalid data token `%s`", tokens[i]);
                    return -1;
                }
            }
        }
    }

    return 0;
}

/**
 * @brief Allocate the compiled bytes based on the line parsed data.
 * @param[in] line Struct that stores the parsed line data.
 * @param[out] compiled_bytes Array that stores the compiled program bytes.
 * @param[in,out] address Current address that should received the compiled bytes.
 * @param[in,out] def_labels Struct that stores all the labels defined so far.
 * @param[in,out] used_labels Struct that stores all the labels used so far.
 * @retval 0: Success.
 * @retval -1: Error.
 */
static int8_t ez8_asm_alloc_line(const ez8_asm_line_t *line, uint8_t *compiled_bytes,
                                 uint16_t *address, ez8_asm_labels_t *def_labels,
                                 ez8_asm_labels_t *used_labels)
{
    if (line->label != NULL) {
        EZ8_DEBUG("\tlabel: `%s` = 0x%02X\n", line->label, *address);
        if (def_labels->size >= EZ8_ASM_MAX_LABELS) {
            EZ8_ERROR("Exceeded the maximum number of labels (max = %u)", EZ8_ASM_MAX_LABELS);
            return -1;
        }
        strcpy(def_labels->labels[def_labels->size].str, line->label);
        def_labels->labels[def_labels->size++].addr = (uint8_t)*address;
    }

#define CHECK_ADDRESS(address)                                                                     \
    do {                                                                                           \
        if ((address) >= EZ8_MEMORY_SIZE) {                                                        \
            EZ8_ERROR("Exceeded the addressable memory size (max = %u bytes)", EZ8_MEMORY_SIZE);   \
            return -1;                                                                             \
        }                                                                                          \
    } while (0)

    if (line->opcode >= 0) {
        const uint8_t opcode_u8 = (uint8_t)line->opcode;
        EZ8_DEBUG("\topcode: 0x%02X (%s) => *0x%02X\n", opcode_u8, instructions[opcode_u8].mnemonic,
                  *address);
        CHECK_ADDRESS(*address);
        compiled_bytes[(*address)++] = opcode_u8;
    }

    if (line->operand >= 0) {
        const uint8_t operand_u8 = (uint8_t)line->operand;
        EZ8_DEBUG("\toperand: 0x%02X (%u) => *0x%02X\n", operand_u8, operand_u8, *address);
        CHECK_ADDRESS(*address);
        compiled_bytes[(*address)++] = operand_u8;
    } else if (line->op_label != NULL) {
        EZ8_DEBUG("\toperand: label `%s` => *0x%02X\n", line->op_label, *address);
        strcpy(used_labels->labels[used_labels->size].str, line->op_label);
        used_labels->labels[used_labels->size++].addr = (uint8_t)(*address)++;
    }

    if (line->data_size != 0) {
        for (unsigned int i = 0; i < line->data_size; i++) {
            CHECK_ADDRESS(*address);
            compiled_bytes[(*address)++] = line->data[i];
        }
    }

#undef CHECK_ADDRESS
    return 0;
}

/**
 * @brief Validate and convert an 8-bit unsigned integer string. A prefix `0` indicates octal base,
 * a prefix `0x` or `0X` indicates hexadecimal base, and no prefix indicates decimal base.
 * @param[in] str Input string.
 * @return Resulting 8-bit unsigned integer. Return -1 for an invalid string.
 */
static int16_t ez8_asm_parse_uint8(const char *str)
{
    errno = 0;
    char *end;
    const long result = strtol(str, &end, 0);

    if ((end != str + strlen(str)) || (errno == ERANGE) || (result < 0) || (result > 0xFF)) {
        return -1;
    }

    return (int16_t)result;
}

/**
 * @brief Validate an Assembly label string. A label may contain letters `a-z` `A-Z`, numeric digits
 * `0-9` and underscores `_`. A label must start with a letter or underscore, and end with a colon.
 * @param[in] str Input string.
 * @param[in] define `true` indicates the string is a label definition and must end with a colon.
 * @return Label string size. Return -1 for an invalid label.
 */
static int16_t ez8_asm_parse_label(const char *str, const bool define)
{
    if (!isalpha(str[0]) && (str[0] != '_')) {
        return -1;
    }

    unsigned int i = 0;
    while ((str[i] != '\0') && (isalnum(str[i]) || (str[i] == '_'))) {
        if (++i >= EZ8_ASM_LABEL_MAX_SIZE) {
            return -1;
        }
    }

    if (define && ((str[i] != ':') || (str[i + 1] != '\0'))) {
        return -1;
    }

    return (int16_t)i;
}

/**
 * @brief Validate and convert an instruction mnemonic string.
 * @param[in] str Input string.
 * @return Mnemonic index (instruction opcode). Return -1 for an invalid mnemonic.
 */
static int16_t ez8_asm_parse_mnemonic(const char *str)
{
    char mnemonic[10] = {0};
    const unsigned int max_size = sizeof(mnemonic) / sizeof(mnemonic[0]) - 1;
    for (unsigned int i = 0; (i < max_size) && (str[i] != '\0'); i++) {
        mnemonic[i] = (char)toupper(str[i]);
    }

    for (unsigned int i = 0; i < EZ8_INSTRUCTIONS; i++) {
        if (strcmp(mnemonic, instructions[i].mnemonic) == 0) {
            return (int16_t)i;
        }
    }

    return -1;
}

/**
 * @brief Validate and convert an Assembly directive string.
 * @param[in] str Input string.
 * @return Directive index. Return -1 for an invalid directive.
 */
static int8_t ez8_asm_parse_directive(const char *str)
{
    char directive[10] = {0};
    const unsigned int max_size = sizeof(directive) / sizeof(directive[0]) - 1;
    for (unsigned int i = 0; (i < max_size) && (str[i] != '\0'); i++) {
        directive[i] = (char)toupper(str[i]);
    }

    for (unsigned int i = 0; i < EZ8_ASM_DIRECTIVES; i++) {
        if (strcmp(directive, directives[i]) == 0) {
            return (int8_t)i;
        }
    }

    return -1;
}

/****************************************** END OF FILE *******************************************/
