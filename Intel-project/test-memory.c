/**
 * @test-memory.c
 * @brief black-box testing of memory management functions
 *
 * @author Jean-Cédric Chappelier
 * @date 2019
 */

#if defined _WIN32  || defined _WIN64
#define __USE_MINGW_ANSI_STDIO 1
#endif

#include "error.h"
#include "memory.h"
#include "page_walk.h"
#include "util.h"  // for zero_init_var()
#include "addr_mng.h" // for init_virt_addr64()
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h> // for isspace()
#include <inttypes.h> // for SCNx macro

// ======================================================================
static void error(const char* pgm, const char* msg)
{
    assert(msg != NULL);
    fputs("ERROR: ", stderr);
    fputs(msg, stderr);
    fprintf(stderr, "\nusage:    %s (dump|desc) filename (p|o|u|n) spacer "\
            "[list of VA to print]\n", pgm);
    fprintf(stderr, "examples: %s dump memory_dump.bin o , 0xff000\n", pgm);
    fprintf(stderr, "          %s desc memory_description.txt o , 0xff000 0xfe000\n", pgm);
}

// ======================================================================
int main(int argc, char *argv[])
{

    if (argc < 6) {
        error(argv[0], "please provide command, format, spacer and filename to read from:");
        return 1;
    }
    int dump = 1;
    if (strcmp(argv[1], "dump")) {
        if (strcmp(argv[1], "desc")) {
            error(argv[0], "unknown command.");
            return 1;
        }
        dump = 0;
    }

    void* mem_space = NULL;
    size_t mem_size = 0;
    int err = ERR_NONE;
    if (dump)
        err = mem_init_from_dumpfile(argv[2], &mem_space, &mem_size);        
    else
        err = mem_init_from_description(argv[2], &mem_space, &mem_size);

    addr_fmt_t t_fmt;
    switch(argv[3][0]) {
    case 'p':
        t_fmt = POINTER;
        break;
    case 'o':
        t_fmt = OFFSET;
        break;
    case 'u':
        t_fmt = OFFSET_U;
        break;
    case 'n':
    default:
        t_fmt = NONE;
        break;
    }

    /* Limit spacer to one character */
    argv[4][1] = 0;

    if (err == ERR_NONE) {
        virt_addr_t vaddr;
        zero_init_var(vaddr);

        int i;
        uint64_t vaddr64;
        for(i = 5; i < argc; i++) {
            if(sscanf(argv[i], "%"SCNx64, &vaddr64) != 1) {
                puts("pas compris ! ==> Abandon");
                continue;
            }
            const int error = init_virt_addr64(&vaddr, vaddr64);
            if (error != ERR_NONE) {
                puts("Mauvaise adresse ==> Abandon");
                free(mem_space);
                return 2;
            }

            vmem_page_dump_with_options(mem_space, &vaddr, t_fmt, 16, argv[4]);

        }

    } else {
        error(argv[0], "problem initializing memory from provided file.");
        return 3;
    }

    free(mem_space);
    return 0;
}
