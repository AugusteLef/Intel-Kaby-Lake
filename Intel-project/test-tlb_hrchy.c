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
#include "tlb_hrchy.h"
#include "tlb_hrchy_mng.h"

#include <inttypes.h> // for PRIx macros

// --------------------------------------------------
#define print_all_tlb_entries(tlb, TYPE, N)                                      \
    do {                                                                         \
        fputc('\n', f_out); fputc('\n', f_out);                                  \
        for (int tlb_line_index = 0; tlb_line_index < (N); tlb_line_index++) {   \
            if(((TYPE *) (tlb) + tlb_line_index)->v)                             \
                fprintf(f_out, "%d; %08X; %05X;\n" ,                             \
                        ((TYPE *) (tlb) + tlb_line_index)->v,                    \
                        ((TYPE *) (tlb) + tlb_line_index)->tag,                  \
                        ((TYPE *) (tlb) + tlb_line_index)->phy_page_num          \
                );                                                               \
            else                                                                 \
                fprintf(f_out, "%d; --------; -----;\n" ,                        \
                        ((TYPE *) (tlb) + tlb_line_index)->v                     \
                );                                                               \
        }} while(0)

// ======================================================================
static void usage()
{
    fputs("Please provide 3 filenames:\n", stderr);
    fputs("\t- one (txt) to read commands from;\n", stderr);
    fputs("\t- one (bin) to memory content from;\n", stderr);
    fputs("\t- one to write output to.\n", stderr);
}

// ======================================================================
int main(int argc, char* argv[])
{
    if (argc < 4) {
        usage();
        return 1;
    }

    program_t pgm;
    if (program_read(argv[1], &pgm) != ERR_NONE) {
        fprintf(stderr, "Cannot open \"%s\" for reading commands.\n", argv[1]);
        return 2;
    }

    // For testing purposes, print the array of recent accesses to a file
    FILE * f_out = fopen(argv[3], "w");
    if (f_out == NULL) {
        fprintf(stderr, "Cannot open \"%s\" for writting.\n", argv[3]);
        return 3;
    }

    void* mem_space = NULL;
    size_t mem_size = 0;
    if (mem_init_from_dumpfile(argv[2], &mem_space, &mem_size) != ERR_NONE) {
        fclose(f_out);
        fprintf(stderr, "Cannot read memory dump from \"%s\".\n", argv[2]);
        return 4;
    }

    /**
     * Statically allocate space for the L1-ITLB, L1-DTLB, and L2-TLB
     *
     * Specs:
     *  -- Direct mapped
     *  -- 16 lines for L1, 64 lines for L2
     */

    l1_itlb_entry_t l1_itlb[L1_ITLB_LINES];
    l1_dtlb_entry_t l1_dtlb[L1_DTLB_LINES];
    l2_tlb_entry_t l2_tlb[L2_TLB_LINES];

    tlb_flush((void *)l1_itlb, L1_ITLB);
    tlb_flush((void *)l1_dtlb, L1_DTLB);
    tlb_flush((void *)l2_tlb, L2_TLB);

    phy_addr_t paddr;
    zero_init_var(paddr);

    for (size_t prog_line_index = 0; prog_line_index < pgm.nb_lines; prog_line_index++) {

        int hit = 0;
        fprintf(f_out, "\n" SIZE_T_FMT ": DATA/INSTRUCTION = %d\n", prog_line_index, pgm.listing[prog_line_index].type == DATA ? DATA : INSTRUCTION);
        tlb_search(mem_space, &(pgm.listing[prog_line_index].vaddr), &paddr, pgm.listing[prog_line_index].type == DATA ? DATA : INSTRUCTION, l1_itlb, l1_dtlb, l2_tlb, &hit);

        fprintf(f_out, "-------------------------------------------------------------------\n");
        fprintf(f_out, "After program line " SIZE_T_FMT "...\n\n", prog_line_index);
        fprintf(f_out, "VA = ");
        print_virtual_address(f_out, &(pgm.listing[prog_line_index].vaddr));
        fprintf(f_out, "; PA  = ");
        print_physical_address(f_out, &paddr);
        fprintf(f_out, "\n\n");
        if (hit) fprintf(f_out, "HIT...\n\n");
        else fprintf(f_out, "MISS...\n\n");

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
        fprintf(f_out, "\n\nL1_ITLB:");
        print_all_tlb_entries(l1_itlb, l1_itlb_entry_t, L1_ITLB_LINES);
        fprintf(f_out, "\n\nL1_DTLB:");
        print_all_tlb_entries(l1_dtlb, l1_dtlb_entry_t, L1_DTLB_LINES);
        fprintf(f_out, "\n\nL2_TLB:");
        print_all_tlb_entries(l2_tlb, l2_tlb_entry_t, L2_TLB_LINES);
#pragma GCC diagnostic pop

        fprintf(f_out, "-------------------------------------------------------------------\n");
    }

    /**
     * Garbage collecting
     */
    fclose(f_out);
    free(mem_space);

    return EXIT_SUCCESS;
}


