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

/*implements reg_parser.h*/
#include "reg_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

int
find_add_mem(char* mem, bool* neg, int* base)
{

    bool first_num = false;
    size_t len     = strlen(mem);
    // find the index of the memory displacement followed by '+' or '-' character
    for ( size_t i = 1; i < len; i++ )
    {
        if ( IN_RANGE(mem[i], '0', '9') && (mem[i - 1] == '-' || mem[i - 1] == '+') )
            first_num = true;
        if ( first_num && IN_RANGE(mem[i + 1], '0', '9') )
            *base = 10;
        // assemblyline does not support memory displacement arithmetic
        if ( first_num && mem[i + 1] == '*' )
            first_num = false;
        if ( first_num && mem[i - 1] == '-' )
        {
            *neg = true;
            return i;
        }
        else if ( first_num && mem[i - 1] == '+' )
            return i;
        first_num = false;
    }
    return NA;
}

uint32_t
process_neg_disp(uint32_t neg_num)
{
    // convert neg_num to negative 2's complement representation
    uint32_t new_disp = ~neg_num + 1;
    if ( neg_num < 0x81 )
        new_disp &= 0xff;
    return new_disp;
}

int
get_opcode_offset(struct instr *instrc)
{

    unsigned int base = instrc->opd[0].reg & MODE_MASK;
    unsigned int index = instrc->opd[0].index & MODE_MASK;
    if (IN_RANGE(base, reg16, ext64))
        return 1;
    else if (IN_RANGE(index, reg16, ext64))
        return 1;
    else
        return NONE;
}

void
get_reg_str(char* opd_str, char* reg)
{

    int j      = 0;
    size_t len = strlen(opd_str);
    // copies the register from mem to reg ex: "rax ," -> "rax"
    for ( size_t i = 0; i < len; i++ )
    {
        // exit upon encounting '*' character (it will be an index register)
        if ( opd_str[i] == '*' )
        {
            reg = NULL;
            return;
        }
        if ( j > 0 && IN_RANGE(opd_str[i], 'a', 'z') )
            reg[j++] = opd_str[i];
        else if ( j > 0 && IN_RANGE(opd_str[i], '0', '9') )
            reg[j++] = opd_str[i];
        else if ( j > 0 )
            return;
        if ( j < 1 && IN_RANGE(opd_str[i], 'a', 'z') )
            reg[j++] = opd_str[i];
        if ( j > 4 )
            return;
    }
}

static unsigned int
check_sib_disp(struct instr* instruc, char scale, char next)
{
    // scale can only be a 1 digit decimal number
    if ( next != ']' && next != '+' && next != '-' && next != '[' )
        return EXIT_FAILURE;

    switch ( scale )
    {
    case '1':
        instruc->sib_disp = SIB;
        break;
    case '2':
        instruc->sib_disp = SIB2;
        break;
    case '4':
        instruc->sib_disp = SIB4;
        break;
    case '8':
        instruc->sib_disp = SIB8;
        break;
    default:
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

unsigned int
get_index_reg(struct instr* instruc, char* mem, char* reg)
{

    bool plus     = false;
    bool multiply = false;
    size_t len    = strlen(mem);
    // check closing bracket
    if ( mem[len - 1] != ']' )
        return EXIT_FAILURE;
    // default sib_disp;
    instruc->sib_disp = SIB;
    // copies the index register from mem to reg ex: "[rcx+rax+0x16]" -> "rax"
    for ( size_t i = 0; i < len; i++ )
    {
        if ( (multiply || plus) && IN_RANGE(mem[i], 'a', 'z') )
        {
            int j = i;
            int k = 0;
            while ( ((IN_RANGE(mem[j], 'a', 'x')) || (IN_RANGE(mem[j], '0', '9'))) && k < 5 )
                reg[k++] = mem[j++];
            if ( !instruc->sib_disp && mem[j] == '*' )
            {
                FAIL_IF(check_sib_disp(instruc, mem[j + 1], mem[j + 2]));
            }
            else if ( instruc->sib_disp && mem[j] == '*' )
                return EXIT_FAILURE;
            return EXIT_SUCCESS;
        }
        if ( mem[i] == '+' )
            plus = true;
        else if ( mem[i] == '*' && i > 1 )
        {
            multiply = true;
            FAIL_IF(check_sib_disp(instruc, mem[i - 1], mem[i - 2]));
        }
        else
            plus = false;
    }
    return EXIT_SUCCESS;
}

char
get_operand_type(char* operand)
{

    int i = 0;
    // skip empty space
    while ( operand[i] == ' ' )
        i++;
    // the starting character of each operand note the type
    if ( operand[i] == '[' )
        return 'm';
    if ( operand[i] >= 'a' && operand[i] <= 's' )
        return 'r';
    if ( operand[i] == 'x' )
        return 'v';
    if ( operand[i] == 'y' )
        return 'y';
    if ( operand[i] >= '0' && operand[i] <= '9' )
        return 'i';
    if ( operand[i] >= '-' )
        return 'i';
    else
        return 'e';
}

asm_reg
find_reg(int row, const int col, char* reg_str)
{
    // match reg_str to REG_TABLE reg_conversion entry
    while ( REG_TABLE[row].reg_conversion[col][0] != '\0' )
    {
        if ( !strcmp(reg_str, REG_TABLE[row].reg_conversion[col]) )
            return REG_TABLE[row].gen_reg;
        row++;
    }
    fprintf(stderr, "assembyline: %s register not found\n", reg_str);
    return reg_error;
}

asm_reg
str_to_reg(char* reg)
{
    // the operand does not contain a register
    if ( reg[0] == '\0' )
        return reg_none;
    unsigned int end = strlen(reg) - 1;
    // 64 bit register
    if ( reg[0] == 'r' )
    {
        if ( IN_RANGE(reg[1], 'a', 'z') )
            return reg64 | find_reg(0, 4, reg);
        if ( IN_RANGE(reg[1], '0', '9') && reg[end] == 'd' )
            return ext32 | find_reg(8, 3, reg);
        if ( IN_RANGE(reg[1], '0', '9') && reg[end] == 'w' )
            return ext16 | find_reg(8, 2, reg);
        if ( IN_RANGE(reg[1], '0', '9') && reg[end] == 'b' )
            return ext8 | find_reg(8, 0, reg);
        else
            return ext64 | find_reg(8, 4, reg);
        // vector registers
    }
    else if ( reg[0] == 'm' )
    {
        return mmx64 | find_reg(16, 4, reg);
    }
    else if ( reg[0] == 'x' )
    {
        return mmx64 | find_reg(16, 5, reg);
    }
    else if ( reg[0] == 'y' )
    {
        return mmx64 | find_reg(16, 6, reg);
        // 64 bit register
    }
    else if ( reg[0] == 'e' )
    {
        return reg32 | find_reg(0, 3, reg);
    }
    // 16-8 bit register
    else
    {
        if ( reg[end] == 'l' )
            return reg8 | find_reg(0, 0, reg);
        if ( reg[end] == 'h' )
            return noext8 | find_reg(4, 1, reg);
        return reg16 | find_reg(0, 2, reg);
    }
    return reg_error;
}