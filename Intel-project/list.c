#include "list.h"
#include "util.h" // for SIZE_T_FMT
#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h> // for memset()
#include <inttypes.h> // for SCNx macros
#include <assert.h>
#include <ctype.h>


int is_empty_list(const list_t* this){

    M_REQUIRE_NON_NULL(this); 

    if(this->back == NULL && this->front == NULL){
        return 1;
    }
    else if(this->back == NULL || this->front == NULL){
        return ERR_BAD_PARAMETER;
    }
    else{
        return 0;
    }

}


void init_list(list_t* this){

    //There is not value to return (void method) so we decide to not use the M_REQUIRE (as
    //an TA advided us) and just check if the argument is/are NULL, if yes we do nothing
    if(this != NULL){
        this->front = NULL;
        this->back = NULL;
    }
}


void clear_list(list_t* this){

    //There is not value to return (void method) so we decide to not use the M_REQUIRE (as
    //an TA advided us) and just check if the argument is/are NULL, if yes we do nothing
    if(this != NULL){
    
        node_t* current = this->front;
        node_t* temp = NULL;
    
        while(current != NULL){
            temp = current->next;

            //FREE NODE
            current->next = NULL;
            current->previous = NULL;
            current->value = 0;

            free(current);
            current = temp;      
        }

        // ##### NOT NECESSARY --> double check with TA + correction part. 2 so we just put NULL 
        
        //Free the tmp + current variable
        // free(temp);
        temp=NULL;
        // free(current);
        current=NULL; 
    
        //Nullify the list
        this->front = NULL;
        this->back = NULL;

    }
}


node_t* push_back(list_t* this, const list_content_t* value){


    M_REQUIRE_NON_NULL_CUSTOM_ERR(this, NULL);
    M_REQUIRE_NON_NULL_CUSTOM_ERR(value, NULL);

    node_t* added = malloc(sizeof(node_t));
    M_REQUIRE_NON_NULL(added);

    // CASE OF EMPTY LIST
    if(is_empty_list(this)){
        
        //Set front and back
        this->back = added;
        this->front = added;
        
        //Set added node
        added->next = NULL;
        added->previous = NULL;
        added->value = *value;

    } else {

        //Next of last
        this->back->next = added;

        //Set added node
        added->value = *value;
        added->previous = this->back;
        added->next = NULL;

        //Update back  
        this->back = added;

    }
    
    return added;
    
}


node_t* push_front(list_t* this, const list_content_t* value){

    M_REQUIRE_NON_NULL_CUSTOM_ERR(value, NULL);
    M_REQUIRE_NON_NULL_CUSTOM_ERR(this, NULL);

    node_t* added = malloc(sizeof(node_t));
    M_REQUIRE_NON_NULL(added);

    // CASE OF EMPTY LIST
    if(is_empty_list(this)){
        
        //Set front and back
        this->back = added;
        this->front = added;
        
        //Set added node
        added->next = NULL;
        added->previous = NULL;
        added->value = *value;

    } else {

        //Previous of head
        this->front->previous = added;

        //Set added node
        added->value = *value;
        added->next = this->front;
        added->previous = NULL;

        //Update front
        this->front = added;

    }
    
    return added;

}


void pop_front(list_t* this){

    //There is not value to return (void method) so we decide to not use the M_REQUIRE (as
    //an TA advided us) and just check if the argument is/are NULL, if yes we do nothing
    if(this != NULL) {
    //if non empty
        if(!is_empty_list(this)){

            //if more than one node
            if(this->front == this->back){
            
                //Free first element
                this->front->value = 0;
                free(this->front);

                //Make list NULL - WE US INIT TO AVOID REPETION, AS A TA ADVICE US
                init_list(this);


            } else {
            
                //Store new front pointer value
                node_t* newFront = this->front->next;

            
                //Update first element
            
                //this->front->next->previous = NULL;
                newFront->previous = NULL;

                //Free first element
                this->front->next = NULL;
                this->front->value = 0;
                free(this->front);

                //Update front to second element
                this->front = newFront;
            }
        }
    }
    
}


void pop_back(list_t* this){

    //There is not value to return (void method) so we decide to not use the M_REQUIRE (as
    //an TA advided us) and just check if the argument is/are NULL, if yes we do nothing
    if(this != NULL) {

        if(!is_empty_list(this)){

            if(this->front == this->back){
            
                //Free first element
                this->back->value = 0;
                free(this->front);

                //Make list NULL - WE US INIT TO AVOID REPETION, AS A TA ADVICE US
                init_list(this);

            } else {
            
                //Store new back pointer value
                node_t* newBack = this->back->previous;
            
                //Update last element
                newBack->next = NULL;

                //Free last element
                this->back->previous = NULL;
                this->back->value = 0;
                free(this->back);

                //Update front to second element
                this->back = newBack;
            }
        }
    }
    
}


void move_back(list_t* this, node_t* node){

    //There is not value to return (void method) so we decide to not use the M_REQUIRE (as
    //an TA advided us) and just check if the argument is/are NULL, if yes we do nothing
    if(this != NULL) {
        //If non empty list
        if(!is_empty_list(this)){

            // If there is more than 1 element
            if(this->back != this->front){

                //Case element is the first of the list
                if(this->front == node){
                    push_back(this, &node->value);
                    pop_front(this);
                    return;
                }

                //Case element is the last element, nothing to do
                if(node->next == NULL){
                    return;
                }

                
                //Linking adjacent nodes
                node->previous->next = node->next;
                node->next->previous = node->previous;
                

                //Setting new node as new back
                node->next = NULL;
                node->previous = this->back;
                this->back->next = node;

                this->back = node;
            }
        }

    }
}


int print_list(FILE* stream, const list_t* this){

    M_REQUIRE_NON_NULL(this);
    M_REQUIRE_NON_NULL_CUSTOM_ERR(stream,ERR_IO);

    int counter = 0;

    int print = fprintf(stream, "(");
    M_REQUIRE(print == 1, ERR_BAD_PARAMETER,"%s", "the fprintf return a bad value");


    for_all_nodes(current, this){
        counter += 1;
        print_node(stream, current->value);
        
        //Close the brackets when we reach the last node, otherwise print a separation
        if(current->next == NULL) {
            print = fprintf(stream, ")");
            M_REQUIRE(print == 1, ERR_BAD_PARAMETER,"%s", "the fprintf return a bad value");
        } else {
            print = fprintf(stream, ", ");
            M_REQUIRE(print == 1, ERR_BAD_PARAMETER,"%s", "the fprintf return a bad value");
        }
    }

    return counter;
}


int print_reverse_list(FILE* stream, const list_t* this){
    
    M_REQUIRE_NON_NULL(this);
    M_REQUIRE_NON_NULL_CUSTOM_ERR(stream,ERR_IO);

    int counter = 0;

    int print = fprintf(stream, "(");
    M_REQUIRE(print == 1, ERR_BAD_PARAMETER,"%s", "the fprintf return a bad value");


    for_all_nodes_reverse(current, this){
        counter += 1;
        print_node(stream, current->value);
        
        if(current->previous == NULL) {
            print = fprintf(stream, ")");
            M_REQUIRE(print == 1, ERR_BAD_PARAMETER,"%s", "the fprintf return a bad value");

        } else {
            print = fprintf(stream, ", ");
            M_REQUIRE(print == 1, ERR_BAD_PARAMETER,"%s", "the fprintf return a bad value");

        }
    }

    return counter;

}