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
	glibhelper_unix_socket_client_support sochandle;
	
} example_data_struct;

char exampledata[1024];
//-----------------------------------------------------------------------------
static gboolean receive_cb(glibhelper_client_session_handle session)
{
	ssize_t ret = 0;

	ret = glibhelper_client_socket_read(session, exampledata, sizeof(exampledata));
	fprintf (stderr, "cli in %ld\n",ret);

	return TRUE;
}
//-----------------------------------------------------------------------------
static void destroyed_session_cb(glibhelper_client_session_handle session)
{
	example_data_struct *ex;
	
	ex = (example_data_struct *)glibhelper_client_get_userdata(session);
	ex->sochandle = NULL;

	fprintf (stderr, "disconnected cb\n");
}
//-----------------------------------------------------------------------------
static gboolean timeout_cb(glibhelper_timerfd_support_handle handle)
{
	ssize_t ret = -1; 
	example_data_struct *ex;
	
	ex = (example_data_struct *)glibhelper_timerfd_get_userdata(handle);

	if (ex->sochandle != NULL) {
		ret = glibhelper_client_socket_write(ex->sochandle,exampledata, sizeof(exampledata)); 
	}
	fprintf (stderr, "timer cb\n");

	return TRUE;
}
//-----------------------------------------------------------------------------
static glibhelper_client_socket_config scfg = {
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
	example_data_struct ex= {NULL};

	glibhelper_unix_socket_client_support sochandle = NULL;;
	glibhelper_timerfd_support_handle timerhandle = NULL;

	gloop = g_main_loop_new(NULL, FALSE);	//get default event loop context
	if (gloop == NULL)
		goto finish;

	bret = glibhelper_siganl_initialize(gloop);
	if (bret == FALSE)
		goto finish;

	scfg.operation.receive = receive_cb;
	scfg.operation.destroyed_session = destroyed_session_cb;

	tcfg.operation.timeout = timeout_cb;

	bret = glibhelper_connect_socket(&sochandle, NULL, &scfg, &ex);
	if (bret != TRUE) {
		fprintf(stderr,"glibhelper_connect_socket error\n");
	}

	ex.sochandle = sochandle;

	bret = glibhelper_create_timerfd(&timerhandle, NULL, &tcfg, &ex);
	if (bret != TRUE) {
		fprintf(stderr,"glibhelper_create_timerfd error\n");
	}

    g_main_loop_run(gloop);

finish:

	if (timerhandle != NULL)
		glibhelper_terminate_timerfd(timerhandle);

	if (ex.sochandle != NULL)
		glibhelper_terminate_client_socket(ex.sochandle);

	fprintf(stderr,"term!!\n");

	return 0;
}
