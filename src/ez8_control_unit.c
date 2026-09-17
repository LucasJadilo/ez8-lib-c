/**
 * @file ez8_control_unit.c
 * @brief This module contains functions to generate the Control Unit's logic.
 * @author Lucas Jadilo
 */

/**************************************************************************************************/
/*  Private Includes                                                                              */
/**************************************************************************************************/

#include "ez8_control_unit.h"
#include "ez8_common.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/**************************************************************************************************/
/*  Private Macros                                                                                */
/**************************************************************************************************/

#define EZ8_CU_EPROM_SIZE   (1U << 12U)
#define EZ8_CU_X            EZ8_CU_BIT_DONT_CARE
#define EZ8_CU_IR_SIZE      5U
#define EZ8_CU_IR_BIT_POS   6U
#define EZ8_CU_IR_BIT_MASK  (~(~0U << EZ8_CU_IR_SIZE))
#define EZ8_CU_IC_SIZE      4U
#define EZ8_CU_IC_BIT_POS   2U
#define EZ8_CU_IC_BIT_MASK  (~(~0U << EZ8_CU_IC_SIZE))
#define EZ8_CU_IC_MAX_VALUE ((1U << EZ8_CU_IC_SIZE) - 1U)

#define WRITE_BITS(var, bits_pos, bits_mask, value)                                                \
    (((var) & ~((bits_mask) << (bits_pos))) | (((value) & (bits_mask)) << (bits_pos)))

#define READ_BITS(var, bits_pos, bits_mask) (((var) >> (bits_pos)) & (bits_mask))

#define TOGGLE_BITS(var, bits_mask)                                                                \
    do {                                                                                           \
        (var) ^= (bits_mask);                                                                      \
    } while (0)

#define EZ8_CU_TRUTH_TABLE(IR, IC, EQUAL, COUT, ...)                                               \
    ez8_cu_add_truth_table(ttable, (IR), (IC), (EQUAL), (COUT), __VA_ARGS__, -1)

#define EZ8_CU_READ_CYCLE_1(IR, IC, EQUAL, COUT, ...)                                              \
    EZ8_CU_TRUTH_TABLE(IR, IC, EQUAL, COUT, EZ8_CU_O_ACO, EZ8_CU_O_ALE, __VA_ARGS__)

#define EZ8_CU_READ_CYCLE_2(IR, IC, EQUAL, COUT, ...)                                              \
    EZ8_CU_TRUTH_TABLE(IR, IC, EQUAL, COUT, EZ8_CU_O_ACO, EZ8_CU_O_DEN, EZ8_CU_O_RD, __VA_ARGS__)

#define EZ8_CU_READ_CYCLE_3(IR, IC, EQUAL, COUT, ...)                                              \
    EZ8_CU_TRUTH_TABLE(IR, IC, EQUAL, COUT, EZ8_CU_O_ACO, EZ8_CU_O_DEN, EZ8_CU_O_RD, __VA_ARGS__)

#define EZ8_CU_READ_CYCLE_4(IR, IC, EQUAL, COUT, ...)                                              \
    EZ8_CU_TRUTH_TABLE(IR, IC, EQUAL, COUT, EZ8_CU_O_ACO, __VA_ARGS__)

#define EZ8_CU_WRITE_CYCLE_1(IR, IC, EQUAL, COUT, ...)                                             \
    EZ8_CU_TRUTH_TABLE(IR, IC, EQUAL, COUT, EZ8_CU_O_ACO, EZ8_CU_O_DI_DO, EZ8_CU_O_ALE, __VA_ARGS__)

#define EZ8_CU_WRITE_CYCLE_2(IR, IC, EQUAL, COUT, ...)                                             \
    EZ8_CU_TRUTH_TABLE(IR, IC, EQUAL, COUT, EZ8_CU_O_ACO, EZ8_CU_O_DI_DO, EZ8_CU_O_DEN,            \
                       EZ8_CU_O_WR, __VA_ARGS__)

#define EZ8_CU_WRITE_CYCLE_3(IR, IC, EQUAL, COUT, ...)                                             \
    EZ8_CU_TRUTH_TABLE(IR, IC, EQUAL, COUT, EZ8_CU_O_ACO, EZ8_CU_O_DI_DO, EZ8_CU_O_DEN, __VA_ARGS__)

#define EZ8_CU_WRITE_CYCLE_4(IR, IC, EQUAL, COUT, ...)                                             \
    EZ8_CU_TRUTH_TABLE(IR, IC, EQUAL, COUT, EZ8_CU_O_ACO, __VA_ARGS__)

/**************************************************************************************************/
/*  Private Types                                                                                 */
/**************************************************************************************************/

typedef enum ez8_cu_logic_level {
    EZ8_CU_BIT_LOW = 0,
    EZ8_CU_BIT_HIGH = 1,
    EZ8_CU_BIT_DONT_CARE = 0xFF
} ez8_cu_logic_level_t;

typedef enum ez8_cu_input_bit_pos {
    EZ8_CU_I_COUT = 0,
    EZ8_CU_I_EQUAL = 1,
    EZ8_CU_I_IC = 2,
    EZ8_CU_I_IR = 6
} ez8_cu_input_bit_pos_t;

