/**
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file	glibhelper-unix-socket-support.h
 * @brief	header for glibhelper-unix-socket-support
 */
#ifndef GLIBHELPER_UINX_SOCKET_SUPPORT_H
#define GLIBHELPER_UINX_SOCKET_SUPPORT_H
//-----------------------------------------------------------------------------
#include <glib.h>
#include <gio/gio.h>


//-----------------------------------------------------------------------------
struct s_glibhelper_unix_socket_server_support;
typedef struct s_glibhelper_unix_socket_server_support *glibhelper_unix_socket_server_support;

typedef void* glibhelper_server_session_handle;


typedef void (*fp_get_new_session_callback_sv)(glibhelper_server_session_handle session); 
typedef gboolean (*fp_receive_callback_sv)(glibhelper_server_session_handle session); 
typedef void (*fp_destroyed_session_callback_sv)(glibhelper_server_session_handle session); 

struct s_glibhelper_server_socket_operation {
	fp_get_new_session_callback_sv get_new_session; /**< Callbuck for sever accepted new session. */
	fp_receive_callback_sv receive; /**< Callbuck for packet receive. */
	fp_destroyed_session_callback_sv destroyed_session; /**< Callbuck for destroyed session. */
};

/** glibhelper_server_socket_config.*/
typedef struct s_glibhelper_server_socket_config {
	struct s_glibhelper_server_socket_operation operation; /**< server socket event handler. */
	int socketbuf_size; /**< server socket buffer size roundup(packet_size * queue). */
	char socket_name[92]; /**< server socket name. abs name or socket file name. */
} glibhelper_server_socket_config;

//-----------------------------------------------------------------------------
struct s_glibhelper_unix_socket_client_support;
typedef struct s_glibhelper_unix_socket_client_support *glibhelper_unix_socket_client_support;

typedef void* glibhelper_client_session_handle;


typedef gboolean (*fp_receive_callback_cl)(glibhelper_client_session_handle session); 
typedef void (*fp_destroyed_session_callback_cl)(glibhelper_client_session_handle session); 

struct s_glibhelper_client_socket_operation {
	fp_receive_callback_cl receive; /**< Callbuck for packet receive. */
	fp_destroyed_session_callback_cl destroyed_session; /**< Callbuck for destroyed session. */
};

/** glibhelper_server_socket_config.*/
typedef struct s_glibhelper_client_socket_config {
	struct s_glibhelper_client_socket_operation operation; /**< server socket event handler. */
	char socket_name[92]; /**< server socket name. abs name or socket file name. */
} glibhelper_client_socket_config;


//-----------------------------------------------------------------------------
gboolean glibhelper_create_server_socket(glibhelper_unix_socket_server_support *handle, GMainContext *context, glibhelper_server_socket_config *config, void *userdata);
gboolean glibhelper_terminate_server_socket(glibhelper_unix_socket_server_support handle);
int glibhelper_server_get_fd(glibhelper_server_session_handle handle);
void* glibhelper_server_get_userdata(glibhelper_server_session_handle handle);
ssize_t glibhelper_server_socket_read(glibhelper_server_session_handle handle, void *buf, size_t count);
ssize_t glibhelper_server_socket_write(glibhelper_server_session_handle handle, void *buf, size_t count);
glibhelper_unix_socket_server_support glibhelper_server_socket_server_support_from_session_handle(glibhelper_server_session_handle handle);
int glibhelper_server_socket_broadcast(glibhelper_unix_socket_server_support handle, void *buf, size_t count);


//-----------------------------------------------------------------------------
gboolean glibhelper_connect_socket(glibhelper_unix_socket_client_support *handle, GMainContext *context, glibhelper_client_socket_config *config, void* userdata);
gboolean glibhelper_terminate_client_socket(glibhelper_unix_socket_client_support handle);
int glibhelper_client_get_fd(glibhelper_client_session_handle handle);
void* glibhelper_client_get_userdata(glibhelper_client_session_handle handle);
ssize_t glibhelper_client_socket_read(glibhelper_client_session_handle handle, void *buf, size_t count);
ssize_t glibhelper_client_socket_write(glibhelper_client_session_handle handle, void *buf, size_t count);


//-----------------------------------------------------------------------------
#endif //#ifndef GLIBHELPER_UINX_SOCKET_SUPPORT_H
