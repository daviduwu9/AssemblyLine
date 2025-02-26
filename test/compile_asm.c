#include <assemblyline.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#ifdef TEST_DEBUG
#define dbg(x, ...) printf("[DEBUG] " x, __VA_ARGS__)
#else
#define dbg(x, ...)
#endif

///
/// The code to evaluate for comparison
///

// clang-format off
#include <test_zero_byte_rbp.h>

#ifndef CODE
#error "no code defined"
#endif
// clang-format on

__attribute__((noinline, section("mytest"), naked)) void
test(void)
{
    __asm__(CODE);
}


int
main()
{
    // get the section size
    extern unsigned char __start_mytest[];
    extern unsigned char __stop_mytest[];
    size_t generated_bytes_size = __stop_mytest - __start_mytest;

    // prepare asmline
    assemblyline_t al = asm_create_instance(NULL, 0);

    // adjust (because gcc always adds some junk instructions)
    unsigned char* generated_bytes = (unsigned char*)test;
    generated_bytes += 4;
    generated_bytes_size -= 6;

    dbg("compiled code starts at %p, size = %lu\n", generated_bytes, generated_bytes_size);

    // put 'code' to a variable on the stack
    char original_code[] = CODE;
    int result           = EXIT_SUCCESS;
    char *token, *savetok, *ptr;

    size_t compiled_offset = 0, linenum = 0;
    ptr = original_code;

    while ( result == EXIT_SUCCESS )
    {
        linenum++;

        // get next instruction string
        token = strtok_r(ptr, "\n", &savetok);
        if ( !token )
            break;

        ptr = NULL;

        dbg("Compiling '%s'\n", token);

        int generated_offset = asm_get_offset(al);
        if ( generated_offset != compiled_offset )
        {
            fprintf(
                stderr,
                "Error: offset mismatch at line %lu, expected %lu, got %d\n",
                linenum,
                compiled_offset,
                generated_offset);
            result = EXIT_FAILURE;
            break;
        }

        // get a pointer to the start of the current instruction string
        const unsigned char* start = asm_get_code(al) + generated_offset;

        // assemble the current line
        if ( asm_assemble_str(al, token) != EXIT_SUCCESS )
        {
            fprintf(
                stderr,
                "Instruction >>%s<< failed to compile (line=%lu, offset=%d)\n",
                token,
                linenum,
                generated_offset);
            result = EXIT_FAILURE;
            break;
        }

        // get the size of the current instruction
        const size_t insn_sz = asm_get_offset(al) - generated_offset;

        // get a pointer to the start of the past instruction string
        const unsigned char* end = start + insn_sz;

        dbg("Successfully compiled instruction: start=%p end=%p size=%lu, offset=%d\n",
            start,
            end,
            insn_sz,
            generated_offset);

        // diff the expected and the generated codes
        const int res = memcmp(start, &generated_bytes[compiled_offset], insn_sz);
        if ( !res )
        {
            // valid case, continue
            compiled_offset += insn_sz;
            continue;
        }

        // display the detail of the error
        fprintf(stderr, "Instruction >>%s<< (size=%lu) failed at offset=%lu\n", token, insn_sz, compiled_offset);
        fprintf(stderr, "Expected: ");
        for ( size_t i = 0; i < insn_sz; i++ )
            fprintf(stderr, "%.02x ", generated_bytes[compiled_offset + i]);
        fprintf(stderr, "\n");

        fprintf(stderr, "Got:      ");
        for ( size_t i = 0; i < insn_sz; i++ )
            fprintf(stderr, "%.02x ", start[i]);
        fprintf(stderr, "\n");

        result = EXIT_FAILURE;
        break;
    }

    // cleanup
    asm_destroy_instance(al);

    // exit
    return result;
}
