/**
 * @test-tlb_simple.c
 * @brief Test for full-associative TLB
 *
 * @author Mirjana Stojilovic & J.-C. Chappelier
 * @date 2018-19
 */

// for some C99 printf flags like %PRI to compile in Windows
#if defined _WIN32  || defined _WIN64
#define __USE_MINGW_ANSI_STDIO 1
#endif

#include "error.h"
#include "util.h"
#include "addr_mng.h"
#include "commands.h"
#include "memory.h"
#include "list.h"
#include "tlb.h"
#include "tlb_mng.h"

#include <inttypes.h> // for PRIx macros

int main(int argc, char* argv[])
{
    if (argc < 4) {
        fprintf(stderr, "please provide 3 filenames:\n");
        fprintf(stderr, "\t- one (txt) to read commands from;\n");
        fprintf(stderr, "\t- one (bin) to memory content from;\n");
        fprintf(stderr, "\t- one to write output to.\n");
        return 1;
    }

    program_t pgm;
    if (program_read(argv[1], &pgm) != ERR_NONE) {
        fprintf(stderr, "Cannot open \"%s\" for reading commands.", argv[1]);
        return 2;
    }


    // For testing purposes, print the array of recent accesses to a file
    FILE * f_out = fopen(argv[3], "w");
    if (f_out == NULL) {
        fprintf(stderr, "Cannot open \"%s\" for writting.", argv[3]);
        return 3;
    }

    void* mem_space = NULL;
    size_t mem_size = 0;
    if (mem_init_from_dumpfile(argv[2], &mem_space, &mem_size) != ERR_NONE) {
        fclose(f_out);
        fprintf(stderr, "Cannot read memory dump from \"%s\".", argv[2]);
        return 4;
    }


    // Allocate TLB
    tlb_entry_t tlb[TLB_LINES];
    tlb_flush(tlb);

    // fill in the linked-list with all tlb line indices
    list_t ll;
    init_list(&ll);
    
    for (list_content_t line_index = 0; line_index < TLB_LINES; line_index++) {
        (void)push_back(&ll, &line_index);
    }
    
    /*
    * Create the object replacement policy.
    *
    */
    replacement_policy_t replacement_policy = {
        .ll             = &ll,
        .move_back      = move_back,
        .push_back      = push_back
    };

    phy_addr_t paddr;
    zero_init_var(paddr);

    for (size_t prog_line_index = 0; prog_line_index < pgm.nb_lines; prog_line_index++) {

        int hit = 0;
        int err = tlb_search(mem_space, &(pgm.listing[prog_line_index].vaddr), &paddr, tlb, &replacement_policy, &hit);

        fprintf(f_out, "-------------------------------------------------------------------\n");
        fprintf(f_out, "After program line " SIZE_T_FMT "...\n\n", prog_line_index);
        fprintf(f_out, "VA = ");
        print_virtual_address(f_out, &(pgm.listing[prog_line_index].vaddr));
        if (err == ERR_NONE) {
            fprintf(f_out, "; PA  = ");
            print_physical_address(f_out, &paddr);
            fprintf(f_out, "\n\n");
            if (hit) fprintf(f_out, "HIT...\n\n");
            else fprintf(f_out, "MISS...\n\n");

            for (size_t tlb_line_index = 0; tlb_line_index < TLB_LINES; tlb_line_index++) {
                fprintf(f_out, "%d; %"PRIx64"; %05X;\n",
                        tlb[tlb_line_index].v,
                        (uint64_t) tlb[tlb_line_index].tag,
                        tlb[tlb_line_index].phy_page_num
                       );
            }
            print_list(f_out, &ll);
        } else {
            fprintf(f_out, "error with tlb_search(): %s\n", ERR_MESSAGES[err - ERR_NONE]);
        }
        fprintf(f_out, "-------------------------------------------------------------------\n");
    }

    /**
     * Garbage collecting
     */
    fclose(f_out);
    clear_list(&ll);
    free(mem_space);

    return EXIT_SUCCESS;
}


