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
	struct s_glibhelper_unix_socket_client_support *parent;
	GIOChannel *gio_source;
	GSource *event_source;
};

struct s_glibhelper_unix_socket_client_support {
	struct s_gelibhelper_io_channel cli;
	struct s_glibhelper_client_socket_operation operation;
	GMainContext *context;
	void *userdata;
};

/**
 * Get session socket fd from client session handle.
 *
 * @param [in]	handle	Server session handle
 *
 * @return int
 * @retval >=0 fd.
 * @retval <0 error (Illegal handle)
 */

int glibhelper_client_get_fd(glibhelper_client_session_handle handle)
{
	struct s_glibhelper_unix_socket_client_support *helper = NULL;
	int fd = -1;

	if ( handle == NULL)
		return -1;

	helper = (struct s_glibhelper_unix_socket_client_support*)handle;
	fd = g_io_channel_unix_get_fd (helper->cli.gio_source);

	return fd;
}
/**
 * Get userdata from client session handle.
 * The userdata is set at glibhelper_connect_socket.
 *
 * @param [in]	handle	Server session handle
 *
 * @return void*
 * @retval !NULL userdata.
 * @retval NULL userdata is NULL or Illegal handle error.
 */
void* glibhelper_client_get_userdata(glibhelper_client_session_handle handle)
{
	struct s_glibhelper_unix_socket_client_support *helper = NULL;

	if ( handle == NULL)
		return NULL;

	helper = (struct s_glibhelper_unix_socket_client_support*)handle;

	return helper->userdata;
}
/**
 * Read packet from socket using glibhelper_client_session_handle.
 *
 * @param [in]	handle	Client session handle
 * @param [in]	buf Pointer to read buffer.
 * @param [in]	counte Number of bytes for buffer.
 *
 * @return ssize_t
 * @retval >=0 Number of bytes read.
* @retval <0 error (refer to error no).
 */
ssize_t glibhelper_client_socket_read(glibhelper_client_session_handle handle, void *buf, size_t count)
{
	struct s_glibhelper_unix_socket_client_support *helper = NULL;
	ssize_t ret = -1;
	int fd = -1;

	if ( handle == NULL) {
		errno = EINVAL;
		return -1;
	}

	fd = glibhelper_client_get_fd(handle);
	if (fd < 0) {
		errno = EINVAL;
		return -1;
	}

	do {
		ret = read(fd, buf, count);
	} while((ret == -1) && (errno == EINTR));

	return ret;
}
/**
 * Write packet to socket using glibhelper_client_session_handle.
 *
 * @param [in]	handle	Client session handle
 * @param [in]	buf Pointer to write data buffer.
 * @param [in]	counte Number of bytes for buffer.
 *
 * @return ssize_t
 * @retval >=0 Number of bytes read.
 * @retval <0 error (refer to error no).
 */
