#include "tlb_hrchy_mng.h"
#include "tlb_hrchy.h"
#include "mem_access.h"
#include "addr.h"
#include "addr_mng.h"
#include "error.h"
#include "page_walk.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h> // for memset()
#include <inttypes.h> // for SCNx macros

// Define mask 
#define mask6bit 63
#define mask4bit 15


// DECLARATION OF OUR AUXILIARY METHODS (insert_l1 and invalidation) and macro


int insert_l1(l1_itlb_entry_t * l1_itlb,
              l1_dtlb_entry_t * l1_dtlb,
              uint64_t virtual_page_number,
              phy_addr_t * paddr,
              mem_access_t access);

int invalidation(l1_itlb_entry_t * l1_itlb,
              l1_dtlb_entry_t * l1_dtlb,
              uint64_t virtual_page_number,
              phy_addr_t * paddr,
              mem_access_t access,
              l2_tlb_entry_t * l2_tlb);

#define init(TYPE,LINES) \
   TYPE* VAR = (TYPE*) tlb_entry;\
                VAR->v = 1;\
                VAR->phy_page_num = paddr->phy_page_num;\
                VAR->tag = (virtual_page_number >> LINES);
    
#define flush(TYPE,LINES) \
   TYPE* VAR = (TYPE*) tlb;\
        for(size_t i = 0; i < LINES; i++){ \
            VAR[i].v = 0; \
            VAR[i].tag = 0; \
            VAR[i].phy_page_num = 0; \
        }

#define insert(TYPE, LINES) \
    TYPE* VAR_TLB = (TYPE*) tlb; \
        TYPE* VAR_ENTRY = (TYPE*) tlb_entry; \
        VAR_TLB[line_index] = *VAR_ENTRY; 
    
#define hit(TYPE, LINES) \
  TYPE* VAR = (TYPE*) tlb; \
        uint32_t tag = addr >> LINES; \
        uint32_t index = addr & mask4bit; \
        if ((tag == VAR[index].tag) && (1 == VAR[index].v)) { \
            paddr->phy_page_num = VAR[index].phy_page_num; \
            paddr->page_offset = offset; \
            return 1; \
        } \
        else{ \
            return 0; \
        }
                


int tlb_entry_init( const virt_addr_t * vaddr,
                    const phy_addr_t * paddr,
                    void * tlb_entry,
                    tlb_t tlb_type){

        
        M_REQUIRE_NON_NULL(tlb_entry);
        M_REQUIRE_NON_NULL(vaddr);
        M_REQUIRE_NON_NULL(paddr);
        //The check of the tlb_type is done in the switch ***
        uint64_t virtual_page_number = virt_addr_t_to_virtual_page_number(vaddr); 

        //Depending the tlb_type, we cast the tlb_entry into the right one and initialize it with the correct values
        if(tlb_type == L1_DTLB){
            init(l1_dtlb_entry_t, L1_DTLB_LINES_BITS);
        } else if(tlb_type == L1_ITLB){
            init(l1_itlb_entry_t, L1_ITLB_LINES_BITS);
        } else if(tlb_type == L2_TLB){
            init(l2_tlb_entry_t, L2_TLB_LINES_BITS);
        } else{
            return ERR_BAD_PARAMETER;
        }

        return ERR_NONE;
}



int tlb_flush(void *tlb, tlb_t tlb_type){
        
        M_REQUIRE_NON_NULL(tlb);
        //The check of the tlb_type is done in the switch ***

        // Find the correct type of tlb and put every value of the tlb_entries in there to 0
        if(tlb_type == L1_DTLB){
            flush(l1_dtlb_entry_t, L1_DTLB_LINES_BITS);
        } else if(tlb_type == L1_ITLB){
            flush(l1_itlb_entry_t, L1_ITLB_LINES_BITS);
        } else if(tlb_type == L2_TLB){
            flush(l2_tlb_entry_t, L2_TLB_LINES_BITS);
        } else{
            return ERR_BAD_PARAMETER;
        }

        return ERR_NONE;
}


int tlb_insert(uint32_t line_index,
                const void * tlb_entry,
                void * tlb,
                tlb_t tlb_type){

        
        M_REQUIRE_NON_NULL(tlb_entry);
        M_REQUIRE_NON_NULL(tlb);
        //The check of the tlb_type is done in the switch ***


        // Depending the type, do the appropriate caste + add the entry in the tlb add the right index
        if(tlb_type == L1_DTLB){
            M_REQUIRE(line_index < L1_DTLB_LINES, ERR_BAD_PARAMETER,"%s", "invalid line index");
            insert(l1_dtlb_entry_t, L1_DTLB_LINES);
        } else if(tlb_type == L1_ITLB){
            M_REQUIRE(line_index < L1_ITLB_LINES, ERR_BAD_PARAMETER,"%s", "invalid line index");
            insert(l1_itlb_entry_t, L1_ITLB_LINES);
        } else if(tlb_type == L2_TLB){
            M_REQUIRE(line_index < L2_TLB_LINES, ERR_BAD_PARAMETER,"%s", "invalid line index");
            insert(l2_tlb_entry_t, L2_TLB_LINES);
        } else{
            return ERR_BAD_PARAMETER;
        }

        return ERR_NONE;

}


