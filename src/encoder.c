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

/*implements functions for encoding register and prefix opcode
as well process any offsets or immediate operands*/
#include "encoder.h"

#include <stdlib.h>

#include "instr_parser.h"
#include "instructions.h"
#include "prefix.h"
#include "reg_parser.h"

/**
 * sets the operand size to 8 bits if the instruction referenced by
 * @param instrc only support 8-bit registers
 */
static void
auto_set_byte(struct instr* instrc)
{
    // instructions that only supports a single 8-bit operand
    if ( INSTR_TABLE[instrc->key].type == BYTE_OPD )
    {
        instrc->keyword.is_byte = true;
        instrc->op_offset       = 0;
    }
}

/**
 * sets the operand size to 8 bits if the instruction referenced by
 * @param instrc only support 8-bit registers
 */
static void
auto_set_operand(struct instr* instrc, int r)
{
    // exit if keyword is present or register is 64 bits or greater
    if ( instrc->keyword.is_keyword || (r & reg64) )
        return;
    switch ( r & BIT_MASK )
    {
    case BIT_32:
        instrc->keyword.is_dword = true;
        break;

    case BIT_16:
        instrc->keyword.is_word = true;
        break;

    default:
        if ( r < reg16 )
        {
            instrc->keyword.is_byte = true;
            instrc->op_offset       = 0;
        }
    }
}

void
encode_offset(struct instr* instrc)
{
    // check if register is no prefix
    if ( (instrc->opd[0].reg & MODE_MASK) == noext8 || (instrc->opd[1].reg & MODE_MASK) == noext8 )
        return;
    // set opcode offset
    if ( INSTR_TABLE[instrc->key].type != CONTROL_FLOW && !instrc->keyword.is_byte )
        instrc->op_offset = get_opcode_offset(instrc);
}

/**
 * determines whether a zero byte is required for @param instrc depending on
 * the @param m register position
 */
static int
encode_mem(struct instr* instrc, int m)
{

    if ( !instrc->mem_disp )
        return NA;
    if ( (instrc->opd[m].reg & BIT_MASK) == BIT_32 || (instrc->opd[m].index & BIT_MASK) == BIT_32 )
        instrc->hex.is_67H = true;
    if ( (instrc->opd[m].reg & BIT_MASK) == BIT_16 )
        instrc->hex.is_66H = true;

    // auto correct SIB syntax in nasm and smart mode
    if ( (instrc->assembly_opt & NASM_SIB_INDEX_BASE_SWAP) && (instrc->opd[m].index & REG_MASK) == spl &&
         !instrc->sib_disp )
    {
        asm_reg swap         = instrc->opd[m].index;
        instrc->opd[m].index = instrc->opd[m].reg;
        instrc->opd[m].reg   = swap;
    }
    // if r/m value is a memory reference and is the spl register
    if ( (instrc->opd[m].reg & VALUE_MASK) == spl && instrc->opd[m].index == reg_none )
        instrc->is_sib_const = true;
    if ( instrc->mem_offset )
        return EXIT_SUCCESS;
    // check for an additional zero byte when memory displace is zero
    unsigned int reg_opd = instrc->opd[m].reg & MODE_MASK;
    if ( reg_opd > ext16 && reg_opd < mmx64 && !instrc->mem_offset && (instrc->opd[m].reg & VALUE_MASK) == bpl )
    {
        instrc->mod_disp  = MOD8;
        instrc->zero_byte = true;
    }
    return EXIT_SUCCESS;
}

/**
 * determines register and prefix opcode of @param instrc with 2 operands:
 * @param r and @param m used for MR or RM encoding
 */
static int
encode_two_opds(struct instr* instrc, int r, int m)
{
    if ( encode_mem(instrc, m) != NA )
        auto_set_operand(instrc, instrc->opd[r].reg);
    // check if a second register exist in memory reference ex: [rax+rcx]
    FAIL_IF(get_reg(instrc, &instrc->opd[m], instrc->opd[r].reg));
    instrc->hex.rex = get_rex_prefix(instrc, &instrc->opd[m], &instrc->opd[r]);
    return EXIT_SUCCESS;
}

/**
 * determines register and prefix opcode of @param instrc with 3 operands:
 * @param r, @param m, and @param v used for RMV or RVM encoding
 */
static int
encode_three_opds(struct instr* instrc, int r, int m, int v)
{
    if ( encode_mem(instrc, m) != NA )
        auto_set_operand(instrc, instrc->opd[r].reg);

    // get vvvv parameter
    instrc->hex.vvvv = instrc->opd[v].reg & 0xf;
    FAIL_IF(get_reg(instrc, &instrc->opd[m], instrc->opd[r].reg));
    // used for RXB and R
    instrc->hex.rex = get_rex_prefix(instrc, &instrc->opd[m], &instrc->opd[r]);
    return EXIT_SUCCESS;
}

