/**
 * Copyright 2022 University of Adelaide
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*implements functions for tokenizing a preprocessed assembly instruction
 * string*/
#include "tokenizer.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "instr_parser.h"
#include "reg_parser.h"

#ifdef _MSC_VER
#include <tchar.h>
#define strtok_r strtok_s
#endif


/**
 * sets @param len number of characters to ' ' for @param str
 */
static inline void
clearstring(char* str, int len)
{
    for ( int i = 0; i < len; i++ )
        str[i] = ' ';
}

/**
 * Given an instance of @param instr_buffer and the sign of memory offset @param
 * neg, sets the 'm' displacement value for modRM32_64[m][r] table
 */
static inline void
get_mod_disp(struct instr* instr_buffer, bool neg)
{
    // set modRM_64[i][j] 'i' displacement value depending on the range of
    // memory_offset
    uint32_t memory_offset = instr_buffer->mem_offset;
    if ( memory_offset == 0 )
        instr_buffer->mod_disp = 0;
    else if ( !neg )
    {
        if ( IN_RANGE(memory_offset, 1, MAX_SIGNED_8BIT) )
            instr_buffer->mod_disp &= MOD8;
        else if ( memory_offset > MAX_SIGNED_8BIT )
            instr_buffer->mod_disp &= MOD16;
    }
    else
    {
        if ( IN_RANGE(memory_offset, 1, MAX_UNSIGNED_8BIT) )
            instr_buffer->mod_disp &= MOD8;
        else if ( memory_offset > MAX_UNSIGNED_8BIT )
            instr_buffer->mod_disp &= MOD16;
    }
}

/**
 * Given an instance of @param instr_buffer convert a memory displacement string
 * representation @param mem into its equivalent unsigned long representation
 */
static inline int
mem_tok(struct instr* instr_buffer, char* mem, int opd_pos)
{

    instr_buffer->mem_disp = true;
    bool neg               = false;
    int base               = 16;
    // find the index position of the memory displacement string
    int index = find_add_mem(mem, &neg, &base);
    FAIL_IF_MSG(get_index_reg(instr_buffer, mem, instr_buffer->opd[opd_pos].sib), "invalid memory syntax\n");
    instr_buffer->mem_offset = 0;
    // convert string to unsigned long for memory displacement representation
    if ( index != NA )
    {
        instr_buffer->mem_offset = strtoul(mem + index, NULL, base);
        // if displacement is negative represent in 2's complement
        if ( neg )
            instr_buffer->mem_offset = process_neg_disp(instr_buffer->mem_offset);
    }
    get_mod_disp(instr_buffer, neg);
    return EXIT_SUCCESS;
}

/**
 * Given an instance of @param instr_buffer convert a immediate string
 * representation @param imme into its equivalent unsigned long representation
 */
static inline void
imm_tok(struct instr* instr_buffer, char* imme)
{

    size_t imme_str_len = strlen(imme);
    char* saved_saved;
    instr_buffer->imm = true;
    int base          = 10;
    imme              = strtok_r(imme, " ", &saved_saved);
    if ( imme[1] == 'x' || imme[2] == 'x' )
    {
        base = 16;
        if ( (instr_buffer->assembly_opt & SMART_MOV_IMM) && imme_str_len < 18 )
            instr_buffer->assembly_opt |= NASM_MOV_IMM;
    }
    // convert string to unsigned long for immediate representation
    instr_buffer->cons = strtoul(imme, NULL, base);
}

/**
 * Given an instance of @param instr_buffer, checks for the presence of a
 * keyword (byte, short, or long) for @param opds at operand postion
 * @param opd_pos.
 */