int tlb_hit( const virt_addr_t * vaddr,
             phy_addr_t * paddr,
             const void  * tlb,
             tlb_t tlb_type){

        if (vaddr == NULL || paddr == NULL || tlb == NULL) {
            return 0;
        }
        
        uint64_t addr = virt_addr_t_to_virtual_page_number(vaddr);
        uint32_t offset = vaddr->page_offset;

        if(tlb_type == L1_DTLB){
            hit(l1_dtlb_entry_t, L1_DTLB_LINES_BITS);
        } else if(tlb_type == L1_ITLB){
            hit(l1_itlb_entry_t, L1_ITLB_LINES_BITS);
        } else if(tlb_type == L2_TLB){
            hit(l2_tlb_entry_t, L2_TLB_LINES_BITS);
        } else{
            return 0;
        }
        

}

int tlb_search( const void * mem_space,
                const virt_addr_t * vaddr,
                phy_addr_t * paddr,
                mem_access_t access,
                l1_itlb_entry_t * l1_itlb,
                l1_dtlb_entry_t * l1_dtlb,
                l2_tlb_entry_t * l2_tlb,
                int* hit_or_miss){


        M_REQUIRE_NON_NULL(vaddr);
        M_REQUIRE_NON_NULL(mem_space);
        M_REQUIRE_NON_NULL(paddr);
        M_REQUIRE_NON_NULL(l1_itlb);
        M_REQUIRE_NON_NULL(l1_dtlb);
        M_REQUIRE_NON_NULL(l2_tlb);
        M_REQUIRE_NON_NULL(hit_or_miss);


        switch (access)
        {
            case INSTRUCTION:
                *hit_or_miss = tlb_hit(vaddr, paddr, l1_itlb, L1_ITLB);        
                break;
            case DATA:
                *hit_or_miss = tlb_hit(vaddr, paddr, l1_dtlb, L1_DTLB);
                break;

            default:
                return ERR_BAD_PARAMETER;
        }

        if(*hit_or_miss == 1){
            return ERR_NONE;
        } else {
            
            *hit_or_miss = tlb_hit(vaddr, paddr, l2_tlb, L2_TLB);

            uint64_t addr = virt_addr_t_to_virtual_page_number(vaddr);
            
            //Get index and tag from vaddr                
            if (*hit_or_miss == 1) {
                    return insert_l1(l1_itlb, l1_dtlb, addr, paddr, access);
            } else {
                int err = page_walk(mem_space, vaddr, paddr);
                if(err == ERR_NONE){

                    //INVALIDATION REQUIREMENT
                    int errEvictor = invalidation(l1_itlb, l1_dtlb, addr, paddr, access, l2_tlb);
                    M_REQUIRE(errEvictor == ERR_NONE, errEvictor,"%s", "Evition error");

                    //INSERT IN L2
                    l2_tlb_entry_t* entry_l2 = malloc(sizeof(l2_tlb_entry_t));
                    M_REQUIRE_NON_NULL(entry_l2);
                    entry_l2->v = 1;
                    uint32_t tag_l2 = addr >> L2_TLB_LINES_BITS;
                    entry_l2->tag = tag_l2;
                    entry_l2->phy_page_num = paddr->phy_page_num;

                    uint32_t index_l2 = addr & mask6bit;
                    int err_l2 = tlb_insert(index_l2, entry_l2, l2_tlb, L2_TLB);
                    M_REQUIRE(err_l2 == ERR_NONE, err_l2,"%s", "Error in insert l2");

                    //free the entry_l2
                    free(entry_l2);

                    //INSERT IN L1
                    return insert_l1(l1_itlb, l1_dtlb, addr, paddr, access);
                }
                else {
                    return err;
                }
            }
            

        }
        
        return ERR_NONE;

}


