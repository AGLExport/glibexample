/**
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file	glibhelper-unix-socket-support.c
 * @brief	unix domain socket seq packet helper for glib event loop
 */
#include <glib.h>
#include <gio/gio.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>

#include "glibhelper-unix-socket-support-util.h"
#include "glibhelper-unix-socket-support.h"

struct s_gelibhelper_io_channel {
	struct s_glibhelper_unix_socket_server_support *parent;
	GIOChannel *gio_source;
	GSource *event_source;
};

struct s_glibhelper_unix_socket_server_support {
	struct s_gelibhelper_io_channel server;
	struct s_glibhelper_server_socket_operation operation;
	GMainContext *context;
	void *userdata;
	GList *clientlist;
	int socketbuf_size;
};

/**
 *
 *
 * @param [in]	source	Pointor for active GIOChannel
 * @param [in]	condition	I/O event condition.
 * @param [in]	data	Pointer for 
 *
 * @return gboolean
 * @retval TRUES Success callback or non abnormal error.
 * @retval FALSE Critical error. Callback stop.
 */
static gboolean clientchannel_socket_event (GIOChannel *source,
								GIOCondition condition,
								gpointer data)
{
	struct s_glibhelper_unix_socket_server_support *helper = NULL;
	struct s_gelibhelper_io_channel *session = NULL;
	struct s_gelibhelper_io_channel *del_session = NULL;
	GList* listptr = NULL;
	int sessionfd = -1;

	if (source == NULL || data == NULL)
		return FALSE;// Arg error -> Stop callback Fail safe

	sessionfd = g_io_channel_unix_get_fd (source);
	session = (struct s_gelibhelper_io_channel*)data;
	helper = session->parent;

	if ((condition & (G_IO_ERR | G_IO_HUP)) != 0) {	 //Client side socket was closed.
#if 1	//test imp
		// Cleanup session
		helper->clientlist = g_list_remove(helper->clientlist, session);

		if (helper->operation.destroyed_session != NULL)
			helper->operation.destroyed_session((glibhelper_server_session_handle)session);

		g_source_destroy(session->event_source);
		g_io_channel_unref(session->gio_source);
		g_free(session);
#else
		listptr = g_list_first(helper->clientlist);

		for (int i=0; i < 1000;i++)	{	//loop limit
			if (listptr == NULL) break;

			del_session = (struct s_gelibhelper_io_channel*)listptr->data;

			if (del_session->gio_source == source)
			{
				// Cleanup session
				helper->clientlist = g_list_delete_link(helper->clientlist, listptr);

				if (helper->operation.destroyed_session != NULL)
					helper->operation.destroyed_session((glibhelper_server_session_handle)session);

				g_source_destroy(session->event_source);
				g_io_channel_unref(source);
				g_free(session);
				break;
			} else
				listptr = g_list_next(listptr);
		}
#endif
	} else if ((condition & G_IO_IN) != 0) {	// receive data
		//dummy read
		uint64_t hoge[4];
		int fd = g_io_channel_unix_get_fd (source);
		int ret = read(fd, hoge, sizeof(hoge));
		fprintf (stderr, "cli in %d\n",ret);
		
	} else {	//	G_IO_NVAL or undefined
		return FALSE;	// When this event return FALSE, this event watch is disabled
	}

	return TRUE;
}

/**
 *
 *
 * @param [in]	source	Pointor for active GIOChannel
 * @param [in]	condition	I/O event condition.
 * @param [in]	data	Pointer for 
 *
 * @return gboolean
 * @retval TRUES Success callback or non abnormal error.
 * @retval FALSE Critical error. Callback stop.
 */
static gboolean server_socket_event (GIOChannel *source,
								GIOCondition condition,
								gpointer data)
{
	GIOChannel *new_session_io = NULL;
	GSource *new_session_source = NULL;
	struct s_glibhelper_unix_socket_server_support *helper = NULL;
	struct s_gelibhelper_io_channel *new_session = NULL;
	int serverfd = -1;
	int clifd = -1;
	guint id = 0;

	if (source == NULL || data == NULL)
		return FALSE;// Arg error -> Stop callback Fail safe

	serverfd = g_io_channel_unix_get_fd (source);
	helper = (struct s_glibhelper_unix_socket_server_support*)data;
	
	if ((condition & (G_IO_ERR | G_IO_HUP)) != 0) {	
		//Other side socket was closed. The lisning socket will not active this event.  Fail safe.
		return FALSE;	// When this event return FALSE, this event watch is disabled
	} else if ((condition & G_IO_IN) != 0) {	// in comming connect
		// When this pass get some error, shall cloase new session and wait new connect.
		clifd = accept4(serverfd, NULL, NULL,SOCK_NONBLOCK|SOCK_CLOEXEC);
		if (clifd < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK
				|| errno == ECONNABORTED || errno == EINTR)// Non abnormal error.
				return TRUE;
			else
				return FALSE;
		}

		(void)setsockopt(clifd, SOL_SOCKET, SO_SNDBUF, &helper->socketbuf_size, sizeof(helper->socketbuf_size));

		new_session_io = g_io_channel_unix_new (clifd);
		if (new_session_io == NULL) {
			close(clifd);
			return TRUE;
		}

		g_io_channel_set_close_on_unref(new_session_io,TRUE);	// fd close on final unref

		new_session = (struct s_gelibhelper_io_channel*)g_malloc(sizeof(struct s_gelibhelper_io_channel));
		if (new_session == NULL) {
			g_io_channel_unref(new_session_io);
			return TRUE;
		}
		memset(new_session,0,sizeof(struct s_gelibhelper_io_channel));

		new_session_source = g_io_create_watch(new_session_io,(G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL));
		if (new_session_source == NULL) {
			g_io_channel_unref(new_session_io);
			g_free(new_session);
			return TRUE;
		}

		g_source_set_callback(new_session_source, (GSourceFunc)clientchannel_socket_event,(gpointer) new_session, NULL);

		id = g_source_attach(new_session_source, helper->context);

		new_session->parent = helper;
		new_session->gio_source = new_session_io;
		new_session->event_source = new_session_source;

		helper->clientlist = g_list_append( helper->clientlist, new_session);

		if (helper->operation.get_new_session != NULL)
			helper->operation.get_new_session((glibhelper_server_session_handle)new_session);
	} else {	//	G_IO_NVAL or undefined 
		return FALSE;	// When this event return FALSE, this event watch is disabled
	}

	return TRUE;
}

