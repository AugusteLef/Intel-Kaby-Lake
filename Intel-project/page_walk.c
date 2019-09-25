#include "page_walk.h"
#include <stdio.h>
#include "error.h"
#include "inttypes.h"
#include <stdlib.h>
#include "addr_mng.h"

static inline pte_t read_page_entry(const pte_t * start, 
                                    pte_t page_start,
                                    uint16_t index); 


int page_walk(const void* mem_space, const virt_addr_t* vaddr, phy_addr_t* paddr){

    M_REQUIRE_NON_NULL(mem_space);
    M_REQUIRE_NON_NULL(vaddr);
    M_REQUIRE_NON_NULL(paddr);

    //Walk through pages
    pte_t pudTabAddress = read_page_entry(mem_space, 0, vaddr->pgd_entry);
    M_REQUIRE(pudTabAddress%4096 == 0, ERR_BAD_PARAMETER, "%s", "Address of the page pud is false");

    pte_t pmdTabAddress = read_page_entry(mem_space, pudTabAddress, vaddr->pud_entry);
    M_REQUIRE(pmdTabAddress%4096 == 0, ERR_BAD_PARAMETER, "%s", "Address of the page pmd is false");

    pte_t pteTabAddress = read_page_entry(mem_space, pmdTabAddress, vaddr->pmd_entry);
    M_REQUIRE(pteTabAddress%4096 == 0, ERR_BAD_PARAMETER, "%s", "Address of the page pte is false");

    pte_t physical = read_page_entry(mem_space, pteTabAddress, vaddr->pte_entry);
    M_REQUIRE(physical%4096 == 0, ERR_BAD_PARAMETER, "%s", "Address of the physcal page is false");

    //Init physical address (+ no need to check the return value because we actually directly return it)
    return init_phy_addr(paddr, physical, vaddr->page_offset);
    
}


static inline pte_t read_page_entry(const pte_t * start, 
                                    pte_t page_start,
                                    uint16_t index){ 
                                        
    return start[page_start/sizeof(pte_t) + index];
}
