/**
 * @memory.c
 * @brief memory management functions (dump, init from file, etc.)
 */

#if defined _WIN32  || defined _WIN64
#define __USE_MINGW_ANSI_STDIO 1
#endif

#include "memory.h"
#include "page_walk.h"
#include "addr_mng.h"
#include "util.h" // for SIZE_T_FMT
#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h> // for memset()
#include <inttypes.h> // for SCNx macros
#include <assert.h>
#include <ctype.h>


// Declaration of auxiliary functions
int page_file_read(const char* filename, void** memory, pte_t address);
void next_line();

// Define a max for the length of the path of a file
#define MAX_FILE_NAME_SIZE 128


// ======================================================================
/**
 * @brief Tool function to print an address.
 *
 * @param show_addr the format how to display addresses; see addr_fmt_t type in memory.h
 * @param reference the reference address; i.e. the top of the main memory
 * @param addr the address to be displayed
 * @param sep a separator to print after the address (and its colon, printed anyway)
 *
 */
static void address_print(addr_fmt_t show_addr, const void* reference,
                          const void* addr, const char* sep)
{
    switch (show_addr) {
    case POINTER:
        (void)printf("%p", addr);
        break;
    case OFFSET:
        (void)printf("%zX", (const char*)addr - (const char*)reference);
        break;
    case OFFSET_U:
        (void)printf(SIZE_T_FMT, (const char*)addr - (const char*)reference);
        break;
    default:
        // do nothing
        return;
    }
    (void)printf(":%s", sep);
}

// ======================================================================
/**
 * @brief Tool function to print the content of a memory area
 *
 * @param reference the reference address; i.e. the top of the main memory
 * @param from first address to print
 * @param to first address NOT to print; if less that `from`, nothing is printed;
 * @param show_addr the format how to display addresses; see addr_fmt_t type in memory.h
 * @param line_size how many memory byted to print per stdout line
 * @param sep a separator to print after the address and between bytes
 *
 */
static void mem_dump_with_options(const void* reference, const void* from, const void* to,
                                  addr_fmt_t show_addr, size_t line_size, const char* sep)
{
    assert(line_size != 0);
    size_t nb_to_print = line_size;
    for (const uint8_t* addr = from; addr < (const uint8_t*) to; ++addr) {
        if (nb_to_print == line_size) {
            address_print(show_addr, reference, addr, sep);
        }
        (void)printf("%02"PRIX8"%s", *addr, sep);
        if (--nb_to_print == 0) {
            nb_to_print = line_size;
            putchar('\n');
        }
    }
    if (nb_to_print != line_size) putchar('\n');
}

// ======================================================================
// See memory.h for description
int vmem_page_dump_with_options(const void *mem_space, const virt_addr_t* from,
                                addr_fmt_t show_addr, size_t line_size, const char* sep)
{
#ifdef DEBUG
    debug_print("mem_space=%p\n", mem_space);
    (void)fprintf(stderr, __FILE__ ":%d:%s(): virt. addr.=", __LINE__, __func__);
    print_virtual_address(stderr, from);
    (void)fputc('\n', stderr);
#endif
    phy_addr_t paddr;
    zero_init_var(paddr);

    M_EXIT_IF_ERR(page_walk(mem_space, from, &paddr),
                  "calling page_walk() from vmem_page_dump_with_options()");
#ifdef DEBUG
    (void)fprintf(stderr, __FILE__ ":%d:%s(): phys. addr.=", __LINE__, __func__);
    print_physical_address(stderr, &paddr);
    (void)fputc('\n', stderr);
#endif

    const uint32_t paddr_offset = ((uint32_t) paddr.phy_page_num << PAGE_OFFSET);
    const char * const page_start = (const char *)mem_space + paddr_offset;
    const char * const start = page_start + paddr.page_offset;
    const char * const end_line = start + (line_size - paddr.page_offset % line_size);
    const char * const end   = page_start + PAGE_SIZE;
    debug_print("start=%p (offset=%zX)\n", (const void*) start, start - (const char *)mem_space);
    debug_print("end  =%p (offset=%zX)\n", (const void*) end, end   - (const char *)mem_space) ;
    mem_dump_with_options(mem_space, page_start, start, show_addr, line_size, sep);
    const size_t indent = paddr.page_offset % line_size;
    if (indent == 0) putchar('\n');
    address_print(show_addr, mem_space, start, sep);
    for (size_t i = 1; i <= indent; ++i) printf("  %s", sep);
    mem_dump_with_options(mem_space, start, end_line, NONE, line_size, sep);
    mem_dump_with_options(mem_space, end_line, end, show_addr, line_size, sep);
    return ERR_NONE;
}