typedef enum ez8_cu_output_bit_pos {
    EZ8_CU_O_AI = 0,
    EZ8_CU_O_AO = 1,
    EZ8_CU_O_BI = 2,
    EZ8_CU_O_RO = 3,
    EZ8_CU_O_SHR = 4,
    EZ8_CU_O_S0 = 5,
    EZ8_CU_O_S1 = 6,
    EZ8_CU_O_S2 = 7,
    EZ8_CU_O_S3 = 8,
    EZ8_CU_O_CIN = 9,
    EZ8_CU_O_MODE = 10,
    EZ8_CU_O_IRI = 11,
    EZ8_CU_O_PCI = 12,
    EZ8_CU_O_PCDO = 13,
    EZ8_CU_O_PCAO = 14,
    EZ8_CU_O_PCCE = 15,
    EZ8_CU_O_SPI = 16,
    EZ8_CU_O_SPO = 17,
    EZ8_CU_O_SPCE = 18,
    EZ8_CU_O_SPU_SPD = 19,
    EZ8_CU_O_DARI = 20,
    EZ8_CU_O_DARO = 21,
    EZ8_CU_O_DI_DO = 22,
    EZ8_CU_O_DEN = 23,
    EZ8_CU_O_ACO = 24,
    EZ8_CU_O_ALE = 25,
    EZ8_CU_O_M_IO = 26,
    EZ8_CU_O_RD = 27,
    EZ8_CU_O_WR = 28,
    EZ8_CU_O_ILC = 29
} ez8_cu_output_bit_pos_t;

typedef struct ez8_cu_output {
    const char *name;
    ez8_cu_logic_level_t active;
} ez8_cu_output_t;

/**************************************************************************************************/
/*  Private Variables                                                                             */
/**************************************************************************************************/

static const ez8_cu_output_t outputs[EZ8_CU_OUTPUTS] = {
    [EZ8_CU_O_AI] = {"AI", EZ8_CU_BIT_LOW},
    [EZ8_CU_O_AO] = {"AO", EZ8_CU_BIT_LOW},
    [EZ8_CU_O_BI] = {"BI", EZ8_CU_BIT_LOW},
    [EZ8_CU_O_RO] = {"RO", EZ8_CU_BIT_LOW},
    [EZ8_CU_O_SHR] = {"SHR", EZ8_CU_BIT_LOW},
    [EZ8_CU_O_S0] = {"S0", EZ8_CU_BIT_HIGH},
    [EZ8_CU_O_S1] = {"S1", EZ8_CU_BIT_HIGH},
    [EZ8_CU_O_S2] = {"S2", EZ8_CU_BIT_HIGH},
    [EZ8_CU_O_S3] = {"S3", EZ8_CU_BIT_HIGH},
    [EZ8_CU_O_CIN] = {"CIN", EZ8_CU_BIT_LOW},
    [EZ8_CU_O_MODE] = {"MODE", EZ8_CU_BIT_HIGH},
    [EZ8_CU_O_IRI] = {"IRI", EZ8_CU_BIT_LOW},
    [EZ8_CU_O_PCI] = {"PCI", EZ8_CU_BIT_LOW},
    [EZ8_CU_O_PCDO] = {"PCDO", EZ8_CU_BIT_LOW},
    [EZ8_CU_O_PCAO] = {"PCAO", EZ8_CU_BIT_LOW},
    [EZ8_CU_O_PCCE] = {"PCCE", EZ8_CU_BIT_HIGH},
    [EZ8_CU_O_SPI] = {"SPI", EZ8_CU_BIT_LOW},
    [EZ8_CU_O_SPO] = {"SPO", EZ8_CU_BIT_LOW},
    [EZ8_CU_O_SPCE] = {"SPCE", EZ8_CU_BIT_LOW},
    [EZ8_CU_O_SPU_SPD] = {"SPU/SPD", EZ8_CU_BIT_HIGH},
    [EZ8_CU_O_DARI] = {"DARI", EZ8_CU_BIT_LOW},
    [EZ8_CU_O_DARO] = {"DARO", EZ8_CU_BIT_LOW},
    [EZ8_CU_O_DI_DO] = {"DI/DO", EZ8_CU_BIT_HIGH},
    [EZ8_CU_O_DEN] = {"DEN", EZ8_CU_BIT_LOW},
    [EZ8_CU_O_ACO] = {"ACO", EZ8_CU_BIT_LOW},
    [EZ8_CU_O_ALE] = {"ALE", EZ8_CU_BIT_LOW},
    [EZ8_CU_O_M_IO] = {"M/IO", EZ8_CU_BIT_LOW},
    [EZ8_CU_O_RD] = {"RD", EZ8_CU_BIT_LOW},
    [EZ8_CU_O_WR] = {"WR", EZ8_CU_BIT_LOW},
    [EZ8_CU_O_ILC] = {"ILC", EZ8_CU_BIT_LOW}};

/**************************************************************************************************/
/*  Private Function Declarations                                                                 */
/**************************************************************************************************/

static void ez8_cu_int_to_bin_str(char *bin_str, uint32_t integer, uint8_t n_bits);

static int8_t ez8_cu_add_truth_table(ez8_cu_truth_table_t *ttable, uint8_t ir, uint8_t ic,
                                     ez8_cu_logic_level_t equal, ez8_cu_logic_level_t cout, ...);

/**************************************************************************************************/
/*  Public Function Definitions                                                                   */
/**************************************************************************************************/

