#include "tlb_mng.h"
#include "util.h" // for SIZE_T_FMT
#include "error.h"
#include "page_walk.h"
#include "addr_mng.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h> // for memset()
#include <inttypes.h> // for SCNx macros
#include <assert.h>
#include <ctype.h>

int tlb_flush(tlb_entry_t * tlb){

    M_REQUIRE_NON_NULL(tlb);

    //Loop throught tlb entries and set to 0 all entries
    for(size_t i = 0; i < TLB_LINES; i++){
        tlb[i].v = 0;
        tlb[i].tag = 0;
        tlb[i].phy_page_num = 0;
    }
    
    return ERR_NONE;
}


int tlb_entry_init( const virt_addr_t * vaddr,
                    const phy_addr_t * paddr,
                    tlb_entry_t * tlb_entry){

        M_REQUIRE_NON_NULL(paddr);
        M_REQUIRE_NON_NULL(tlb_entry);
        M_REQUIRE_NON_NULL(vaddr);


        //Copy function arguments to the tlb_entry + active the validation bit 
        tlb_entry->tag = virt_addr_t_to_virtual_page_number(vaddr);
        tlb_entry->phy_page_num = (paddr->phy_page_num);
        tlb_entry->v = 1;

        return ERR_NONE;

}



int tlb_insert(uint32_t line_index,
                const tlb_entry_t * tlb_entry,
                tlb_entry_t * tlb){

        M_REQUIRE_NON_NULL(tlb);
        M_REQUIRE_NON_NULL(tlb_entry);
        // No need to check >= 0 since line_index is a UINT (Correction part. 2)
        M_REQUIRE(line_index < TLB_LINES, ERR_BAD_PARAMETER, "%s", "The index is not in the bound");

        //insert a the correct place of the tlb the tlb_entry
        tlb[line_index] = *tlb_entry;
        
        return ERR_NONE;
}





int tlb_hit(const virt_addr_t * vaddr,
            phy_addr_t * paddr,
            const tlb_entry_t * tlb,
            replacement_policy_t * replacement_policy){

    //Return 0 in case of non-valid argument (see pdf)
    if(vaddr == NULL || paddr == NULL || tlb == NULL || replacement_policy == NULL){
        return 0;
    }
    else{

        // Get the virt_page_num and offset 
        uint64_t virt_page_num = virt_addr_t_to_virtual_page_number(vaddr);
        uint16_t offset = vaddr->page_offset;
        
        //Iteration on all node (from end to start) and check if one of them correspont to
        //the one we are searching (right tag + valid)
        for_all_nodes_reverse(node, replacement_policy->ll) {
            if(virt_page_num == tlb[node->value].tag && 1 == tlb[node->value].v) {
                paddr->phy_page_num = tlb[node->value].phy_page_num;
                paddr->page_offset = offset;
                replacement_policy->move_back(replacement_policy->ll, node);
                return 1;
            }
        }
        return 0;
    }
}


int tlb_search( const void * mem_space,
                const virt_addr_t * vaddr,
                phy_addr_t * paddr,
                tlb_entry_t * tlb,
                replacement_policy_t * replacement_policy,
                int* hit_or_miss){


        M_REQUIRE_NON_NULL(mem_space);
        M_REQUIRE_NON_NULL(vaddr);
        M_REQUIRE_NON_NULL(paddr);
        M_REQUIRE_NON_NULL(tlb);
        M_REQUIRE_NON_NULL(replacement_policy);
        M_REQUIRE_NON_NULL(hit_or_miss);
        // Check if it's a MISS or an HIT
        *hit_or_miss = tlb_hit(vaddr, paddr, tlb, replacement_policy);
        
        //If it's a MISS we do the following block, else there is nothing to do 
        if(*hit_or_miss == 0){
            int err = page_walk(mem_space, vaddr, paddr);
            if(err == ERR_NONE){
                //Create and init a new TLB entry
                // (We check the return value of the malloc + )
                tlb_entry_t * entry = malloc(sizeof(tlb_entry_t));
                M_REQUIRE_NON_NULL(entry);
                int initErr = tlb_entry_init(vaddr, paddr, entry);
                M_REQUIRE(initErr == 0, ERR_BAD_PARAMETER, "%s", ERR_MESSAGE[ERR_BAD_PARAMETER]);
                int insertErr = tlb_insert(replacement_policy->ll->front->value, entry, tlb);
                M_REQUIRE(insertErr == 0, ERR_BAD_PARAMETER, "%s", ERR_MESSAGE[ERR_BAD_PARAMETER]);
                replacement_policy->move_back(replacement_policy->ll, replacement_policy->ll->front);
                free(entry);

                return ERR_NONE;
            } else {
                return err;
            }
        }
        
        return ERR_NONE;
}