int mem_init_from_dumpfile(const char* filename, void** memory,
                           size_t* mem_capacity_in_bytes){

    M_REQUIRE_NON_NULL(filename);
    M_REQUIRE_NON_NULL(memory);
    M_REQUIRE_NON_NULL(mem_capacity_in_bytes);

    FILE* file = fopen(filename, "rb");
    M_REQUIRE_NON_NULL_CUSTOM_ERR(file, ERR_IO);
    
    // va tout au bout du fichier
    int seek = fseek(file, 0L, SEEK_END);

    // ### Here an assistant told us to do this 'if' condition to be able to close the file in case of error before sending en ERR_MESSAGE
    // If seek return an erro, close the file and send an erro
    if (seek != 0) {
        int close = fclose(file);
        // Check that the file has been closed correctly, then require seek == 0 to send an error
        M_REQUIRE(close == 0, ERR_BAD_PARAMETER,"%s", "flcose returned an error");    
        M_REQUIRE(seek == 0, ERR_BAD_PARAMETER,"%s", "fseek on file returned an error");
    }

    // indique la position, et donc la taille (en octets)
    *mem_capacity_in_bytes = (size_t) ftell(file);

    // revient au début du fichier (pour le lire par la suite)
    rewind(file);

    // ### Here an assistant told us to do this 'if' condition to be able to close the file in case of error before sending en ERR_MESSAGE
    //Allocate memory
    *memory = calloc(*mem_capacity_in_bytes, 1);
    // Check that the allocation have been well executed, if not close the file
    if (*memory == NULL) {
        int close = fclose(file);
        // Check that the file has been closed correctly, then require *memory is non NULL to send an error
        M_REQUIRE(close == 0, ERR_BAD_PARAMETER,"%s", "flcose returned an error");
        M_REQUIRE_NON_NULL(*memory);
    }

    //Copy bytes into memory
    fread(*memory, 1, *mem_capacity_in_bytes, file);

    int close = fclose(file);
    M_REQUIRE(close == 0, ERR_BAD_PARAMETER,"%s", "flcose returned an error");    

    return ERR_NONE;
}