void ez8_cu_gen_truth_table(ez8_cu_truth_table_t *ttable)
{
    ttable->n_rows = 0;
    for (unsigned int i = 0; i < EZ8_CU_TRUTH_TABLE_SIZE; i++) {
        ttable->rows[i].in_bits = 0;
        ttable->rows[i].out_bits = 0;
    }

    // Reset
    EZ8_CU_TRUTH_TABLE(EZ8_CU_X, 0, EZ8_CU_X, EZ8_CU_X, -1);

    // Opcode Fetch
    EZ8_CU_READ_CYCLE_1(EZ8_CU_X, 1, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_PCAO);
    EZ8_CU_READ_CYCLE_2(EZ8_CU_X, 2, EZ8_CU_X, EZ8_CU_X, -1);
    EZ8_CU_READ_CYCLE_3(EZ8_CU_X, 3, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_IRI);
    EZ8_CU_READ_CYCLE_4(EZ8_CU_X, 4, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_PCCE);

    // Execution LDA
    EZ8_CU_READ_CYCLE_1(EZ8_OPCODE_LDA, 5, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_PCAO);
    EZ8_CU_READ_CYCLE_2(EZ8_OPCODE_LDA, 6, EZ8_CU_X, EZ8_CU_X, -1);
    EZ8_CU_READ_CYCLE_3(EZ8_OPCODE_LDA, 7, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_DARI);
    EZ8_CU_READ_CYCLE_4(EZ8_OPCODE_LDA, 8, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_PCCE);
    EZ8_CU_READ_CYCLE_1(EZ8_OPCODE_LDA, 9, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_DARO);
    EZ8_CU_READ_CYCLE_2(EZ8_OPCODE_LDA, 10, EZ8_CU_X, EZ8_CU_X, -1);
    EZ8_CU_READ_CYCLE_3(EZ8_OPCODE_LDA, 11, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_AI);
    EZ8_CU_READ_CYCLE_4(EZ8_OPCODE_LDA, 12, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_ILC);

    // Execution LDI
    EZ8_CU_READ_CYCLE_1(EZ8_OPCODE_LDI, 5, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_PCAO);
    EZ8_CU_READ_CYCLE_2(EZ8_OPCODE_LDI, 6, EZ8_CU_X, EZ8_CU_X, -1);
    EZ8_CU_READ_CYCLE_3(EZ8_OPCODE_LDI, 7, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_AI);
    EZ8_CU_READ_CYCLE_4(EZ8_OPCODE_LDI, 8, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_PCCE, EZ8_CU_O_ILC);

    // Execution STA
    EZ8_CU_READ_CYCLE_1(EZ8_OPCODE_STA, 5, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_PCAO);
    EZ8_CU_READ_CYCLE_2(EZ8_OPCODE_STA, 6, EZ8_CU_X, EZ8_CU_X, -1);
    EZ8_CU_READ_CYCLE_3(EZ8_OPCODE_STA, 7, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_DARI);
    EZ8_CU_READ_CYCLE_4(EZ8_OPCODE_STA, 8, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_PCCE);
    EZ8_CU_WRITE_CYCLE_1(EZ8_OPCODE_STA, 9, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_DARO);
    EZ8_CU_WRITE_CYCLE_2(EZ8_OPCODE_STA, 10, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_AO);
    EZ8_CU_WRITE_CYCLE_3(EZ8_OPCODE_STA, 11, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_AO);
    EZ8_CU_WRITE_CYCLE_4(EZ8_OPCODE_STA, 12, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_ILC);

    // Execution LIA
    EZ8_CU_TRUTH_TABLE(EZ8_OPCODE_LIA, 5, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_ACO, EZ8_CU_O_AO,
                       EZ8_CU_O_DARI);
    EZ8_CU_READ_CYCLE_1(EZ8_OPCODE_LIA, 6, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_DARO);
    EZ8_CU_READ_CYCLE_2(EZ8_OPCODE_LIA, 7, EZ8_CU_X, EZ8_CU_X, -1);
    EZ8_CU_READ_CYCLE_3(EZ8_OPCODE_LIA, 8, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_AI);
    EZ8_CU_READ_CYCLE_4(EZ8_OPCODE_LIA, 9, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_ILC);

    // Execution NOT
    EZ8_CU_TRUTH_TABLE(EZ8_OPCODE_NOT, 5, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_ACO, EZ8_CU_O_MODE,
                       EZ8_CU_O_RO, EZ8_CU_O_AI, EZ8_CU_O_ILC);

    // Execution AND
    EZ8_CU_READ_CYCLE_1(EZ8_OPCODE_AND, 5, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_PCAO);
    EZ8_CU_READ_CYCLE_2(EZ8_OPCODE_AND, 6, EZ8_CU_X, EZ8_CU_X, -1);
    EZ8_CU_READ_CYCLE_3(EZ8_OPCODE_AND, 7, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_DARI);
    EZ8_CU_READ_CYCLE_4(EZ8_OPCODE_AND, 8, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_PCCE);
    EZ8_CU_READ_CYCLE_1(EZ8_OPCODE_AND, 9, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_DARO);
    EZ8_CU_READ_CYCLE_2(EZ8_OPCODE_AND, 10, EZ8_CU_X, EZ8_CU_X, -1);
    EZ8_CU_READ_CYCLE_3(EZ8_OPCODE_AND, 11, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_BI);
    EZ8_CU_READ_CYCLE_4(EZ8_OPCODE_AND, 12, EZ8_CU_X, EZ8_CU_X, -1);
    EZ8_CU_TRUTH_TABLE(EZ8_OPCODE_AND, 13, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_ACO, EZ8_CU_O_MODE,
                       EZ8_CU_O_S3, EZ8_CU_O_S1, EZ8_CU_O_S0, EZ8_CU_O_RO, EZ8_CU_O_AI,
                       EZ8_CU_O_ILC);

    // Execution OR
    EZ8_CU_READ_CYCLE_1(EZ8_OPCODE_OR, 5, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_PCAO);
    EZ8_CU_READ_CYCLE_2(EZ8_OPCODE_OR, 6, EZ8_CU_X, EZ8_CU_X, -1);
    EZ8_CU_READ_CYCLE_3(EZ8_OPCODE_OR, 7, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_DARI);
    EZ8_CU_READ_CYCLE_4(EZ8_OPCODE_OR, 8, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_PCCE);
    EZ8_CU_READ_CYCLE_1(EZ8_OPCODE_OR, 9, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_DARO);
    EZ8_CU_READ_CYCLE_2(EZ8_OPCODE_OR, 10, EZ8_CU_X, EZ8_CU_X, -1);
    EZ8_CU_READ_CYCLE_3(EZ8_OPCODE_OR, 11, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_BI);
    EZ8_CU_READ_CYCLE_4(EZ8_OPCODE_OR, 12, EZ8_CU_X, EZ8_CU_X, -1);
    EZ8_CU_TRUTH_TABLE(EZ8_OPCODE_OR, 13, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_ACO, EZ8_CU_O_MODE,
                       EZ8_CU_O_S3, EZ8_CU_O_S2, EZ8_CU_O_S1, EZ8_CU_O_RO, EZ8_CU_O_AI,
                       EZ8_CU_O_ILC);

    // Execution XOR
    EZ8_CU_READ_CYCLE_1(EZ8_OPCODE_XOR, 5, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_PCAO);
    EZ8_CU_READ_CYCLE_2(EZ8_OPCODE_XOR, 6, EZ8_CU_X, EZ8_CU_X, -1);
    EZ8_CU_READ_CYCLE_3(EZ8_OPCODE_XOR, 7, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_DARI);
    EZ8_CU_READ_CYCLE_4(EZ8_OPCODE_XOR, 8, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_PCCE);
    EZ8_CU_READ_CYCLE_1(EZ8_OPCODE_XOR, 9, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_DARO);
    EZ8_CU_READ_CYCLE_2(EZ8_OPCODE_XOR, 10, EZ8_CU_X, EZ8_CU_X, -1);
    EZ8_CU_READ_CYCLE_3(EZ8_OPCODE_XOR, 11, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_BI);
    EZ8_CU_READ_CYCLE_4(EZ8_OPCODE_XOR, 12, EZ8_CU_X, EZ8_CU_X, -1);
    EZ8_CU_TRUTH_TABLE(EZ8_OPCODE_XOR, 13, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_ACO, EZ8_CU_O_MODE,
                       EZ8_CU_O_S2, EZ8_CU_O_S1, EZ8_CU_O_RO, EZ8_CU_O_AI, EZ8_CU_O_ILC);

    // Execution SHL
    EZ8_CU_TRUTH_TABLE(EZ8_OPCODE_SHL, 5, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_ACO, EZ8_CU_O_S3,
                       EZ8_CU_O_S2, EZ8_CU_O_RO, EZ8_CU_O_AI, EZ8_CU_O_ILC);

    // Execution SHR
    EZ8_CU_TRUTH_TABLE(EZ8_OPCODE_SHR, 5, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_ACO, EZ8_CU_O_SHR,
                       EZ8_CU_O_AI, EZ8_CU_O_ILC);

    // Execution ADD
    EZ8_CU_READ_CYCLE_1(EZ8_OPCODE_ADD, 5, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_PCAO);
    EZ8_CU_READ_CYCLE_2(EZ8_OPCODE_ADD, 6, EZ8_CU_X, EZ8_CU_X, -1);
    EZ8_CU_READ_CYCLE_3(EZ8_OPCODE_ADD, 7, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_DARI);
    EZ8_CU_READ_CYCLE_4(EZ8_OPCODE_ADD, 8, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_PCCE);
    EZ8_CU_READ_CYCLE_1(EZ8_OPCODE_ADD, 9, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_DARO);
    EZ8_CU_READ_CYCLE_2(EZ8_OPCODE_ADD, 10, EZ8_CU_X, EZ8_CU_X, -1);
    EZ8_CU_READ_CYCLE_3(EZ8_OPCODE_ADD, 11, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_BI);
    EZ8_CU_READ_CYCLE_4(EZ8_OPCODE_ADD, 12, EZ8_CU_X, EZ8_CU_X, -1);
    EZ8_CU_TRUTH_TABLE(EZ8_OPCODE_ADD, 13, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_ACO, EZ8_CU_O_S3,
                       EZ8_CU_O_S0, EZ8_CU_O_RO, EZ8_CU_O_AI, EZ8_CU_O_ILC);

    // Execution SUB
    EZ8_CU_READ_CYCLE_1(EZ8_OPCODE_SUB, 5, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_PCAO);
    EZ8_CU_READ_CYCLE_2(EZ8_OPCODE_SUB, 6, EZ8_CU_X, EZ8_CU_X, -1);
    EZ8_CU_READ_CYCLE_3(EZ8_OPCODE_SUB, 7, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_DARI);
    EZ8_CU_READ_CYCLE_4(EZ8_OPCODE_SUB, 8, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_PCCE);
    EZ8_CU_READ_CYCLE_1(EZ8_OPCODE_SUB, 9, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_DARO);
    EZ8_CU_READ_CYCLE_2(EZ8_OPCODE_SUB, 10, EZ8_CU_X, EZ8_CU_X, -1);
    EZ8_CU_READ_CYCLE_3(EZ8_OPCODE_SUB, 11, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_BI);
    EZ8_CU_READ_CYCLE_4(EZ8_OPCODE_SUB, 12, EZ8_CU_X, EZ8_CU_X, -1);
    EZ8_CU_TRUTH_TABLE(EZ8_OPCODE_SUB, 13, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_ACO, EZ8_CU_O_CIN,
                       EZ8_CU_O_S2, EZ8_CU_O_S1, EZ8_CU_O_RO, EZ8_CU_O_AI, EZ8_CU_O_ILC);

    // Execution CMP
    EZ8_CU_READ_CYCLE_1(EZ8_OPCODE_CMP, 5, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_PCAO);
    EZ8_CU_READ_CYCLE_2(EZ8_OPCODE_CMP, 6, EZ8_CU_X, EZ8_CU_X, -1);
    EZ8_CU_READ_CYCLE_3(EZ8_OPCODE_CMP, 7, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_DARI);
    EZ8_CU_READ_CYCLE_4(EZ8_OPCODE_CMP, 8, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_PCCE);
    EZ8_CU_READ_CYCLE_1(EZ8_OPCODE_CMP, 9, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_DARO);
    EZ8_CU_READ_CYCLE_2(EZ8_OPCODE_CMP, 10, EZ8_CU_X, EZ8_CU_X, -1);
    EZ8_CU_READ_CYCLE_3(EZ8_OPCODE_CMP, 11, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_BI);
    EZ8_CU_READ_CYCLE_4(EZ8_OPCODE_CMP, 12, EZ8_CU_X, EZ8_CU_X, -1);
    EZ8_CU_TRUTH_TABLE(EZ8_OPCODE_CMP, 13, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_ACO, EZ8_CU_O_S2,
                       EZ8_CU_O_S1, EZ8_CU_O_RO, EZ8_CU_O_ILC);

    // Execution JMP
    EZ8_CU_READ_CYCLE_1(EZ8_OPCODE_JMP, 5, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_PCAO);
    EZ8_CU_READ_CYCLE_2(EZ8_OPCODE_JMP, 6, EZ8_CU_X, EZ8_CU_X, -1);
    EZ8_CU_READ_CYCLE_3(EZ8_OPCODE_JMP, 7, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_PCI);
    EZ8_CU_READ_CYCLE_4(EZ8_OPCODE_JMP, 8, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_ILC);

    // Execution JPE
    EZ8_CU_READ_CYCLE_1(EZ8_OPCODE_JPE, 5, 1, 1, EZ8_CU_O_PCAO);
    EZ8_CU_READ_CYCLE_2(EZ8_OPCODE_JPE, 6, 1, 1, -1);
    EZ8_CU_READ_CYCLE_3(EZ8_OPCODE_JPE, 7, 1, 1, EZ8_CU_O_PCI);
    EZ8_CU_READ_CYCLE_4(EZ8_OPCODE_JPE, 8, 1, 1, EZ8_CU_O_ILC);
    EZ8_CU_TRUTH_TABLE(EZ8_OPCODE_JPE, 5, 0, EZ8_CU_X, EZ8_CU_O_ACO, EZ8_CU_O_PCCE, EZ8_CU_O_ILC);
    EZ8_CU_TRUTH_TABLE(EZ8_OPCODE_JPE, 5, EZ8_CU_X, 0, EZ8_CU_O_ACO, EZ8_CU_O_PCCE, EZ8_CU_O_ILC);

    // Execution JPA
    EZ8_CU_READ_CYCLE_1(EZ8_OPCODE_JPA, 5, 0, 0, EZ8_CU_O_PCAO);
    EZ8_CU_READ_CYCLE_2(EZ8_OPCODE_JPA, 6, 0, 0, -1);
    EZ8_CU_READ_CYCLE_3(EZ8_OPCODE_JPA, 7, 0, 0, EZ8_CU_O_PCI);
    EZ8_CU_READ_CYCLE_4(EZ8_OPCODE_JPA, 8, 0, 0, EZ8_CU_O_ILC);
    EZ8_CU_TRUTH_TABLE(EZ8_OPCODE_JPA, 5, 1, EZ8_CU_X, EZ8_CU_O_ACO, EZ8_CU_O_PCCE, EZ8_CU_O_ILC);
    EZ8_CU_TRUTH_TABLE(EZ8_OPCODE_JPA, 5, EZ8_CU_X, 1, EZ8_CU_O_ACO, EZ8_CU_O_PCCE, EZ8_CU_O_ILC);

    // Execution JPB
    EZ8_CU_READ_CYCLE_1(EZ8_OPCODE_JPB, 5, 0, 1, EZ8_CU_O_PCAO);
    EZ8_CU_READ_CYCLE_2(EZ8_OPCODE_JPB, 6, 0, 1, -1);
    EZ8_CU_READ_CYCLE_3(EZ8_OPCODE_JPB, 7, 0, 1, EZ8_CU_O_PCI);
    EZ8_CU_READ_CYCLE_4(EZ8_OPCODE_JPB, 8, 0, 1, EZ8_CU_O_ILC);
    EZ8_CU_TRUTH_TABLE(EZ8_OPCODE_JPB, 5, 1, EZ8_CU_X, EZ8_CU_O_ACO, EZ8_CU_O_PCCE, EZ8_CU_O_ILC);
    EZ8_CU_TRUTH_TABLE(EZ8_OPCODE_JPB, 5, 0, 0, EZ8_CU_O_ACO, EZ8_CU_O_PCCE, EZ8_CU_O_ILC);

    // Execution CALL
    EZ8_CU_TRUTH_TABLE(EZ8_OPCODE_CALL, 5, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_ACO, EZ8_CU_O_PCDO,
                       EZ8_CU_O_DARI, EZ8_CU_O_PCCE);
    EZ8_CU_WRITE_CYCLE_1(EZ8_OPCODE_CALL, 6, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_SPO);
    EZ8_CU_WRITE_CYCLE_2(EZ8_OPCODE_CALL, 7, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_PCDO);
    EZ8_CU_WRITE_CYCLE_3(EZ8_OPCODE_CALL, 8, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_PCDO);
    EZ8_CU_WRITE_CYCLE_4(EZ8_OPCODE_CALL, 9, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_SPCE);
    EZ8_CU_READ_CYCLE_1(EZ8_OPCODE_CALL, 10, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_DARO);
    EZ8_CU_READ_CYCLE_2(EZ8_OPCODE_CALL, 11, EZ8_CU_X, EZ8_CU_X, -1);
    EZ8_CU_READ_CYCLE_3(EZ8_OPCODE_CALL, 12, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_PCI);
    EZ8_CU_READ_CYCLE_4(EZ8_OPCODE_CALL, 13, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_ILC);

    // Execution RET
    EZ8_CU_TRUTH_TABLE(EZ8_OPCODE_RET, 5, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_ACO, EZ8_CU_O_SPU_SPD,
                       EZ8_CU_O_SPCE);
    EZ8_CU_READ_CYCLE_1(EZ8_OPCODE_RET, 6, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_SPO);
    EZ8_CU_READ_CYCLE_2(EZ8_OPCODE_RET, 7, EZ8_CU_X, EZ8_CU_X, -1);
    EZ8_CU_READ_CYCLE_3(EZ8_OPCODE_RET, 8, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_PCI);
    EZ8_CU_READ_CYCLE_4(EZ8_OPCODE_RET, 9, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_ILC);

    // Execution LSP
    EZ8_CU_TRUTH_TABLE(EZ8_OPCODE_LSP, 5, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_ACO, EZ8_CU_O_AO,
                       EZ8_CU_O_SPI, EZ8_CU_O_ILC);

    // Execution PUSH
    EZ8_CU_WRITE_CYCLE_1(EZ8_OPCODE_PUSH, 5, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_SPO);
    EZ8_CU_WRITE_CYCLE_2(EZ8_OPCODE_PUSH, 6, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_AO);
    EZ8_CU_WRITE_CYCLE_3(EZ8_OPCODE_PUSH, 7, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_AO);
    EZ8_CU_WRITE_CYCLE_4(EZ8_OPCODE_PUSH, 8, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_SPCE, EZ8_CU_O_ILC);

    // Execution POP
    EZ8_CU_TRUTH_TABLE(EZ8_OPCODE_POP, 5, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_ACO, EZ8_CU_O_SPU_SPD,
                       EZ8_CU_O_SPCE);
    EZ8_CU_READ_CYCLE_1(EZ8_OPCODE_POP, 6, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_SPO);
    EZ8_CU_READ_CYCLE_2(EZ8_OPCODE_POP, 7, EZ8_CU_X, EZ8_CU_X, -1);
    EZ8_CU_READ_CYCLE_3(EZ8_OPCODE_POP, 8, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_AI);
    EZ8_CU_READ_CYCLE_4(EZ8_OPCODE_POP, 9, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_ILC);

    // Execution IN
    EZ8_CU_READ_CYCLE_1(EZ8_OPCODE_IN, 5, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_PCAO);
    EZ8_CU_READ_CYCLE_2(EZ8_OPCODE_IN, 6, EZ8_CU_X, EZ8_CU_X, -1);
    EZ8_CU_READ_CYCLE_3(EZ8_OPCODE_IN, 7, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_DARI);
    EZ8_CU_READ_CYCLE_4(EZ8_OPCODE_IN, 8, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_PCCE);
    EZ8_CU_READ_CYCLE_1(EZ8_OPCODE_IN, 9, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_M_IO, EZ8_CU_O_DARO);
    EZ8_CU_READ_CYCLE_2(EZ8_OPCODE_IN, 10, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_M_IO);
    EZ8_CU_READ_CYCLE_3(EZ8_OPCODE_IN, 11, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_M_IO, EZ8_CU_O_AI);
    EZ8_CU_READ_CYCLE_4(EZ8_OPCODE_IN, 12, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_M_IO, EZ8_CU_O_ILC);

    // Execution OUT
    EZ8_CU_READ_CYCLE_1(EZ8_OPCODE_OUT, 5, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_PCAO);
    EZ8_CU_READ_CYCLE_2(EZ8_OPCODE_OUT, 6, EZ8_CU_X, EZ8_CU_X, -1);
    EZ8_CU_READ_CYCLE_3(EZ8_OPCODE_OUT, 7, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_DARI);
    EZ8_CU_READ_CYCLE_4(EZ8_OPCODE_OUT, 8, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_PCCE);
    EZ8_CU_WRITE_CYCLE_1(EZ8_OPCODE_OUT, 9, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_M_IO, EZ8_CU_O_DARO);
    EZ8_CU_WRITE_CYCLE_2(EZ8_OPCODE_OUT, 10, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_M_IO, EZ8_CU_O_AO);
    EZ8_CU_WRITE_CYCLE_3(EZ8_OPCODE_OUT, 11, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_M_IO, EZ8_CU_O_AO);
    EZ8_CU_WRITE_CYCLE_4(EZ8_OPCODE_OUT, 12, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_M_IO, EZ8_CU_O_ILC);

    // Execution NOP
    EZ8_CU_TRUTH_TABLE(EZ8_OPCODE_NOP, 4, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_ACO, EZ8_CU_O_PCCE,
                       EZ8_CU_O_ILC);

    // Execution HALT
    EZ8_CU_TRUTH_TABLE(EZ8_OPCODE_HALT, 1, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_ACO, EZ8_CU_O_ILC);
    EZ8_CU_TRUTH_TABLE(EZ8_OPCODE_HALT, 4, EZ8_CU_X, EZ8_CU_X, EZ8_CU_O_ACO, EZ8_CU_O_ILC);
}

