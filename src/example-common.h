/**
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file	example-common.h
 * @brief	common header for glibhelper example process
 */
#ifndef EXAMPLE_COMMON_H
#define EXAMPLE_COMMON_H
//-----------------------------------------------------------------------------
#include <stdint.h>

//-----------------------------------------------------------------------------
#define EX_COMMAND_SEND_STR (0)
#define	EX_COMMAND_SEND_INT64 (1) 

//-----------------------------------------------------------------------------
typedef struct s_ex_command {
	int64_t command;
	int8_t data[2040];
} ex_command_t;

typedef struct s_ex_command_str {
	int64_t command;
	char str[2040];
} ex_command_str_t;

typedef struct s_ex_command_int64 {
	int64_t command;
	int64_t value1;
	int64_t value2;
	int64_t value3;
	int64_t value4;
} ex_command_int64_t;

//-----------------------------------------------------------------------------
#endif //#ifndef EXAMPLE_COMMON_H