static inline void
check_for_keyword(struct instr* instr_buffer, char* all_opd, int opd_pos)
{

    switch ( all_opd[0] )
    {
    case 'b':
        if ( strstr(all_opd, "byte") == NULL )
            return;
        instr_buffer->keyword.is_byte = true;
        clearstring(all_opd, BYTE_LEN);
        break;

    case 'w':
        if ( strstr(all_opd, "word") == NULL )
            return;
        instr_buffer->keyword.is_word = true;
        clearstring(all_opd, BYTE_LEN);
        break;

    case 'd':
        if ( strstr(all_opd, "dword") == NULL )
            return;
        instr_buffer->keyword.is_dword = true;
        clearstring(all_opd, DWORD_LEN);
        break;

    case 'q':
        if ( strstr(all_opd, "qword") == NULL )
            return;
        clearstring(all_opd, DWORD_LEN);
        break;

    case 's':
        if ( opd_pos != FIRST_OPERAND || all_opd[1] != 'h' )
            return;
        if ( strstr(all_opd, "short") == NULL )
            return;
        instr_buffer->keyword.is_short = true;
        instr_buffer->keyword.is_long  = false;
        clearstring(all_opd, SHORT_LEN);
        break;

    case 'l':
        if ( opd_pos != FIRST_OPERAND || all_opd[1] != 'o' )
            return;
        if ( strstr(all_opd, "long") == NULL )
            return;
        instr_buffer->keyword.is_long  = true;
        instr_buffer->keyword.is_short = false;
        clearstring(all_opd, LONG_LEN);
        break;

    default:
        return;
    }
}

/**
 * Given an instance of @param instr_buffer, checks the operand type of
 * @param opds at operand postion @param opd_pos to get the register if
 * operand is not an immediate.
 */
static inline int
check_operand_type(struct instr* instr_buffer, char* all_opd, int opd_pos, char* saved_opd)
{
    switch ( instr_buffer->opd[opd_pos].type )
    {
    // convert immediate to unsigned long
    case 'i':
        imm_tok(instr_buffer, all_opd);
        if ( strtok_r(NULL, "", &saved_opd) == NULL )
            return EXIT_SUCCESS;
        else
            FAIL_IF_MSG(true, "cannot have an operand after immediate\n");
        break;
    // get register string from operand
    case 'r':
    case 'v':
    case 'y':
    case 'm':
        get_reg_str(all_opd, instr_buffer->opd[opd_pos].str);
        if ( instr_buffer->opd[opd_pos].type == 'm' )
            return mem_tok(instr_buffer, all_opd, opd_pos);
        return EXIT_SUCCESS;
    // operand type is not found
    default:
        FAIL_IF_VAR(true, "illegal operand : \"%s\"\n", instr_buffer->instruction);
    }
}

/**
 * Given an instance of @param instr_buffer, tokenize operand string @param
 * opds at operand postion @param opd_pos.
 */
static inline int
operand_tok(struct instr* instr_buffer, char* opds, int opd_pos)
{
    char* saved_opd;
    FAIL_IF(opds[0] == ',');
    char* all_opd = instr_buffer->opd[opd_pos].ptr;
    // get the 1st operand
    all_opd = strtok_r(opds, ",", &saved_opd);
    check_for_keyword(instr_buffer, all_opd, opd_pos);
    // get the operand type can be 'i', 'r', or 'm'
    instr_buffer->opd[opd_pos].type = get_operand_type(all_opd);
    FAIL_IF(check_operand_type(instr_buffer, all_opd, opd_pos, saved_opd));
    // get next operand
    char* next_operands = strtok_r(NULL, "", &saved_opd);
    if ( next_operands == NULL )
        return EXIT_SUCCESS;
    // recursively call operand_tok to process next operand
    if ( opd_pos <= FOURTH_OPERAND )
        return operand_tok(instr_buffer, next_operands, opd_pos + 1);
    else
        return EXIT_FAILURE;
}

int
instr_tok(struct instr* instr_buffer, char* comp_instr)
{
    char* saved_instr;
    // get the string component containing the instruction
    char* instruction_str = strtok_r(comp_instr, " \t", &saved_instr);
    strncpy(instr_buffer->instruction, instruction_str, MAX_INSTR_LEN);
    char* next_operands = strtok_r(NULL, "", &saved_instr);
    if ( next_operands != NULL )
        return operand_tok(instr_buffer, next_operands, FIRST_OPERAND);
    return EXIT_SUCCESS;
}