int8_t ez8_cu_gen_truth_table_csv(const ez8_cu_truth_table_t *ttable, const char *csv_file_name)
{
    printf("Generating CSV file for the CU's truth table\n");

    uint32_t default_outputs = 0;

    for (unsigned int i = 0; i < EZ8_CU_OUTPUTS; i++) {
        default_outputs = WRITE_BITS(default_outputs, i, 1U, ~outputs[i].active);
    }

    uint32_t sorted_outputs[EZ8_CU_TRUTH_TABLE_SIZE] = {0};

    for (unsigned int i = 0; i < EZ8_CU_TRUTH_TABLE_SIZE; i++) {
        sorted_outputs[i] = default_outputs;
    }

    for (unsigned int i = 0; i < ttable->n_rows; i++) {
        sorted_outputs[ttable->rows[i].in_bits] = ttable->rows[i].out_bits;
    }

    OPEN_FILE(file, csv_file_name, "w");

    PRINT_FILE(file, csv_file_name, "Inputs, Outputs   , Mnemonic, IR, IC, EQUAL, COUT");
    for (int i = EZ8_CU_OUTPUTS - 1; i >= 0; i--) {
        PRINT_FILE(file, csv_file_name, ", %s", outputs[i].name);
    }
    PRINT_FILE(file, csv_file_name, "\n");

    for (unsigned int inputs = 0; inputs < EZ8_CU_TRUTH_TABLE_SIZE; inputs++) {
        const uint8_t ir_bits = READ_BITS(inputs, EZ8_CU_I_IR, EZ8_CU_IR_BIT_MASK);
        char ir_str[EZ8_CU_IR_SIZE + 1];
        ez8_cu_int_to_bin_str(ir_str, ir_bits, EZ8_CU_IR_SIZE);

        const uint8_t ic_bits = READ_BITS(inputs, EZ8_CU_I_IC, EZ8_CU_IC_BIT_MASK);
        char ic_str[EZ8_CU_IC_SIZE + 1];
        ez8_cu_int_to_bin_str(ic_str, ic_bits, EZ8_CU_IC_SIZE);

        PRINT_FILE(file, csv_file_name, "0x%03X , 0x%08X, %-8s, %-2u, %-2u, %-5u, %-4u", inputs,
                   sorted_outputs[inputs],
                   (ir_bits < EZ8_INSTRUCTIONS) ? instructions[ir_bits].mnemonic : "-", ir_bits,
                   ic_bits, READ_BITS(inputs, EZ8_CU_I_EQUAL, 1U),
                   READ_BITS(inputs, EZ8_CU_I_COUT, 1U));

        for (int j = EZ8_CU_OUTPUTS - 1; j >= 0; j--) {
            const ez8_cu_logic_level_t out_bit = READ_BITS(sorted_outputs[inputs], j, 1U);
            char out_bit_str[3] = {0};
            out_bit_str[0] = (char)out_bit + '0';
            out_bit_str[1] = (out_bit == outputs[j].active) ? '*' : ' ';
            PRINT_FILE(file, csv_file_name, ", %-*s", (int)strlen(outputs[j].name), out_bit_str);
        }

        PRINT_FILE(file, csv_file_name, "\n");
    }

    fclose(file);
    printf("File `%s` generated successfully\n", csv_file_name);
    return 0;
}