ssize_t glibhelper_client_socket_write(glibhelper_client_session_handle handle, void *buf, size_t count)
{
	struct s_glibhelper_unix_socket_client_support *helper = NULL;
	ssize_t ret = -1;
	int fd = -1;

	if ( handle == NULL) {
		errno = EINVAL;
		return -1;
	}

	fd = glibhelper_client_get_fd(handle);
	if (fd < 0) {
		errno = EINVAL;
		return -1;
	}

	do {
		ret = write(fd, buf, count);
	} while((ret == -1) && (errno == EINTR));

	return ret;

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
static gboolean clientchannel_socket_event (GIOChannel *source,
								GIOCondition condition,
								gpointer data)
{
	struct s_glibhelper_unix_socket_client_support *helper = NULL;
	gboolean bret = TRUE;
	int sessionfd = -1;

	if (source == NULL || data == NULL)
		return FALSE;// Arg error -> Stop callback Fail safe

	sessionfd = g_io_channel_unix_get_fd (source);
	helper = (struct s_glibhelper_unix_socket_client_support*)data;

	if ((condition & (G_IO_ERR | G_IO_HUP)) != 0) {	 //Client side socket was closed.
		// Cleanup session
		if (helper->operation.destroyed_session != NULL)
			helper->operation.destroyed_session((glibhelper_client_session_handle)helper);

		g_source_destroy(helper->cli.event_source);
		g_io_channel_unref(helper->cli.gio_source);
		g_free(helper);
	} else if ((condition & G_IO_IN) != 0) {	// receive data
		if (helper->operation.receive != NULL)
			bret = helper->operation.receive((glibhelper_client_session_handle)helper);
	} else {	//	G_IO_NVAL or undefined
		bret = FALSE;	// When this event return FALSE, this event watch is disabled
	}

	return bret;
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
gboolean glibhelper_connect_socket(glibhelper_unix_socket_client_support *handle, GMainContext *context, glibhelper_client_socket_config *config, void* userdata)
{
	int clifd = -1;
	int ret = -1;
	int len = 0, connectlen = 0;
	GIOChannel *gcliio = NULL;
	GSource *gclisource = NULL;
	struct sockaddr_un socketinfo;
	struct s_glibhelper_unix_socket_client_support *helper;
	guint id = 0;

	if (handle == NULL || config == NULL)
		return FALSE;

	helper = (struct s_glibhelper_unix_socket_client_support*)g_malloc(sizeof(struct s_glibhelper_unix_socket_client_support));
	if (helper == NULL)
		return FALSE;
	memset(helper,0,sizeof(struct s_glibhelper_unix_socket_client_support));

	clifd = socket(AF_UNIX, SOCK_SEQPACKET|SOCK_CLOEXEC|SOCK_NONBLOCK, AF_UNIX);
	if (clifd < 0) {
		goto errorout;
	}

	memset(&socketinfo, 0, sizeof(socketinfo));
	socketinfo.sun_family = AF_UNIX;

	len = glibhelper_get_socket_name_type(config->socket_name);
	if (len < 0)
		goto errorout;
	else if (len == 0) { //socket file
		strncpy(socketinfo.sun_path, config->socket_name, sizeof(socketinfo.sun_path) - 1);
		connectlen = sizeof(socketinfo);
	} else {
		memcpy(socketinfo.sun_path, config->socket_name, len);
		connectlen = len + sizeof(sa_family_t);
	}

	ret = connect(clifd, (const struct sockaddr *)&socketinfo, connectlen);
	if (ret < 0) {
		goto errorout;
	}

	gcliio = g_io_channel_unix_new (clifd);	// Create gio channel by fd.
	if (gcliio == NULL)
		goto errorout;

	g_io_channel_set_close_on_unref(gcliio,TRUE);	// fd close on final unref
	clifd = -1; // The fd close automatically, do not close myself.

	gclisource = g_io_create_watch(gcliio,(G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL));
	if (gclisource == NULL)
		goto errorout;

	g_source_set_callback(gclisource, (GSourceFunc)clientchannel_socket_event,(gpointer)helper, NULL);

	id = g_source_attach(gclisource, context);

	g_source_unref(gclisource);

	helper->operation = config->operation;
	helper->cli.gio_source = gcliio;
	helper->cli.event_source = gclisource;
	helper->cli.parent = helper;
	helper->context = context;
	helper->userdata = userdata;

	(*handle) = (glibhelper_unix_socket_client_support)(helper);

	return TRUE;

errorout:
	
	if (gcliio != NULL)
		g_io_channel_unref(gcliio);

	if (clifd >= 0)
		close(clifd);

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
gboolean glibhelper_terminate_client_socket(glibhelper_unix_socket_client_support handle)
{
	struct s_glibhelper_unix_socket_client_support *helper = NULL;

	if (handle == NULL)
		return FALSE;// Arg error

	helper = (struct s_glibhelper_unix_socket_client_support *)handle;

	// Destroy server socket
	g_source_destroy(helper->cli.event_source);
	g_io_channel_unref(helper->cli.gio_source);
	g_free(helper);

	return TRUE;
}
