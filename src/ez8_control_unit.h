/**
 * @file ez8_control_unit.h
 * @brief This module contains functions to generate the Control Unit's logic.
 * @author Lucas Jadilo
 */

#ifndef EZ8_CONTROL_UNIT_H
#define EZ8_CONTROL_UNIT_H

/**************************************************************************************************/
/*  Public Includes                                                                               */
/**************************************************************************************************/

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************/
/*  Public Macros                                                                                 */
/**************************************************************************************************/

#define EZ8_CU_INPUTS           11U
#define EZ8_CU_OUTPUTS          30U
#define EZ8_CU_TRUTH_TABLE_SIZE (1U << EZ8_CU_INPUTS)

/**************************************************************************************************/
/*  Public Types                                                                                  */
/**************************************************************************************************/

typedef struct ez8_cu_truth_table_row {
    uint16_t in_bits;
    uint32_t out_bits;
} ez8_cu_truth_table_row_t;

typedef struct ez8_cu_truth_table {
    ez8_cu_truth_table_row_t rows[EZ8_CU_TRUTH_TABLE_SIZE];
    uint16_t n_rows;
} ez8_cu_truth_table_t;

/**************************************************************************************************/
/*  Public Function Declarations                                                                  */
/**************************************************************************************************/

/**
 * @brief Generate the Control Unit's truth table.
 * @param[out] ttable Struct that will store all the table's input-output combinations.
 */
void ez8_cu_gen_truth_table(ez8_cu_truth_table_t *ttable);

/**
 * @brief Generate a CSV file with the Control Unit's truth table.
 * @param[in] ttable Struct that stores all the table's input-output combinations.
 * @param[in] csv_file_name Path string to the CSV file that will be created.
 * @retval 0: Success.
 * @retval -1: Error.
 */
int8_t ez8_cu_gen_truth_table_csv(const ez8_cu_truth_table_t *ttable, const char *csv_file_name);

/**
 * @brief Generate an EPROM binary file based on the Control Unit's truth table. Each EPROM holds an
 * 8-bit portion of every combination based on the EPROM index.
 * @param[in] ttable Struct that stores all the table's input-output combinations.
 * @param[in] eprom_index EPROM index. EPROM 0 has output bits 0-7, EPROM 1 has bits 8-15, ...
 * @param[in] bin_file_name Path string to the binary file that will be created.
 * @retval 0: Success.
 * @retval -1: Error.
 */
int8_t ez8_cu_gen_eprom_file(const ez8_cu_truth_table_t *ttable, uint8_t eprom_index,
                             const char *bin_file_name);

/**
 * @brief Print the input-output control combinations of every cycle for each instruction.
 * @param[in] ttable Struct that stores all the table's input-output combinations.
 * @param[in] csv_file_name Path string to the CSV file that will be created.
 * @retval 0: Success.
 * @retval -1: Error.
 */
int8_t ez8_cu_gen_instruction_cycles_csv(const ez8_cu_truth_table_t *ttable,
                                         const char *csv_file_name);

#ifdef __cplusplus
}
#endif

#endif // EZ8_CONTROL_UNIT_H

/****************************************** END OF FILE *******************************************/
