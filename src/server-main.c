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

typedef struct s_example_data_struct {
	glibhelper_unix_socket_server_support sochandle;
	glibhelper_timerfd_support_handle timerfd;
} example_data_struct;

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

	ssize_t ret = glibhelper_server_socket_read(session, hoge, sizeof(hoge));
	fprintf (stderr, "cli in %ld\n",ret);

	ssize_t ret2 = glibhelper_server_socket_write(session, hoge, sizeof(hoge));
	fprintf (stderr, "cli to %ld\n",ret);

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
	uint64_t bb[2];
	example_data_struct *ex = NULL;
	void *ptr = NULL;
	int ret = -1;

	fprintf (stderr, "timer cb\n");

	ptr = glibhelper_timerfd_get_userdata(handle);
	if (ptr != NULL) {
		ex = (example_data_struct*)ptr;
		ret = glibhelper_server_socket_broadcast(ex->sochandle, bb, sizeof(bb));
		fprintf (stderr, "broad cast to %d clients\n",ret);
	}

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
int main (int argc, char **argv)
{
	GMainLoop *gloop = NULL;
	int ret = -1;
	gboolean bret = FALSE;
	example_data_struct ex = {0};

	glibhelper_unix_socket_server_support sochandle = NULL;;
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

	bret = glibhelper_create_server_socket(&sochandle, NULL, &scfg, &ex);
	if (bret != TRUE) {
		fprintf(stderr,"glibhelper_create_server_socket error\n");
	}
	ex.sochandle = sochandle;

	bret = glibhelper_create_timerfd(&timerhandle, NULL, &tcfg, &ex);
	if (bret != TRUE) {
		fprintf(stderr,"glibhelper_create_timerfd error\n");
	}
	ex.timerfd = timerhandle;

    g_main_loop_run(gloop);

finish:

	if (ex.timerfd != NULL)
		glibhelper_terminate_timerfd(ex.timerfd);

	if (ex.sochandle != NULL)
		glibhelper_terminate_server_socket(ex.sochandle);

	fprintf(stderr,"term!!\n");

	return 0;
}
