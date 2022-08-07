/**
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file	cluster-service.c
 * @brief	main source file for cluster-service
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "glibhelper-unix-socket-support.h"
#include "glibhelper-unix-socket-support-util.h"
#include "glibhelper-timerfd-support.h"
#include "glibhelper-signal.h"

#include "example-common.h"

#define SOCKET_NAME "/tmp/9Lq7BNBnBycd6nxy.socket"

#include <glib.h>
#include <gio/gio.h>

typedef struct s_example_data_struct_sub {
	glibhelper_unix_socket_client_support clisochandle;
	glibhelper_unix_socket_internal_support insochandle;
} example_data_struct_sub;

typedef struct s_example_data_struct_main {
	glibhelper_unix_socket_internal_support insochandle;
	glibhelper_timerfd_support_handle timerhandle;
} example_data_struct_main;

char exampledata[1024];
//-----------------------------------------------------------------------------
static gboolean receive_cb(glibhelper_client_session_handle session)
{
	ssize_t ret = 0;

	ex_command_t cmd;
	ex_command_str_t *cmdstr = NULL;
	ex_command_int64_t *cmdint64 = NULL;

	ret = glibhelper_client_socket_read(session, &cmd, sizeof(cmd));

	if (cmd.command == EX_COMMAND_SEND_STR) {
		cmdstr = (ex_command_str_t*)&cmd;

		fprintf (stderr, "Cli receive str \"%s\"\n",cmdstr->str);

	} else if (cmd.command == EX_COMMAND_SEND_INT64) {
		cmdint64 = (ex_command_int64_t*)&cmd;

		fprintf (stderr, "Cli receive int64 %ld\n",cmdint64->value1);

	} else
		fprintf (stderr, "command error : cli  (datasize =%ld)\n",ret);

	return TRUE;
}
//-----------------------------------------------------------------------------
static void destroyed_session_cb(glibhelper_client_session_handle session)
{
	example_data_struct_sub *ex;
	
	ex = (example_data_struct_sub *)glibhelper_client_get_userdata(session);
	ex->clisochandle = NULL;

	fprintf (stderr, "disconnected cb\n");
}
//-----------------------------------------------------------------------------
static gboolean timeout_cb(glibhelper_timerfd_support_handle handle)
{
	ssize_t ret = -1; 
	example_data_struct_main *ex;
	
	ex = (example_data_struct_main *)glibhelper_timerfd_get_userdata(handle);

	if (ex->insochandle != NULL) {
		ret = glibhelper_client_socket_write(ex->insochandle,exampledata, sizeof(exampledata)); 
	}
	fprintf (stderr, "timer cb\n");

	return TRUE;
}
//-----------------------------------------------------------------------------
static gboolean receive_cb_in_primary(glibhelper_internal_session_handle session)
{
	fprintf (stderr, "Internal socket primary in \n");

	return TRUE;
}
//-----------------------------------------------------------------------------
static void destroyed_session_cb_in_primary(glibhelper_internal_session_handle session)
{
	fprintf (stderr, "Internal socket disconnected cb primary\n");
}
//-----------------------------------------------------------------------------
static gboolean receive_cb_in_secondary(glibhelper_internal_session_handle session)
{
	ssize_t ret = 0;

	ret = glibhelper_internal_socket_read(session, exampledata, sizeof(exampledata));
	fprintf (stderr, "Internal socket secondary in %ld\n",ret);

	return TRUE;
}
//-----------------------------------------------------------------------------
static void destroyed_session_cb_in_secondary(glibhelper_internal_session_handle session)
{
	fprintf (stderr, "Internal socket disconnected cb secondary\n");
}
//-----------------------------------------------------------------------------
static glibhelper_client_socket_config scfg = {
	//.socket_name = SOCKET_NAME
	.socket_name = "\0/agl/testserver"
};

static 	glibhelper_timerfd_config tcfg = {
	.interval = 1000 * 1000 * 1000 //(ns)
};

glibhelper_internal_socket_config inscfg;
//-----------------------------------------------------------------------------
typedef struct s_sub_thread_data {
	GMainLoop *gsubloop;
	example_data_struct_sub *ex_sub;
} sub_thread_data;
//-----------------------------------------------------------------------------
static gpointer subthread(gpointer data)
{
	sub_thread_data *subdata = (sub_thread_data*)data;

    g_main_loop_run(subdata->gsubloop);

	fprintf(stderr,"subthread exit\n");

	return NULL;
}
//-----------------------------------------------------------------------------
int main (int argc, char **argv)
{
	GMainLoop *gloop = NULL;
	int ret = -1;
	gboolean bret = FALSE;
	example_data_struct_main ex_main = {NULL};
	example_data_struct_sub ex_sub = {NULL};
	GMainContext *subctx = NULL;
	GMainLoop *gsubloop = NULL;
	GThread *sub_thread = NULL;

	glibhelper_unix_socket_client_support sochandle = NULL;
	glibhelper_unix_socket_internal_support insochandle_p = NULL;
	glibhelper_unix_socket_internal_support insochandle_s = NULL;
	glibhelper_timerfd_support_handle timerhandle = NULL;

	sub_thread_data sub_data;

	gloop = g_main_loop_new(NULL, FALSE);	//get default event loop context
	if (gloop == NULL)
		goto finish;

	subctx = g_main_context_new();
	gsubloop = g_main_loop_new(subctx, FALSE);

	bret = glibhelper_siganl_initialize(gloop);
	if (bret == FALSE)
		goto finish;

	scfg.operation.receive = receive_cb;
	scfg.operation.destroyed_session = destroyed_session_cb;

	tcfg.operation.timeout = timeout_cb;

	bret = glibhelper_connect_socket(&sochandle, subctx, &scfg, &ex_sub);
	if (bret != TRUE) {
		fprintf(stderr,"glibhelper_connect_socket error\n");
	}
	ex_sub.clisochandle = sochandle;

	inscfg.socketbuf_size = glibhelper_calculate_socket_buffer_size(2*1024, 16);
	inscfg.operation.receive = receive_cb_in_primary;
	inscfg.operation.destroyed_session = destroyed_session_cb_in_primary;

	bret = glibhelper_create_internal_socket(&insochandle_p, NULL, &inscfg, &ex_main);
	if (bret != TRUE) {
		fprintf(stderr,"glibhelper_create_internal_socket error\n");
	}
	ex_main.insochandle = insochandle_p;

	inscfg.socketbuf_size = glibhelper_calculate_socket_buffer_size(2*1024, 16);
	inscfg.operation.receive = receive_cb_in_secondary;
	inscfg.operation.destroyed_session = destroyed_session_cb_in_secondary;

	bret = glibhelper_bind_secondary_internal_socket(&insochandle_s, insochandle_p, subctx, &inscfg, &ex_sub);
	if (bret != TRUE) {
		fprintf(stderr,"glibhelper_create_internal_socket error\n");
	}
	ex_sub.insochandle = insochandle_s;

	bret = glibhelper_create_timerfd(&timerhandle, NULL, &tcfg, &ex_main);
	if (bret != TRUE) {
		fprintf(stderr,"glibhelper_create_timerfd error\n");
	}
	ex_main.timerhandle = timerhandle;

	sub_data.gsubloop = gsubloop;
	sub_data.ex_sub = &ex_sub;

	sub_thread = g_thread_new("subthread_cli", subthread, &sub_data);

    g_main_loop_run(gloop);

	g_main_loop_quit(gsubloop);
	(void)g_thread_join(sub_thread);
finish:
	if (timerhandle != NULL)
		glibhelper_terminate_timerfd(timerhandle);

	if (ex_sub.insochandle != NULL)
		glibhelper_terminate_internal_socket(ex_sub.insochandle);

	if (ex_main.insochandle != NULL)
		glibhelper_terminate_internal_socket(ex_main.insochandle);

	if (ex_sub.clisochandle != NULL)
		glibhelper_terminate_client_socket(ex_sub.clisochandle);

	g_main_loop_ref(gsubloop);
	g_main_context_unref(subctx);

	fprintf(stderr,"term!!\n");

	return 0;
}
