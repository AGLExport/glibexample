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
	.socket_name = SOCKET_NAME
	//.socket_name = "@/agl/testserver";
};

static 	glibhelper_timerfd_config tcfg = {
	.interval = 1000 * 1000 * 1000 //(ns)
};
//-----------------------------------------------------------------------------
int main (int argc, char **argv)
{
	GMainLoop *gloop = NULL;
	int ret = -1;
	gboolean bret = FALSE;

	glibhelper_unix_socket_support_handle sochandle = NULL;;
	glibhelper_timerfd_support_handle timerhandle = NULL;

	gloop = g_main_loop_new(NULL, FALSE);	//get default event loop context
	if (gloop == NULL)
		goto finish;

	bret = glibhelper_siganl_initialize(gloop);
	if (bret == FALSE)
		goto finish;

	scfg.socketbuf_size = glibhelper_calculate_socket_buffer_size(2*1024, 16);
	scfg.operation.get_new_session = get_new_session_cb;
	scfg.operation.receive = receive_cb;
	scfg.operation.destroyed_session = destroyed_session_cb;

	tcfg.operation.timeout = timeout_cb;

	bret = glibhelper_create_server_socket(&sochandle, NULL, &scfg, NULL);
	if (bret != TRUE) {
		fprintf(stderr,"glibhelper_create_server_socket error\n");
	}

	bret = glibhelper_create_timerfd(&timerhandle, NULL, &tcfg, NULL);
	if (bret != TRUE) {
		fprintf(stderr,"glibhelper_create_timerfd error\n");
	}

    g_main_loop_run(gloop);

finish:

	if (timerhandle != NULL)
		glibhelper_terminate_timerfd(timerhandle);

	if (sochandle != NULL)
		glibhelper_terminate_server_socket(sochandle);

	fprintf(stderr,"term!!\n");

	return 0;
}