/**
 * determines register and prefix opcode of @param instrc with a single
 * operand
 * @param m or special predefined encoding I with 2 operands:
 * @param m and @param i
 */
static int
encode_special_opd(struct instr* instrc, int m, int i)
{
    struct operand no_register = {NULL, {'\0'}, reg_none, {'\0'}, reg_none, 0};
    switch ( INSTR_TABLE[instrc->key].encode_operand )
    {
    case M:
        encode_mem(instrc, m);
        FAIL_IF(get_reg(instrc, &instrc->opd[m], INSTR_TABLE[instrc->key].single_reg_r));
        instrc->hex.rex = get_rex_prefix(instrc, &instrc->opd[m], &no_register);
        break;

    case O:
        FAIL_IF(get_reg(instrc, &instrc->opd[m], INSTR_TABLE[instrc->key].single_reg_r));
        if ( (MODE_MASK & instrc->opd[m].reg) == ext64 )
            instrc->hex.rex = rex_ + rex_b;
        instrc->rd_offset = (instrc->opd[m].reg & VALUE_MASK);

        break;

    case I:
        instrc->hex.rex = get_rex_prefix(instrc, &instrc->opd[m], &instrc->opd[i]);
        break;

    default:
        // do nothing
        break;
    }
    return EXIT_SUCCESS;
}

int
encode_operands(struct instr* instrc)
{
    // xchg instruction with RM operand encoding using rax or al register
    if ( INSTR_TABLE[instrc->key].name == xchg && INSTR_TABLE[instrc->key].encode_operand == RM )
    {
        if ( (reg64 & instrc->opd[0].reg) && (REG_MASK & instrc->opd[0].reg) == al )
        {
            instrc->key++;
            instrc->rd_offset = instrc->opd[1].reg & VALUE_MASK;
        }
        else if ( (reg64 & instrc->opd[1].reg) && (REG_MASK & instrc->opd[1].reg) == al )
        {
            instrc->key++;
            instrc->rd_offset = instrc->opd[0].reg & VALUE_MASK;
        }
    }
    // set 'byte' keyword
    if ( instrc->mem_disp )
        auto_set_byte(instrc);

    // get register value and prefix for each operand encoding type
    switch ( INSTR_TABLE[instrc->key].encode_operand )
    {
    case MR:
        return encode_two_opds(instrc, SECOND_OPERAND, FIRST_OPERAND);

    case RM:
        return encode_two_opds(instrc, FIRST_OPERAND, SECOND_OPERAND);

    case RVM:
        return encode_three_opds(instrc, FIRST_OPERAND, THIRD_OPERAND, SECOND_OPERAND);

    case RMV:
        return encode_three_opds(instrc, FIRST_OPERAND, SECOND_OPERAND, THIRD_OPERAND);

    default:
        return encode_special_opd(instrc, FIRST_OPERAND, SECOND_OPERAND);
    }
}

static void
nasm_register_size_optimize(struct instr* instrc)
{
    switch ( instrc->opd[0].reg & MODE_MASK )
    {
    case reg64:
        instrc->opd[0].reg = (instrc->opd[0].reg & MODE_CLEAR) | reg32;
        break;

    case ext64:
        instrc->opd[0].reg = (instrc->opd[0].reg & MODE_CLEAR) | ext32;
        break;

    default:
        break;
    }
}