int8_t ez8_cu_gen_eprom_file(const ez8_cu_truth_table_t *ttable, const uint8_t eprom_index,
                             const char *bin_file_name)
{
    printf("Generating binary file for CU's EPROM %u (%u bytes)\n", eprom_index, EZ8_CU_EPROM_SIZE);

    uint8_t eprom[EZ8_CU_EPROM_SIZE];
    uint32_t default_outputs = 0;

    for (unsigned int i = 0; i < EZ8_CU_OUTPUTS; i++) {
        default_outputs = WRITE_BITS(default_outputs, i, 1U, ~outputs[i].active);
    }

    for (unsigned int i = 0; i < EZ8_CU_EPROM_SIZE; i++) {
        eprom[i] = (uint8_t)default_outputs;
    }

    const uint8_t first_output = eprom_index * 8;

    for (unsigned int i = 0; i < ttable->n_rows; i++) {
        eprom[ttable->rows[i].in_bits] = (uint8_t)(ttable->rows[i].out_bits >> first_output);
    }

    OPEN_FILE(file, bin_file_name, "wb");

    if (fwrite(eprom, sizeof(eprom[0]), EZ8_CU_EPROM_SIZE, file) != EZ8_CU_EPROM_SIZE) {
        EZ8_ERROR("Unable to write file `%s`", bin_file_name);
        return -1;
    }

    fclose(file);
    printf("File `%s` generated successfully\n", bin_file_name);
    return 0;
}

