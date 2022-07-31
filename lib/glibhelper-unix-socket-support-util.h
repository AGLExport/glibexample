/**
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file	glibhelper-unix-socket-support.h
 * @brief	header for glibhelper-unix-socket-support
 */
#ifndef GLIBHELPER_UINX_SOCKET_SUPPORT_UTIL_H
#define GLIBHELPER_UINX_SOCKET_SUPPORT_UTIL_H
//-----------------------------------------------------------------------------
#include <glib.h>
#include <gio/gio.h>



//-----------------------------------------------------------------------------
int glibhelper_calculate_socket_buffer_size(int packet_max_size, int packet_max_num);
int glibhelper_get_socket_name_type(char *str);



//-----------------------------------------------------------------------------
#endif //#ifndef GLIBHELPER_UINX_SOCKET_SUPPORT_UTIL_H
