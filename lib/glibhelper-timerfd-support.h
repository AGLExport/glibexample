/**
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file	glibhelper-timerfd-support.h
 * @brief	header for glibhelper-timerfd-support
 */
#ifndef GLIBHELPER_TIMERFD_SUPPORT_H
#define GLIBHELPER_TIMERFD_SUPPORT_H
//-----------------------------------------------------------------------------
#include <glib.h>
#include <gio/gio.h>

#include <stdint.h>

struct s_glibhelper_timerfd_support;
typedef struct s_glibhelper_timerfd_support *glibhelper_timerfd_support_handle;

//typedef void* glibhelper_server_session_handle;


typedef gboolean (*fp_timeout_callback)(glibhelper_timerfd_support_handle handle); 

struct s_glibhelper_timerfd_operation {
	fp_timeout_callback timeout; /**< Callbuck for timer timeout. */
};

/** glibhelper_server_socket_config.*/
typedef struct s_glibhelper_timerfd_config {
	struct s_glibhelper_timerfd_operation operation;
	uint64_t interval; /**< Interval for periodic timer(ns). */
} glibhelper_timerfd_config;

//-----------------------------------------------------------------------------
gboolean glibhelper_create_timerfd(glibhelper_timerfd_support_handle *handle, GMainContext *context, glibhelper_timerfd_config *config, void *userdata);
gboolean glibhelper_terminate_timerfd(glibhelper_timerfd_support_handle handle);

void* glibhelper_timerfd_get_userdata(glibhelper_timerfd_support_handle handle);

//-----------------------------------------------------------------------------
#endif //#ifndef GLIBHELPER_TIMERFD_SUPPORT_H
