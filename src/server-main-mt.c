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


#define SOCKET_NAME "/tmp/9Lq7BNBnBycd6nxy.socket"

#include <glib.h>
#include <gio/gio.h>

//-----------------------------------------------------------------------------
static void get_new_session_cb(glibhelper_server_session_handle session)
{
	fprintf (stderr, "connected\n");
}
//-----------------------------------------------------------------------------
static gboolean receive_cb(glibhelper_server_session_handle session)
{
	//dummy read
	uint64_t hoge[4];
	int fd = glibhelper_server_get_fd(session);
	int ret = read(fd, hoge, sizeof(hoge));
	fprintf (stderr, "cli in %d\n",ret);

	return TRUE;
}
//-----------------------------------------------------------------------------
static void destroyed_session_cb(glibhelper_server_session_handle session)
{
	fprintf (stderr, "disconnected cb\n");
}
//-----------------------------------------------------------------------------
static gboolean timeout_cb(glibhelper_timerfd_support_handle handle)
{
	fprintf (stderr, "timer cb\n");

	return TRUE;
}
//-----------------------------------------------------------------------------
static glibhelper_server_socket_config scfg = {
	//.socket_name = SOCKET_NAME
	.socket_name = "\0/agl/testserver"
};

static 	glibhelper_timerfd_config tcfg = {
	.interval = 1000 * 1000 * 1000 //(ns)
};
//-----------------------------------------------------------------------------
typedef struct s_sub_thread_data {
	GMainLoop *gsubloop;
	glibhelper_unix_socket_server_support sochandle;
	glibhelper_timerfd_support_handle timerhandle;
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
	GMainContext *subctx = NULL;
	GMainLoop *gsubloop = NULL;
	GThread *sub_thread = NULL;

	glibhelper_unix_socket_server_support sochandle = NULL;
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

	scfg.socketbuf_size = glibhelper_calculate_socket_buffer_size(2*1024, 16);
	scfg.operation.get_new_session = get_new_session_cb;
	scfg.operation.receive = receive_cb;
	scfg.operation.destroyed_session = destroyed_session_cb;

	tcfg.operation.timeout = timeout_cb;

	bret = glibhelper_create_server_socket(&sochandle, subctx, &scfg, NULL);
	if (bret != TRUE) {
		fprintf(stderr,"glibhelper_create_server_socket error\n");
	}

	bret = glibhelper_create_timerfd(&timerhandle, subctx, &tcfg, NULL);
	if (bret != TRUE) {
		fprintf(stderr,"glibhelper_create_timerfd error\n");
	}

	sub_data.gsubloop = gsubloop;
	sub_data.sochandle = sochandle;
	sub_data.timerhandle = timerhandle;

	sub_thread = g_thread_new("subthread", subthread, &sub_data);

    g_main_loop_run(gloop);

	g_main_loop_quit(gsubloop);
	(void)g_thread_join(sub_thread);
finish:

	if (timerhandle != NULL)
		glibhelper_terminate_timerfd(timerhandle);

	if (sochandle != NULL)
		glibhelper_terminate_server_socket(sochandle);

	fprintf(stderr,"term!!\n");

	return 0;
}
