#pragma once

/**
 * @file memory.h
 * @brief Functions for dealing wih the content of the memory (page directories and data).
 */

#include "addr.h"   // for virt_addr_t
#include <stdlib.h> // for size_t and free()

/**
 * @brief enum type to describe how to print address;
 * currently, it's either:
 *          NONE: don't print any address
 *          POINTER: print the actual pointer value
 *          OFFSET:   print the address as an offset value (in hexa)    from the start of the memory space
 *          OFFSET_U: print the address as an offset value (in decimal) from the start of the memory space
 */
enum addr_fmt {
    NONE,
    POINTER,
    OFFSET,
    OFFSET_U,
};
typedef enum addr_fmt addr_fmt_t;


/**
 * @brief Create and initialize the whole memory space from a provided
 * (binary) file containing one single dump of the whole memory space.
 *
 * @param filename the name of the memory dump file to read from
 * @param memory (modified) pointer to the begining of the memory
 * @param mem_capacity_in_bytes (modified) total size of the created memory
 * @return error code, *p_memory shall be NULL in case of error
 *
 */

int mem_init_from_dumpfile(const char* filename, void** memory, size_t* mem_capacity_in_bytes);


/**
 * @brief Create and initialize the whole memory space from a provided
 * (metadata text) file containing an description of the memory.
 * Its format is:
 *  line1:           TOTAL MEMORY SIZE (size_t)
 *  line2:           PGD PAGE FILENAME
 *  line4:           NUMBER N OF TRANSLATION PAGES (PUD+PMD+PTE)
 *  lines5 to (5+N): LIST OF TRANSLATION PAGES, expressed with two info per line:
 *                       INDEX OFFSET (uint32_t in hexa) and FILENAME
 *  remaining lines: LIST OF DATA PAGES, expressed with two info per line:
 *                       VIRTUAL ADDRESS (uint64_t in hexa) and FILENAME
 *
 * @param filename the name of the memory content description file to read from
 * @param memory (modified) pointer to the begining of the memory
 * @param mem_capacity_in_bytes (modified) total size of the created memory
 * @return error code, *p_memory shall be NULL in case of error
 *
 */

int mem_init_from_description(const char* master_filename, void** memory, size_t* mem_capacity_in_bytes);


/**
 * @brief Prints the content of one page from its virtual address.
 * It prints the content reading it as 32 bits integers.
 * @param   mem_space the origin of the memory space simulating the whole memory
 * @param   from the virtual address of the page to print
 * @param   show_addr an option to indicate how to print the address of each printed bloc; see above
 * @param   line_size an option indicating how many 32-bits integers shall be displayed per line
 * @param   sep an option indicating what character string shall be used to separated 32-bits integer values that are printed
 * @return  error code
 */

int vmem_page_dump_with_options(const void *mem_space, const virt_addr_t* from,
                                addr_fmt_t show_addr, size_t line_size, const char* sep);

#define vmem_page_dump(mem, from) vmem_page_dump_with_options(mem, from, OFFSET, 16, " ")

