/**
 * @file ez8.c
 * @brief Main module of the EZ8 library.
 * @author Lucas Jadilo
 */

/**************************************************************************************************/
/*  Private Includes                                                                              */
/**************************************************************************************************/

#include "ez8.h"
#include "ez8_assembler.h"
#include "ez8_control_unit.h"
#include <string.h>

/**************************************************************************************************/
/*  Private Macros                                                                                */
/**************************************************************************************************/

#if !defined(EZ8_VERSION_MAJOR) || !defined(EZ8_VERSION_MINOR) || !defined(EZ8_VERSION_PATCH)
#define EZ8_VERSION_MAJOR 0
#define EZ8_VERSION_MINOR 0
#define EZ8_VERSION_PATCH 0
#error Macros EZ8_VERSION_MAJOR, EZ8_VERSION_MINOR, EZ8_VERSION_PATCH must be defined via command line (makefile)
#endif

#define STRINGFY_(x) #x
#define STRINGFY(x)  STRINGFY_(x)

#define EZ8_VERSION_BASE                                                                           \
    STRINGFY(EZ8_VERSION_MAJOR) "." STRINGFY(EZ8_VERSION_MINOR) "." STRINGFY(EZ8_VERSION_PATCH)

#ifdef EZ8_VERSION_PRE
#define EZ8_VERSION_PRE_ "-" EZ8_VERSION_PRE
#else
#define EZ8_VERSION_PRE_ ""
#endif

#ifdef EZ8_VERSION_META
#define EZ8_VERSION_META_ "+" EZ8_VERSION_META
#else
#define EZ8_VERSION_META_ ""
#endif

#define EZ8_VERSION EZ8_VERSION_BASE EZ8_VERSION_PRE_ EZ8_VERSION_META_

/**************************************************************************************************/
/*  Public Variables                                                                              */
/**************************************************************************************************/

const char ez8_version[] = EZ8_VERSION;

/**************************************************************************************************/
/*  Public Function Definitions                                                                   */
/**************************************************************************************************/

int8_t ez8_gen_control_logic(const char *output_dir)
{
    if (output_dir == NULL) {
        EZ8_ERROR("Invalid pointer");
        return -1;
    }

    ez8_cu_truth_table_t ttable;
    ez8_cu_gen_truth_table(&ttable);

    char file_path[1000];
    ez8_concat_file_path(file_path, output_dir, "ez8_cu_eprom0.bin");
    if (ez8_cu_gen_eprom_file(&ttable, 0, file_path)) {
        return -1;
    }

    ez8_concat_file_path(file_path, output_dir, "ez8_cu_eprom1.bin");
    if (ez8_cu_gen_eprom_file(&ttable, 1, file_path)) {
        return -1;
    }

    ez8_concat_file_path(file_path, output_dir, "ez8_cu_eprom2.bin");
    if (ez8_cu_gen_eprom_file(&ttable, 2, file_path)) {
        return -1;
    }

    ez8_concat_file_path(file_path, output_dir, "ez8_cu_eprom3.bin");
    if (ez8_cu_gen_eprom_file(&ttable, 3, file_path)) {
        return -1;
    }

    ez8_concat_file_path(file_path, output_dir, "ez8_cu_instruction_cycles.csv");
    if (ez8_cu_gen_instruction_cycles_csv(&ttable, file_path)) {
        return -1;
    }

    ez8_concat_file_path(file_path, output_dir, "ez8_cu_truth_table.csv");
    if (ez8_cu_gen_truth_table_csv(&ttable, file_path)) {
        return -1;
    }

    printf(EZ8_FONT_BOLD_GREEN "Control logic generated successfully!\n" EZ8_FONT_RESET);
    return 0;
}

int8_t ez8_compile_assembly(const char *asm_file_name, const char *output_dir)
{
    if (asm_file_name == NULL) {
        EZ8_ERROR("Invalid pointer");
        return -1;
    }

    char file_path[1000];

    if (output_dir == NULL) {
        strcpy(file_path, asm_file_name);
    } else {
        ez8_concat_file_path(file_path, output_dir, ez8_base_file_name(asm_file_name));
    }

    ez8_insert_file_extension(file_path, file_path, ".i");
    if (ez8_asm_preprocess(asm_file_name, file_path)) {
        return -1;
    }

    uint8_t compiled_bytes[EZ8_MEMORY_SIZE] = {0};
    if (ez8_asm_compile(file_path, compiled_bytes)) {
        return -1;
    }

    ez8_insert_file_extension(file_path, file_path, ".hex");
    if (ez8_asm_create_hex_file(file_path, compiled_bytes, true)) {
        return -1;
    }

    ez8_insert_file_extension(file_path, file_path, ".bin");
    if (ez8_asm_create_bin_file(file_path, compiled_bytes)) {
        return -1;
    }

    printf(EZ8_FONT_BOLD_GREEN "Compilation successful!\n" EZ8_FONT_RESET);
    return 0;
}

/****************************************** END OF FILE *******************************************/