int mem_init_from_description(const char* master_filename, void** memory,
                              size_t* mem_capacity_in_bytes){
    

    M_REQUIRE_NON_NULL(master_filename);
    M_REQUIRE_NON_NULL(memory);
    M_REQUIRE_NON_NULL(mem_capacity_in_bytes);
    
    //Open master file
    FILE* file = fopen(master_filename, "r");
    M_REQUIRE_NON_NULL_CUSTOM_ERR(file, ERR_IO);    
    
    //=====================================MEMORY============================================
    //Read memory size
    fscanf(file, "%zu", mem_capacity_in_bytes);

    //Go to next line
    next_line(file);
    
    // ### Here an assistant told us to do this 'if' condition to be able to close the file in case of error before sending en ERR_MESSAGE
    //Allocate memory
    *memory = calloc(*mem_capacity_in_bytes, 1);
    if (*memory == NULL) {
        int close = fclose(file);
        // Check that the file has been closed correctly, then require *memory is non NULL to send an error
        M_REQUIRE(close == 0, ERR_BAD_PARAMETER,"%s", "flcose returned an error");
        M_REQUIRE_NON_NULL(*memory);
    }


    //=====================================PGD FILE=========================================
    //Get pdg file name 
    char pgdFileName[MAX_FILE_NAME_SIZE];
    fscanf(file, "%s",pgdFileName); 

    //Read and store pgd file
    int read = page_file_read(pgdFileName, memory, 0);
    // ### Here an assistant told us to do this 'if' condition to be able to close the file before sending en ERR_MESSAGE
    if (read != ERR_NONE) {
        int close = fclose(file);
        // Check that the file has been closed correctly, then require *memory is non NULL to send an error
        M_REQUIRE(close == 0, ERR_BAD_PARAMETER,"%s", "flcose returned an error");
        M_REQUIRE(read == ERR_NONE, ERR_BAD_PARAMETER,"%s", "page_file_read returned an error");
    }

    //Go to next line
    next_line(file);

    //=====================================NB TRANSLATION PAGES==============================
    //Get number of translation pages
    int transPages = 0;
    fscanf(file, "%d", &transPages);

    //Go to next line
    next_line(file);

    //=====================================TRANSLATION PAGES==================================
    //Get and store translation pages
    
    //Declare variables
    char currChar;
    char* fileName;
    word_t addr;
 
    fileName = malloc(MAX_FILE_NAME_SIZE);

    for(size_t i = 0; i < transPages; i++){
        //Get the 0x
        fscanf(file, "%c", &currChar);
        if(currChar != '0') return ERR_IO;
        fscanf(file, "%c", &currChar);
        if(currChar != 'x') return ERR_IO;
        
        //Get address
        fscanf(file, "%08"SCNx32, &addr);

        //Get space
        fscanf(file, "%c", &currChar);
        if(!isspace(currChar)) return ERR_IO;
        
        //Get file name
        fscanf(file, "%s",fileName);

        //Dump file into memory at address indicated
        read = page_file_read(fileName, memory, addr);
        // ### Here an assistant told us to do this 'if' condition to be able to close the file before sending en ERR_MESSAGE
        if (read != ERR_NONE) {
            int close = fclose(file);
            // Check that the file has been closed correctly, then require *memory is non NULL to send an error
            M_REQUIRE(close == 0, ERR_BAD_PARAMETER,"%s", "flcose returned an error");
            M_REQUIRE(read == ERR_NONE, ERR_BAD_PARAMETER,"%s", "page_file_read returned an error");
        }
        
        next_line(file);
    }
    

    //=====================================PAGE FILES=======================================
    //Get and store page files

    //Declare variables
    phy_addr_t paddr;
    uint64_t vaddr64;
    virt_addr_t vaddr;
    
    //Get first char
    fscanf(file, "%c", &currChar);

    while(!feof(file) && !ferror(file)){
        //Get the 0x
        if(currChar != '0') return ERR_IO;
        fscanf(file, "%c", &currChar);
        if(currChar != 'x') return ERR_IO;

        //Get virtual address
        fscanf(file, "%016" SCNx64, &vaddr64);

        //Init virtual address from uint64_t
        int init = init_virt_addr64(&vaddr, vaddr64);
        M_REQUIRE(init == ERR_NONE, ERR_BAD_PARAMETER,"%s", "page_file_read returned an error");

        //Get space
        fscanf(file, "%c", &currChar);
        if(!isspace(currChar)) return ERR_IO;
        
        //Get page file name
        fscanf(file, "%s", fileName);

        //Get physical address
        int pwalk = page_walk(*memory, &vaddr, &paddr);
        M_REQUIRE(pwalk == 0, ERR_BAD_PARAMETER,"%s", "page_walk returned an error");

        //Put page in memory
        read = page_file_read(fileName, memory, (paddr.phy_page_num<<PAGE_OFFSET) | paddr.page_offset);
        M_REQUIRE(read == ERR_NONE, ERR_BAD_PARAMETER,"%s", "page_file_read returned an error");

        //Go to next line and get next character
        next_line(file);
        fscanf(file, "%c", &currChar);
    }

    //FREE POINTERS
    free(fileName);

    return ERR_NONE;
}


int page_file_read(const char* filename, void** memory, pte_t address){


    FILE* file = fopen(filename, "rb");
    M_REQUIRE_NON_NULL_CUSTOM_ERR(file, ERR_IO);

    //4096 kb
    size_t capacity = 4096; // or 512

    // Get file size
    int seek = fseek(file, 0L, SEEK_END);
    M_REQUIRE(seek == 0, ERR_BAD_PARAMETER,"%s", "fseek returned an error");

    size_t sizeFile = (size_t) ftell(file);
    rewind(file);

    //Check if file size greater or equal to page size
    M_REQUIRE(sizeFile == capacity, ERR_IO,"%s", "not a page file");
    
    //Copy bytes into memory
    fread((*memory + address), 1, sizeFile, file);

    int close = fclose(file);
    M_REQUIRE(close == 0, ERR_BAD_PARAMETER,"%s", "page_file_read returned an error");


    return ERR_NONE;
}

//Auxiliary function to get new line
void next_line(FILE* file){
    char curr = fgetc(file);
    while(curr != '\n'){
        curr = fgetc(file);
    }   
}
