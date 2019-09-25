/**
 * @file test-cache.c
 * @brief black-box testing of cache management functions
 *
 * @author Atri Bhattacharyya
 * @date 2019
 */

#if defined _WIN32  || defined _WIN64
#define __USE_MINGW_ANSI_STDIO 1
#endif

#include "error.h"
// #include "memory.h"
// #include "util.h"  // for zero_init_var()
// #include "addr_mng.h" // for init_virt_addr64()

#include "cache_mng.h"
#include "commands.h"
#include "memory.h"
#include "page_walk.h"

// #include <stdio.h>
#include <assert.h>
#include <string.h>
// #include <ctype.h> // for isspace()
// #include <inttypes.h> // for SCNx macro

// ======================================================================
static void error(const char* pgm, const char* msg)
{
    assert(msg != NULL);
    fputs("ERROR: ", stderr);
    fputs(msg, stderr);
    fprintf(stderr, "\nusage:    %s (dump|desc) mem_filename command_filename\n", pgm);
    fprintf(stderr, "examples: %s dump memory_dump.bin commands01.txt\n", pgm);
    fprintf(stderr, "          %s desc memory_description.txt commands01.txt\n", pgm);
}

// ======================================================================
void execute_command(void *mem_space,
                     const command_t* command,
                     l1_icache_entry_t *l1_icache,
                     l1_icache_entry_t *l1_dcache,
                     l2_cache_entry_t *l2_cache)
{
    phy_addr_t paddr;
    assert(page_walk(mem_space, &command->vaddr, &paddr) == ERR_NONE);
    uint8_t byte;
    uint32_t word;
    void *l1_cache;

    switch (command->order) {
    case READ:
        l1_cache = (command->type == INSTRUCTION)? l1_icache: l1_dcache;
        if(command->data_size == 4)
            cache_read(mem_space, &paddr, command->type, l1_cache,
                       l2_cache, &word, LRU);
        else
            cache_read_byte(mem_space, &paddr, command->type, l1_cache,
                            l2_cache, &byte, LRU);
        break;
    case WRITE:
        if(command->data_size == 4)
            cache_write(mem_space, &paddr, l1_dcache,
                        l2_cache, &command->write_data, LRU);
        else
            cache_write_byte(mem_space, &paddr, l1_dcache,
                             l2_cache, (uint8_t)command->write_data, LRU);
        break;
    default:
        assert(0);
    }
}

// ======================================================================
int main(int argc, char *argv[])
{
    if (argc < 4) {
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


    program_t pgm;
    if (err == ERR_NONE) {
        if(program_read(argv[3], &pgm) == ERR_NONE) {
            l1_icache_entry_t l1_icache[L1_ICACHE_LINES * L1_ICACHE_WAYS];
            l1_icache_entry_t l1_dcache[L1_DCACHE_LINES * L1_DCACHE_WAYS];
            l2_cache_entry_t l2_cache[L2_CACHE_LINES * L2_CACHE_WAYS];
            memset(l1_icache, 0, sizeof(l1_icache));
            memset(l1_dcache, 0, sizeof(l1_dcache));
            memset(l2_cache,  0, sizeof(l2_cache));

            /* Flush caches before use */
            assert(cache_flush(l1_icache, L1_ICACHE) == ERR_NONE);
            assert(cache_flush(l1_dcache, L1_DCACHE) == ERR_NONE);
            assert(cache_flush(l2_cache, L2_CACHE) == ERR_NONE);

            for_all_lines(line, &pgm) {
                execute_command(mem_space, line, l1_icache, l1_dcache, l2_cache);

                printf("L1_ICACHE: \n\n");
                cache_dump(stdout, l1_icache, L1_ICACHE);
                printf("L1_DCACHE: \n\n");
                cache_dump(stdout, l1_dcache, L1_DCACHE);
                printf("L2_CACHE: \n\n");
                cache_dump(stdout, l2_cache, L2_CACHE);
                printf("\n=======================================\n\n");
            }
        } else {
            error(argv[0], "problem initializing program from provided file.");
            return 3;
        }
    } else {
        error(argv[0], "problem initializing memory from provided file.");
        return 3;
    }

    (void)program_free(&pgm);
    free(mem_space);
    return 0;
}
