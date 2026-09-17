/**
 * @file ez8.h
 * @brief Main module of the EZ8 library. The library should be used by including this file.
 * @author Lucas Jadilo
 */

#ifndef EZ8_H
#define EZ8_H

/**************************************************************************************************/
/*  Public Includes                                                                               */
/**************************************************************************************************/

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************/
/*  Public Variables                                                                              */
/**************************************************************************************************/

extern const char ez8_version[];

/**************************************************************************************************/
/*  Public Function Declarations                                                                  */
/**************************************************************************************************/

/**
 * @brief Generate the Control Unit's truth table and EPROM binary files.
 * @param[in] output_dir Directory path for all output files.
 * @retval 0: Success.
 * @retval -1: Error.
 */
int8_t ez8_gen_control_logic(const char *output_dir);

/**
 * @brief Compile an assembly program into a binary file that is intended for CPU execution.
 * @param[in] asm_file_name Path to the assembly program file that will be compiled.
 * @param[in] output_dir Directory path for all output files. If the pointer is NULL, the assembly
 * program directory will be used as output.
 * @retval 0: Success.
 * @retval -1: Error.
 */
int8_t ez8_compile_assembly(const char *asm_file_name, const char *output_dir);

#ifdef __cplusplus
}
#endif

#endif // EZ8_H

/****************************************** END OF FILE *******************************************/
