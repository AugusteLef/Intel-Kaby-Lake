#pragma once

/**
 * @file list.h
 * @brief Doubly linked lists for PPS-i7Cache project
 */

// for some C99 printf flags like %PRI to compile in Windows
#if defined _WIN32  || defined _WIN64
#define __USE_MINGW_ANSI_STDIO 1
#endif

#include <stdio.h> // for fprintf()
#include <stdint.h> // for uint32_t
#include <inttypes.h> // for PRIx macros

/**
 * @brief List content type
 *
 */
typedef uint32_t list_content_t;
#define print_node(F, N) fprintf(F, "%"PRIu32, N)

/**
 * @brief Doubly linked list type
 *
 */
typedef struct node node_t;
struct list {
    node_t* front;
    node_t* back;
};
typedef struct list list_t;
struct node {
    list_content_t value;
    node_t* previous;
    node_t* next;
};

/**
 * @brief check whether the list is empty or not
 * @param this list to check
 * @return 0 if the list is (well-formed and) not empty
 */
int is_empty_list(const list_t* this);

/**
 * @brief initialize a list to the empty list (like a default constructor)
 * @param this list to initialized
 */
void init_list(list_t* this);

/**
 * @brief clear the whole list (make it empty)
 * @param this list to clear
 */
void clear_list(list_t* this);

/**
 * @brief add a new value at the end of the list
 * @param this list where to add to
 * @param value value to be added
 * @return a pointer to the newly inserted element or NULL in case of error
 */
node_t* push_back(list_t* this, const list_content_t* value);

/**
 * @brief add a new value at the begining of the list
 * @param this list where to add to
 * @param value value to be added
 * @return a pointer to the newly inserted element or NULL in case of error
 */
node_t* push_front(list_t* this, const list_content_t* value);

/**
 * @brief remove the last value
 * @param this list to remove from
 */
void pop_back(list_t* this);

/**
 * @brief remove the first value
 * @param this list to remove from
 */
void pop_front(list_t* this);

/**
 * @brief move a node a the end of the list
 * @param this list to modify
 * @param node pointer to the node to be moved
 */
void move_back(list_t* this, node_t* node);

/**
 * @brief print a list (on one single line, no newline)
 * @param stream where to print to
 * @param this the list to be printed
 * @return number of printed characters
 */
int print_list(FILE* stream, const list_t* this);

/**
 * @brief print a list reversed way
 * @param stream where to print to
 * @param this the list to be printed
 * @return number of printed characters
 */
int print_reverse_list(FILE* stream, const list_t* this);

/**
 * @brief Some useful macros to loop over all nodes of a list.
 * X is the name of the variable to be used for the running node;
 * and L is the list to be looped over.
 * X will be of type `node_t*`
 * and L has to be of type `list_t*`.
 *
 * for_all_nodes() loops from list front to list back, wherease
 * for_all_nodes_reverse() loops from list back to list front.
 *
 * Example usage:
 *    for_all_nodes(node, list) { do_something_with(node); }
 *
 */
#define for_all_nodes(X, L)         for (node_t* X = (L)->front; X != NULL; X = X->next    )
#define for_all_nodes_reverse(X, L) for (node_t* X = (L)->back ; X != NULL; X = X->previous)