int8_t ez8_cu_gen_instruction_cycles_csv(const ez8_cu_truth_table_t *ttable,
                                         const char *csv_file_name)
{
    printf("Generating CSV file for instruction cycles\n");

    OPEN_FILE(file, csv_file_name, "w");

    for (unsigned int opcode = 0; opcode < EZ8_INSTRUCTIONS; opcode++) {
        PRINT_FILE(file, csv_file_name, "\n\n%s:", instructions[opcode].mnemonic);

        for (unsigned int cycle = 1; cycle < EZ8_CU_IC_MAX_VALUE; cycle++) {
            uint32_t last_outputs = 0;

            for (unsigned int i = 0; i < ttable->n_rows; i++) {
                const uint8_t ir_bits =
                    READ_BITS(ttable->rows[i].in_bits, EZ8_CU_IR_BIT_POS, EZ8_CU_IR_BIT_MASK);
                const uint8_t ic_bits =
                    READ_BITS(ttable->rows[i].in_bits, EZ8_CU_IC_BIT_POS, EZ8_CU_IC_BIT_MASK);

                if ((ir_bits == opcode) && (ic_bits == cycle)) {
                    if (ttable->rows[i].out_bits != last_outputs) {
                        last_outputs = ttable->rows[i].out_bits;
                        PRINT_FILE(file, csv_file_name, "\n%2u; 0x%08X;", cycle,
                                   ttable->rows[i].out_bits);
                    } else {
                        PRINT_FILE(file, csv_file_name, ",");
                    }
                    PRINT_FILE(file, csv_file_name, " 0x%03X", ttable->rows[i].in_bits);
                }
            }
        }
    }

    fclose(file);
    printf("File `%s` generated successfully\n", csv_file_name);
    return 0;
}

