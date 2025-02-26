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

/*executes assembly program represented as a string*/
#include <assemblyline.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(__linux__)
#include <sys/mman.h>
#else
#include <windows.h>
#endif

// describes how operands are encoded
typedef enum
{
    ADD,
    SUB,
    MUL,
    DIV,

} operation;

int
execute_double_test(void (*exe)(double*, double*, double*), operation type)
{

    double A[] = {1.0, 2.0, 3.0, 4.0};
    double B[] = {1.0, 2.0, 3.0, 4.0};
    double C[] = {1.0, 1.0, 1.0, 1.0};
    exe(A, B, C);
    switch ( type )
    {
    case ADD:
        for ( int it = 0; it < 4; it++ )
            if ( C[it] != A[it] + B[it] )
                return EXIT_FAILURE;
        break;
    case SUB:
        for ( int it = 0; it < 4; it++ )
            if ( C[it] != A[it] - B[it] )
                return EXIT_FAILURE;
        break;
    case MUL:
        for ( int it = 0; it < 4; it++ )
            if ( C[it] != A[it] * B[it] )
                return EXIT_FAILURE;
        break;
    case DIV:
        for ( int it = 0; it < 4; it++ )
            if ( C[it] != A[it] / B[it] )
                return EXIT_FAILURE;
        break;
    default:
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int
execute_long_test(void (*exe)(long*, long*, long*), operation type)
{

    long A[] = {1, 2, 3, 4};
    long B[] = {1, 2, 3, 4};
    long C[] = {1, 1, 1, 1};
    exe(A, B, C);
    switch ( type )
    {
    case ADD:
        for ( int it = 0; it < 4; it++ )
            if ( C[it] != A[it] + B[it] )
                return EXIT_FAILURE;
        break;
    case SUB:
        for ( int it = 0; it < 4; it++ )
            if ( C[it] != A[it] - B[it] )
                return EXIT_FAILURE;
        break;
    case MUL:
        for ( int it = 0; it < 4; it++ )
            if ( C[it] != A[it] * B[it] )
                return EXIT_FAILURE;
        break;
    case DIV:
        for ( int it = 0; it < 4; it++ )
            if ( C[it] != A[it] / B[it] )
                return EXIT_FAILURE;
        break;
    default:
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

const char* add_double_ymm =
    "vmovupd ymm0, [rdi]\n"
    "vmovupd ymm1, [rsi]\n"
    "vaddpd ymm3, ymm0, ymm1\n"
    "vmovupd [rdx], ymm3\n"
    "ret\n";

const char* sub_double_ymm =
    "vmovupd ymm0, [rdi]\n"
    "vmovupd ymm1, [rsi]\n"
    "vsubpd ymm3, ymm0, ymm1\n"
    "vmovupd [rdx], ymm3\n"
    "ret\n";

const char* mul_double_ymm =
    "vmovupd ymm0, [rdi]\n"
    "vmovupd ymm1, [rsi]\n"
    "vmulpd ymm3, ymm0, ymm1\n"
    "vmovupd [rdx], ymm3\n"
    "ret\n";

const char* div_double_ymm =
    "vmovupd ymm0, [rdi]\n"
    "vmovupd ymm1, [rsi]\n"
    "vdivpd ymm3, ymm0, ymm1\n"
    "vmovupd [rdx], ymm3\n"
    "ret\n";

const char* add_long_ymm =
    "vmovdqu ymm0, [rdi]\n"
    "vmovdqu ymm1, [rsi]\n"
    "vpaddq ymm3, ymm0, ymm1\n"
    "vmovdqu [rdx], ymm3\n"
    "ret\n";

const char* sub_long_ymm =
    "vmovdqu ymm0, [rdi]\n"
    "vmovdqu ymm1, [rsi]\n"
    "vpsubq ymm3, ymm0, ymm1\n"
    "vmovdqu [rdx], ymm3\n"
    "ret\n";

const char* mul_long_ymm =
    "vmovdqu ymm0, [rdi]\n"
    "vmovdqu ymm1, [rsi]\n"
    "vpmuldq ymm3, ymm0, ymm1\n"
    "vmovdqu [rdx], ymm3\n"
    "ret\n";

int
main()
{

    assemblyline_t al = asm_create_instance(NULL, 0);
    if ( asm_assemble_str(al, add_double_ymm) == EXIT_FAILURE )
        return EXIT_FAILURE;

    void (*test_double)(double* A, double* B, double* C) = asm_get_code(al);

    if ( execute_double_test(test_double, ADD) )
    {
        fprintf(stderr, "ADD did not produce expected results\n");
        return EXIT_FAILURE;
    }
    // clear previous test
    asm_set_offset(al, 0);

    if ( asm_assemble_str(al, sub_double_ymm) == EXIT_FAILURE )
        return EXIT_FAILURE;

    test_double = asm_get_code(al);

    if ( execute_double_test(test_double, SUB) )
    {
        fprintf(stderr, "SUBTRACT did not produce expected results\n");
        return EXIT_FAILURE;
    }

    // clear previous test
    asm_set_offset(al, 0);

    if ( asm_assemble_str(al, mul_double_ymm) == EXIT_FAILURE )
        return EXIT_FAILURE;

    test_double = asm_get_code(al);

    if ( execute_double_test(test_double, MUL) )
    {
        fprintf(stderr, "MULTIPLY did not produce expected results\n");
        return EXIT_FAILURE;
    }

    // clear previous test
    asm_set_offset(al, 0);

    if ( asm_assemble_str(al, div_double_ymm) == EXIT_FAILURE )
        return EXIT_FAILURE;

    test_double = asm_get_code(al);

    if ( execute_double_test(test_double, DIV) )
    {
        fprintf(stderr, "DIVIDE did not produce expected results\n");
        return EXIT_FAILURE;
    }

    // clear previous test
    asm_set_offset(al, 0);

    if ( asm_assemble_str(al, add_long_ymm) == EXIT_FAILURE )
        return EXIT_FAILURE;

    void (*test_long)(long* A, long* B, long* C) = asm_get_code(al);

    if ( execute_long_test(test_long, ADD) )
    {
        fprintf(stderr, "ADD long did not produce expected results\n");
        return EXIT_FAILURE;
    }

    // clear previous test
    asm_set_offset(al, 0);

    if ( asm_assemble_str(al, sub_long_ymm) == EXIT_FAILURE )
        return EXIT_FAILURE;

    test_long = asm_get_code(al);

    if ( execute_long_test(test_long, SUB) )
    {
        fprintf(stderr, "SUBTRACT long did not produce expected results\n");
        return EXIT_FAILURE;
    }

    // clear previous test
    asm_set_offset(al, 0);

    if ( asm_assemble_str(al, mul_long_ymm) == EXIT_FAILURE )
        return EXIT_FAILURE;

    test_long = asm_get_code(al);

    if ( execute_long_test(test_long, MUL) )
    {
        fprintf(stderr, "MULTIPLY long did not produce expected results\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
