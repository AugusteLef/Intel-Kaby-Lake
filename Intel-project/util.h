#pragma once

/**
 * @file util.h
 * @brief Some tool tools/functions
 *
 * @author Jean-Cédric Chappelier
 * @date 2017-2018
 */

/**
 * @brief tag a variable as POTENTIALLY unused, to avoid compiler warnings
 */
#define _unused __attribute__((unused))

/**
 * @brief useful to free pointers to const without warning. Use with care!
 */
#define free_const_ptr(X) free((void*)X)

/**
 * @brief useful to init a variable (typically a struct) directly or through a pointer
 */
#define zero_init_var(X) memset(&X, 0, sizeof(X))
#define zero_init_ptr(X) memset(X, 0, sizeof(*X))

/**
 * @brief useful to have C99 (!) %zu to compile in Windows
 */
#if defined _WIN32  || defined _WIN64
#define SIZE_T_FMT "%u"
#else
#define SIZE_T_FMT "%zu"
#endif