/**************************************************************************************************/
/*  Private Function Definitions                                                                  */
/**************************************************************************************************/

/**
 * @brief Convert an integer to a binary ASCII string.
 * @param[out] bin_str Resulting string with the binary value.
 * @param[in] integer Integer to be converted.
 * @param[in] n_bits Number of bits used in the string (padding bits '0' to the left).
 */
static void ez8_cu_int_to_bin_str(char *bin_str, const uint32_t integer, const uint8_t n_bits)
{
    for (unsigned int i = 0; i < n_bits; i++) {
        bin_str[i] = ((integer >> (n_bits - 1 - i)) & 1U) + '0';
    }
    bin_str[n_bits] = '\0';
}

/**
 * @brief Add an input-output control combination into the truth table.
 * @param[in,out] ttable Struct that stores all the table's input-output combinations.
 * @param[in] ir Instruction register bits (control input).
 * @param[in] ic Instruction counter bits (control input).
 * @param[in] equal ALU's EQUAL flag (control input).
 * @param[in] cout ALU's COUT flag (control input).
 * @param[in] ... List of active control outputs.
 */
static int8_t ez8_cu_add_truth_table(ez8_cu_truth_table_t *ttable, const uint8_t ir,
                                     const uint8_t ic, const ez8_cu_logic_level_t equal,
                                     const ez8_cu_logic_level_t cout, ...)
{
    // Initialize output control bits to default values (inactive logic level)
    for (unsigned int i = 0; i < EZ8_CU_OUTPUTS; i++) {
        ttable->rows[ttable->n_rows].out_bits =
            WRITE_BITS(ttable->rows[ttable->n_rows].out_bits, i, 1U, ~outputs[i].active);
    }

    // Receive output control bits and insert them into the truth table
    va_list args;
    va_start(args, cout);
    int arg = va_arg(args, int);
    while (arg >= 0) {
        ttable->rows[ttable->n_rows].out_bits =
            WRITE_BITS(ttable->rows[ttable->n_rows].out_bits, arg, 1U, outputs[arg].active);
        arg = va_arg(args, int);
    }
    va_end(args);

    // Concatenate the input bits into the inputs array, considering don't care bits
    uint8_t in_bits[EZ8_CU_INPUTS] = {0};
    uint8_t n_dont_care_bits = 0;

    if (ir == EZ8_CU_BIT_DONT_CARE) {
        for (unsigned int i = 0; i < EZ8_CU_IR_SIZE; i++) {
            in_bits[EZ8_CU_I_IR + i] = EZ8_CU_BIT_DONT_CARE;
        }
        n_dont_care_bits += EZ8_CU_IR_SIZE;
    } else {
        for (unsigned int i = 0; i < EZ8_CU_IR_SIZE; i++) {
            in_bits[EZ8_CU_I_IR + i] = READ_BITS(ir, i, 1U);
        }
    }

    if (ic == EZ8_CU_BIT_DONT_CARE) {
        for (unsigned int i = 0; i < EZ8_CU_IC_SIZE; i++) {
            in_bits[EZ8_CU_I_IC + i] = EZ8_CU_BIT_DONT_CARE;
        }
        n_dont_care_bits += EZ8_CU_IC_SIZE;
    } else {
        for (unsigned int i = 0; i < EZ8_CU_IC_SIZE; i++) {
            in_bits[EZ8_CU_I_IC + i] = READ_BITS(ic, i, 1U);
        }
    }

    in_bits[EZ8_CU_I_EQUAL] = equal;
    if (equal == EZ8_CU_BIT_DONT_CARE) {
        n_dont_care_bits++;
    }

    in_bits[EZ8_CU_I_COUT] = cout;
    if (cout == EZ8_CU_BIT_DONT_CARE) {
        n_dont_care_bits++;
    }

    // Calculate the number of combinations to be added, considering don't care bits
    uint16_t new_combs = 1 << n_dont_care_bits;

    if ((ttable->n_rows + new_combs) <= EZ8_CU_TRUTH_TABLE_SIZE) {
        unsigned int first_dont_care_pos = 0xFF;

        // Find index of the first don't care bit
        for (unsigned int i = 0; i < EZ8_CU_INPUTS; i++) {
            if (in_bits[i] == EZ8_CU_BIT_DONT_CARE) {
                if (first_dont_care_pos == 0xFF) {
                    first_dont_care_pos = i;
                }
                // Initialize don't care bits with zero
                ttable->rows[ttable->n_rows].in_bits = (uint16_t)WRITE_BITS(
                    ttable->rows[ttable->n_rows].in_bits, i, 1U, EZ8_CU_BIT_LOW);
            } else {
                // Initialize non don't care bits with arguments passed
                ttable->rows[ttable->n_rows].in_bits =
                    (uint16_t)WRITE_BITS(ttable->rows[ttable->n_rows].in_bits, i, 1U, in_bits[i]);
            }
        }

        while (new_combs--) {
            ttable->n_rows++;

            // Copy the bits from the last control combination to the current one
            ttable->rows[ttable->n_rows].in_bits = ttable->rows[ttable->n_rows - 1].in_bits;
            ttable->rows[ttable->n_rows].out_bits = ttable->rows[ttable->n_rows - 1].out_bits;

            // Create a new combination for the don't care bits
            unsigned int next_dont_care_pos = first_dont_care_pos;

            while (next_dont_care_pos != 0xFF) {
                // Toggle the first don't care bit
                TOGGLE_BITS(ttable->rows[ttable->n_rows].in_bits, 1 << next_dont_care_pos);

                if (READ_BITS(ttable->rows[ttable->n_rows].in_bits, next_dont_care_pos, 1U) ==
                    EZ8_CU_BIT_LOW) {
                    // If the bit changes to LOW, find the next don't care bit
                    const unsigned int last_pos = next_dont_care_pos;
                    for (unsigned int i = 0; i < EZ8_CU_INPUTS; i++) {
                        if ((in_bits[i] == EZ8_CU_BIT_DONT_CARE) && (i > next_dont_care_pos)) {
                            next_dont_care_pos = i;
                            break;
                        }
                    }

                    if (last_pos == next_dont_care_pos) {
                        // There are no more don't care bits
                        next_dont_care_pos = 0xFF;
                    }
                } else {
                    next_dont_care_pos = 0xFF;
                }
            }
        }
    } else {
        EZ8_ERROR("Exceeded the maximum number of combinations");
        return -1;
    }
    return 0;
}

/****************************************** END OF FILE *******************************************/