// Name of variable are explicit
// @param      l1_itlb              
// @param      l1_dtlb              
// @param[in]  virtual_page_number 
// @param      paddr                
// @param[in]  access  
// 
// @brief       This function will insert an entry (compute from vaddr and paddr) in the correct l1_tlb     (I or D)         
//
// @return     ERROR CODE DEPENDING OF THE BEHAVIOUR OF THE METHOD
//
int insert_l1(l1_itlb_entry_t * l1_itlb,
              l1_dtlb_entry_t * l1_dtlb,
              uint64_t virtual_page_number,
              phy_addr_t * paddr,
              mem_access_t access){

                M_REQUIRE_NON_NULL(l1_dtlb);
                M_REQUIRE_NON_NULL(l1_itlb);
                M_REQUIRE_NON_NULL(paddr);
                // We directly check the 'access' in the switch

                l1_dtlb_entry_t* entryL1D = NULL;
                l1_itlb_entry_t* entryL1I = NULL;
                uint32_t tag, index = 0;


                switch (access) 
                {
                    case INSTRUCTION:
                        tag = virtual_page_number >> L1_ITLB_LINES_BITS;
                        index = virtual_page_number & mask4bit;
                        
                        entryL1I = malloc(sizeof(l1_itlb_entry_t));
                        entryL1I->tag = tag;
                        entryL1I->v = 1;
                        entryL1I->phy_page_num = paddr->phy_page_num;

                        return tlb_insert(index, entryL1I, l1_itlb, L1_ITLB);
                        break;

                    case DATA:
                        tag = virtual_page_number >> L1_DTLB_LINES_BITS;
                        index = virtual_page_number & mask4bit;

                        entryL1D = malloc(sizeof(l1_dtlb_entry_t));
                        entryL1D->tag = tag;
                        entryL1D->v = 1;
                        entryL1D->phy_page_num = paddr->phy_page_num;

                        return tlb_insert(index, entryL1D, l1_dtlb, L1_DTLB);
                        break;

                    default:
                        return ERR_BAD_PARAMETER;
                        break;
                }

}


/**
 * @brief     This function takes care of the eviction of an entry in a l1_tlb (I or D) if needeed 
 *
 * NAME ARE EXPLICIT
 * @param      l1_itlb              The l 1 itlb
 * @param      l1_dtlb              The l 1 dtlb
 * @param[in]  virtual_page_number  The virtual page number
 * @param      paddr                The paddr
 * @param[in]  access               The access
 * @param      l2_tlb               The l 2 tlb
 *
 * @return     ERROR CODE DEPENDING OF THE BEHAVIOUR OF THE METHOD
 */
int invalidation(l1_itlb_entry_t * l1_itlb,
              l1_dtlb_entry_t * l1_dtlb,
              uint64_t virtual_page_number,
              phy_addr_t * paddr,
              mem_access_t access,
              l2_tlb_entry_t * l2_tlb){

                M_REQUIRE_NON_NULL(l1_dtlb);
                M_REQUIRE_NON_NULL(l1_itlb);
                M_REQUIRE_NON_NULL(l2_tlb);
                M_REQUIRE_NON_NULL(paddr);
                // 'access' is directly check on the switch (default case => error)

                uint32_t tag1, index1, tag2Older, index2, oldVaddr1, oldVaddr2 = 0;

                //We store the index of the tlb2 corresponding to the virtu_p_number
                index2 = virtual_page_number & mask6bit;
                //We store the tag that is a this index (before the insertion of the new one)
                tag2Older = l2_tlb[index2].tag;
                //We recreate a part of the vaddr by concatenate the tag and the index (usefull to then compare with what we found in tlb1)
                oldVaddr2 = (tag2Older << L2_TLB_LINES_BITS) | index2;

                //First we chosse in which tlb we need to evice something (maybe) 
                // Then we get the index and the tag of the correct tlb_entry in the correct tlb and concatenate to obtain a part of the original vaddr
                // Then if this 'vaddr' match with the one of the l2TLB, and that the validation bit is 1 : we put the validation bit to 0
                switch (access) 
                {
                    case INSTRUCTION: 
                        index1 = virtual_page_number & mask4bit;
                        tag1 = l1_dtlb[index1].tag;
                        oldVaddr1 = (tag1 << L1_DTLB_LINES_BITS) | index1;
                        
                        if(1==l1_dtlb[index1].v && oldVaddr1 == oldVaddr2) {
                            l1_dtlb[index1].v = 0;
                        }
                        
                        return ERR_NONE;

                    case DATA:
                        index1 = virtual_page_number & mask4bit;
                        tag1 = l1_itlb[index1].tag;
                        oldVaddr1 = (tag1 << L1_ITLB_LINES_BITS) | index1;
                        
                        if (1==l1_itlb[index1].v && oldVaddr1 == oldVaddr2){
                            l1_itlb[index1].v = 0;
                        }
                        
                        return ERR_NONE;

                    default:
                        return ERR_BAD_PARAMETER;
                        break;
                }

}