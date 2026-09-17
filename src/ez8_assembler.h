/**
 * @file ez8_assembler.h
 * @brief This module contains functions to compile assembly programs.
 * @author Lucas Jadilo
 */

#ifndef EZ8_ASSEMBLER_H
#define EZ8_ASSEMBLER_H

/**************************************************************************************************/
/*  Public Includes                                                                               */
/**************************************************************************************************/

#include "ez8_common.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************/
/*  Public Function Declarations                                                                  */
/**************************************************************************************************/

/**
 * @brief Preprocess an assembly file, removing comments and unecessary white space, and create a
 * new file with the result.
 * @param[in] asm_file_name Path to the assembly file that will be preprocessed.
 * @param[in] pp_file_name Path to the resulting preprocessed file.
 * @retval 0: Success.
 * @retval -1: Error.
 */
int8_t ez8_asm_preprocess(const char *asm_file_name, const char *pp_file_name);

/**
 * @brief Compile a preprocessed assembly file to binary code.
 * @param[in] pp_file_name Path to the preprocessed assembly file that will be compiled.
 * @param[out] compiled_bytes Array that will receive the compiled program bytes.
 * @retval 0: Success.
 * @retval -1: Error.
 */
int8_t ez8_asm_compile(const char *pp_file_name, uint8_t compiled_bytes[EZ8_MEMORY_SIZE]);

/**
 * @brief Create a hexadecimal text file with the given compiled program bytes.
 * @param[in] hex_file_name Path to the new hexadecimal file.
 * @param[in] compiled_bytes Array of compiled bytes to be written.
 * @param[in] fmt_compact `true` for the compact format, `false` for the extended format.
 * @retval 0: Success.
 * @retval -1: Error.
 */
int8_t ez8_asm_create_hex_file(const char *hex_file_name,
                               const uint8_t compiled_bytes[EZ8_MEMORY_SIZE], bool fmt_compact);

/**
 * @brief Create a binary file with the given compiled program bytes.
 * @param[in] bin_file_name Path of the new binary file.
 * @param[in] compiled_bytes Array of compiled bytes to be written.
 * @retval 0: Success.
 * @retval -1: Error.
 */
int8_t ez8_asm_create_bin_file(const char *bin_file_name,
                               const uint8_t compiled_bytes[EZ8_MEMORY_SIZE]);

#ifdef __cplusplus
}
#endif

#endif // EZ8_ASSEMBLER_H

/****************************************** END OF FILE *******************************************/