void 
encode_imm(struct instr *instrc) {
    // ignore imm value if instruction is a branch type
    if ((INSTR_TABLE[instrc->key].type == SHIFT && instrc->cons == 1) ||
        INSTR_TABLE[instrc->key].type == CONTROL_FLOW)
        instrc->imm = false;
    // used for shift instruction with an 8bit imm rather than one
    else if (INSTR_TABLE[instrc->key].type == SHIFT && instrc->cons != 1)
        instrc->key++;
    // change op offset based on reg and imm size
    if (!instrc->imm)
        return;
    // return register used for imm instructions sub, sbb, add, adc
    if (INSTR_TABLE[instrc->key].type == OPERATION) {
    // special case for the al register
        if ((instrc->opd[0].reg == al && instrc->cons != NEG64BIT &&
             instrc->cons != MAX_UNSIGNED_32BIT)) {
            instrc->key++;
        } else if (((instrc->opd[0].reg & REG_MASK) == al && instrc->cons != MAX_UNSIGNED_32BIT
                    && IN_RANGE(instrc->cons, MAX_SIGNED_8BIT + 1, NEG64BIT - 1)
                    && !(IN_RANGE(instrc->cons, NEG80BIT, NEG64BIT - 1))))
            instrc->key++;
    }
    // vector shift instrctions
    if ((instrc->opd[0].reg & MODE_MASK) == mmx64) {
        instrc->reduced_imm = true;
    // special case for test
    } else if (instrc->op_offset == 1 && INSTR_TABLE[instrc->key].type == PAD_ALWAYS) {
        if (IN_RANGE(instrc->cons, NEG32BIT + 1, NEG64BIT)) {
            DO_NOT_PAD(instrc->cons, instrc->reduced_imm, MAX_UNSIGNED_32BIT);
        }
        if ((instrc->opd[0].reg & REG_MASK) == al)
        instrc->key++;
    // 16 to 64 bit register and 8 bit immediate combination
    } else if (instrc->op_offset == 1 && INSTR_TABLE[instrc->key].type != DATA_TRANSFER) {
        //-0x1 and 0x0 are always 8 bits except for mov
        // 8 bit positive
        if (instrc->cons <= 0xe0)
            instrc->op_offset += 2;
        // 8 bit positive immediate
        if (instrc->cons > MAX_SIGNED_8BIT)
            instrc->op_offset = 1;
        // 8 bit negative immediate
        if (IN_RANGE(instrc->cons, NEG8BIT + 1, NEG64BIT) && (instrc->cons & NEG8BIT_CHECK)) {
            instrc->cons &= MAX_UNSIGNED_8BIT;
            instrc->op_offset += 2;
        // negative 32 bit number
        } else if (IN_RANGE(instrc->cons, NEG32BIT + 1, NEG64BIT)) {
            DO_NOT_PAD(instrc->cons, instrc->reduced_imm, MAX_UNSIGNED_32BIT);
        // positive 32 bit number
        } else if (IN_RANGE(instrc->cons, X32BIT_CHECK, MAX_UNSIGNED_32BIT)) {
            DO_NOT_PAD(instrc->cons, instrc->reduced_imm, MAX_UNSIGNED_32BIT);
        if (IN_RANGE(instrc->cons, NEG80_32BIT, MAX_UNSIGNED_32BIT)) {
            instrc->cons &= MAX_UNSIGNED_8BIT;
            instrc->op_offset += 2;
        }
    }
    // special condition for to mov instruction
    } else if (INSTR_TABLE[instrc->key].type == DATA_TRANSFER) {
        // calculate value for +rd and +rw
        instrc->rd_offset = instrc->opd[0].reg & VALUE_MASK;
        // only condition for mov with M operand encoding implementation
        // check if immediate operand is a negative 32 bit value
        if (IN_RANGE(instrc->cons, NEG32BIT + 1, NEG64BIT) && (instrc->cons & NEG32BIT_CHECK)
            && ((instrc->opd[0].reg & reg64) || instrc->mem_disp)) {
            // set mov I operand encoding to M
            instrc->key++;
            DO_NOT_PAD(instrc->cons, instrc->reduced_imm, MAX_UNSIGNED_32BIT);
            return;
        } else if (instrc->cons <= MAX_UNSIGNED_32BIT) {
            if ((instrc->assembly_opt & NASM_MOV_IMM) && !instrc->mem_disp)
                nasm_register_size_optimize(instrc);
            // ensure instruction does not default to movabs
            else if ((instrc->cons < NEG32BIT_CHECK && (instrc->opd[0].reg & MODE_MASK) >= reg64)
                      || instrc->mem_disp)
                instrc->key++;
        }
        if ((instrc->opd[0].reg & MODE_MASK) > noext8) {
            if ((instrc->assembly_opt & NASM_MOV_IMM) && !instrc->mem_disp)
                instrc->op_offset = 8;
            else if (INSTR_TABLE[instrc->key].encode_operand == I)
                instrc->op_offset = 8;
        }
    }
    // mask all bits except for the most significant byte
    if ((instrc->opd[0].reg & MODE_MASK) < reg32) {
        DO_NOT_PAD(instrc->cons, instrc->reduced_imm, MAX_UNSIGNED_16BIT);
    if (((instrc->opd[0].reg & MODE_MASK) == reg16 || (instrc->opd[0].reg & MODE_MASK) == ext16)
         && instrc->cons <= MAX_UNSIGNED_8BIT)
        instrc->reduced_imm = false;
  }
  if ((instrc->opd[0].reg & MODE_MASK) < reg16) {
    DO_NOT_PAD(instrc->cons, instrc->reduced_imm, MAX_UNSIGNED_8BIT);
  }
}