/**
 *
 *
 * @param [in]	source	Pointor for active GIOChannel
 * @param [in]	condition	I/O event condition.
 * @param [in]	data	Pointer for 
 *
 * @return gboolean
 * @retval TRUES Success callback or non abnormal error.
 * @retval FALSE Critical error. Callback stop.
 */
gboolean glibhelper_create_server_socket(glibhelper_unix_socket_server_support *handle, GMainContext *context, glibhelper_server_socket_config *config, void* userdata)
{
	int serverfd = -1;
	int ret = -1;
	int len = 0, bindlen = 0;
	GIOChannel *gserverio = NULL;
	GSource *gserversource = NULL;
	struct sockaddr_un socketinfo;
	struct s_glibhelper_unix_socket_server_support *helper;
	guint id = 0;

	if (handle == NULL || config == NULL)
		return FALSE;

	helper = (struct s_glibhelper_unix_socket_server_support*)g_malloc(sizeof(struct s_glibhelper_unix_socket_server_support));
	if (helper == NULL)
		return FALSE;
	memset(helper,0,sizeof(struct s_glibhelper_unix_socket_server_support));

	serverfd = socket(AF_UNIX, SOCK_SEQPACKET|SOCK_CLOEXEC|SOCK_NONBLOCK, AF_UNIX);
	if (serverfd < 0) {
		goto errorout;
	}

	memset(&socketinfo, 0, sizeof(socketinfo));
	socketinfo.sun_family = AF_UNIX;

	len = glibhelper_get_socket_name_type(config->socket_name);
	if (len < 0)
		goto errorout;
	else if (len == 0) { //socket file
		strncpy(socketinfo.sun_path, config->socket_name, sizeof(socketinfo.sun_path) - 1);
		unlink(socketinfo.sun_path);
		bindlen = sizeof(socketinfo);
	} else {
		memcpy(socketinfo.sun_path, config->socket_name, len);
		bindlen = len + sizeof(sa_family_t);
	}

	ret = bind(serverfd, (const struct sockaddr *) &socketinfo, bindlen);
	if (ret < 0) {
		goto errorout;
	}

	ret = listen(serverfd, 10);
	if (ret < 0) {
		goto errorout;
	}
	
	gserverio = g_io_channel_unix_new (serverfd);	// Create gio channel by fd.
	if (gserverio == NULL)
		goto errorout;

	g_io_channel_set_close_on_unref(gserverio,TRUE);	// fd close on final unref
	serverfd = -1; // The fd close automatically, do not close myself.

	gserversource = g_io_create_watch(gserverio,(G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL));
	if (gserversource == NULL)
		goto errorout;

	g_source_set_callback(gserversource, (GSourceFunc)server_socket_event,(gpointer)helper, NULL);

	id = g_source_attach(gserversource, context);

	g_source_unref(gserversource);

	helper->operation = config->operation;
	helper->server.gio_source = gserverio;
	helper->server.event_source = gserversource;
	helper->context = context;
	helper->userdata = userdata;
	helper->clientlist = NULL;
	helper->socketbuf_size = config->socketbuf_size;

	(*handle) = (glibhelper_unix_socket_server_support)(helper);

	return TRUE;

errorout:
	
	if (gserverio != NULL)
		g_io_channel_unref(gserverio);

	if (serverfd >= 0)
		close(serverfd);

	g_free(helper);

	return FALSE;
}
/**
 *
 *
 * @param [in]	source	Pointor for active GIOChannel
 * @param [in]	condition	I/O event condition.
 * @param [in]	data	Pointer for 
 *
 * @return gboolean
 * @retval TRUES Success callback or non abnormal error.
 * @retval FALSE Critical error. Callback stop.
 */
gboolean glibhelper_terminate_server_socket(glibhelper_unix_socket_server_support handle)
{
	struct s_glibhelper_unix_socket_server_support *helper = NULL;
	struct s_gelibhelper_io_channel *session = NULL;
	GList* listptr = NULL;
	int sessionfd = -1;

	if (handle == NULL)
		return FALSE;// Arg error

	helper = (struct s_glibhelper_unix_socket_server_support*)handle;

	// Destroy all session
	listptr = g_list_first(helper->clientlist);

	for (int i=0; i < 1000;i++)	{	//loop limit
		if (listptr == NULL) break;

		session = (struct s_gelibhelper_io_channel*)listptr->data;

		if (session != NULL) {
			// Cleanup session
			if (helper->operation.destroyed_session != NULL)
				helper->operation.destroyed_session((glibhelper_server_session_handle)session);

			g_source_destroy(session->event_source);
			g_io_channel_unref(session->gio_source);
			g_free(session);
		}
		listptr = g_list_next(listptr);
	}
	g_list_free(helper->clientlist);

	// Destroy server socket
	g_source_destroy(helper->server.event_source);
	g_io_channel_unref(helper->server.gio_source);
	g_free(helper);

	return TRUE;
}
