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

enum socket_internal_side {
	//! Primary side of socket pair channel
	PRIMARY_SIDE = 0,

	//! Secoundary side of socket pair channel
	SECOUNDARY_SIDE = 1,
};

struct s_gelibhelper_io_channel {
	struct s_glibhelper_unix_socket_internal_support *parent;
	GIOChannel *gio_source;
	GSource *event_source;
};

struct s_glibhelper_unix_socket_internal_support {
	struct s_gelibhelper_io_channel io;
	struct s_glibhelper_internal_socket_operation operation;
	GMainContext *context;
	void *userdata;
	enum socket_internal_side side;
	int socketbuf_size;
	int primary_fd;
	int secondary_fd;
	gboolean secondary_fd_leased;
};
/**
 * Get session socket fd from internal session handle.
 *
 * @param [in]	handle	Server session handle
 *
 * @return int
 * @retval >=0 fd.
 * @retval <0 error (Illegal handle)
 */

int glibhelper_internal_get_fd(glibhelper_internal_session_handle handle)
{
	struct s_glibhelper_unix_socket_internal_support *helper = NULL;
	int fd = -1;

	if ( handle == NULL)
		return -1;

	helper = (struct s_glibhelper_unix_socket_internal_support*)handle;
	fd = g_io_channel_unix_get_fd (helper->io.gio_source);

	return fd;
}
/**
 * Get userdata from internal session handle.
 * The userdata is set at glibhelper_connect_socket.
 *
 * @param [in]	handle	Server session handle
 *
 * @return void*
 * @retval !NULL userdata.
 * @retval NULL userdata is NULL or Illegal handle error.
 */
void* glibhelper_internal_get_userdata(glibhelper_internal_session_handle handle)
{
	struct s_glibhelper_unix_socket_internal_support *helper = NULL;

	if ( handle == NULL)
		return NULL;

	helper = (struct s_glibhelper_unix_socket_internal_support*)handle;

	return helper->userdata;
}
/**
 * Read packet from socket using glibhelper_internal_session_handle.
 *
 * @param [in]	handle	Internal session handle
 * @param [in]	buf Pointer to read buffer.
 * @param [in]	counte Number of bytes for buffer.
 *
 * @return ssize_t
 * @retval >=0 Number of bytes read.
* @retval <0 error (refer to error no).
 */
ssize_t glibhelper_internal_socket_read(glibhelper_internal_session_handle handle, void *buf, size_t count)
{
	struct s_glibhelper_unix_socket_internal_support *helper = NULL;
	ssize_t ret = -1;
	int fd = -1;

	if ( handle == NULL) {
		errno = EINVAL;
		return -1;
	}

	fd = glibhelper_internal_get_fd(handle);
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
 * Write packet to socket using glibhelper_internal_session_handle.
 *
 * @param [in]	handle	Internal session handle
 * @param [in]	buf Pointer to write data buffer.
 * @param [in]	counte Number of bytes for buffer.
 *
 * @return ssize_t
 * @retval >=0 Number of bytes read.
 * @retval <0 error (refer to error no).
 */
ssize_t glibhelper_internal_socket_write(glibhelper_internal_session_handle handle, void *buf, size_t count)
{
	struct s_glibhelper_unix_socket_internal_support *helper = NULL;
	ssize_t ret = -1;
	int fd = -1;

	if ( handle == NULL) {
		errno = EINVAL;
		return -1;
	}

	fd = glibhelper_internal_get_fd(handle);
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
static gboolean internalchannel_socket_event (GIOChannel *source,
								GIOCondition condition,
								gpointer data)
{
	struct s_glibhelper_unix_socket_internal_support *helper = NULL;
	gboolean bret = TRUE;
	int sessionfd = -1;

	if (source == NULL || data == NULL)
		return FALSE;// Arg error -> Stop callback Fail safe

	sessionfd = g_io_channel_unix_get_fd (source);
	helper = (struct s_glibhelper_unix_socket_internal_support*)data;

	if ((condition & (G_IO_ERR | G_IO_HUP)) != 0) {	 //Socket was closed.
		// Cleanup session
		if (helper->operation.destroyed_session != NULL)
			helper->operation.destroyed_session((glibhelper_client_session_handle)helper);

		g_source_destroy(helper->io.event_source);
		g_io_channel_unref(helper->io.gio_source);

		if (helper->side == PRIMARY_SIDE) {
			if (helper->secondary_fd_leased == FALSE) // If secondary_fd is not leased, it is closed in detect primary side disconnect.
				(void)close(helper->secondary_fd);
		}

		g_free(helper);
	} else if ((condition & G_IO_IN) != 0) {	// Receive data
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
gboolean glibhelper_create_internal_socket(glibhelper_unix_socket_internal_support *handle, GMainContext *context, glibhelper_internal_socket_config *config, void* userdata)
{
	int ret = -1;
	int pairfd[2] = {-1,-1};
	GIOChannel *gprimaryio = NULL;
	GSource *gprimarysource = NULL;
	struct s_glibhelper_unix_socket_internal_support *helper;
	guint id = 0;

	if (handle == NULL || config == NULL)
		return FALSE;

	helper = (struct s_glibhelper_unix_socket_internal_support*)g_malloc(sizeof(struct s_glibhelper_unix_socket_internal_support));
	if (helper == NULL)
		return FALSE;
	memset(helper,0,sizeof(struct s_glibhelper_unix_socket_internal_support));

	ret = socketpair(AF_UNIX, SOCK_SEQPACKET|SOCK_CLOEXEC|SOCK_NONBLOCK, AF_UNIX, pairfd); 
	if (ret < 0) {
		goto errorout;
	}

	if (helper->socketbuf_size > 0) {
		(void)setsockopt(pairfd[0], SOL_SOCKET, SO_SNDBUF, &config->socketbuf_size, sizeof(config->socketbuf_size));
		(void)setsockopt(pairfd[1], SOL_SOCKET, SO_SNDBUF, &config->socketbuf_size, sizeof(config->socketbuf_size));
	}
	helper->primary_fd = pairfd[0];
	helper->secondary_fd = pairfd[1];
	helper->secondary_fd_leased = FALSE;

	gprimaryio = g_io_channel_unix_new (pairfd[0]);	// Create gio channel by fd.
	if (gprimaryio == NULL)
		goto errorout;

	g_io_channel_set_close_on_unref(gprimaryio,TRUE);	// fd close on final unref
	pairfd[0] = -1; // The fd close automatically, do not close myself.

	gprimarysource = g_io_create_watch(gprimaryio,(G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL));
	if (gprimarysource == NULL)
		goto errorout;

	g_source_set_callback(gprimarysource, (GSourceFunc)internalchannel_socket_event,(gpointer)helper, NULL);

	id = g_source_attach(gprimarysource, context);

	g_source_unref(gprimarysource);

	helper->operation = config->operation;
	helper->io.gio_source = gprimaryio;
	helper->io.event_source = gprimarysource;
	helper->io.parent = helper;
	helper->context = context;
	helper->userdata = userdata;
	helper->socketbuf_size = config->socketbuf_size;
	helper->side = PRIMARY_SIDE;

	(*handle) = (glibhelper_unix_socket_internal_support)(helper);

	return TRUE;

errorout:
	
	if (gprimaryio != NULL)
		g_io_channel_unref(gprimaryio);

	if (pairfd[1] >= 0)
		close(pairfd[1]);

	if (pairfd[0] >= 0)
		close(pairfd[0]);

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
gboolean glibhelper_bind_secondary_internal_socket(glibhelper_unix_socket_internal_support *secondary_handle, glibhelper_unix_socket_internal_support primary_handle, 
	GMainContext *context, glibhelper_internal_socket_config *config, void* userdata)
{
//	int ret = -1;
	GIOChannel *gsecondaryio = NULL;
	GSource *gsecondarysource = NULL;
	struct s_glibhelper_unix_socket_internal_support *primary_helper = NULL;
	struct s_glibhelper_unix_socket_internal_support *secondary_helper = NULL;
	guint id = 0;

	if (secondary_handle == NULL || primary_handle == NULL || config == NULL)
		return FALSE;

	primary_helper = (struct s_glibhelper_unix_socket_internal_support*)primary_handle;

	if (primary_helper->side != PRIMARY_SIDE)
		return FALSE;

	secondary_helper = (struct s_glibhelper_unix_socket_internal_support*)g_malloc(sizeof(struct s_glibhelper_unix_socket_internal_support));
	if (secondary_helper == NULL)
		return FALSE;
	memset(secondary_helper,0,sizeof(struct s_glibhelper_unix_socket_internal_support));
	secondary_helper->primary_fd = -1;
	secondary_helper->secondary_fd = primary_helper->secondary_fd;
	secondary_helper->secondary_fd_leased = TRUE;

	gsecondaryio = g_io_channel_unix_new (primary_helper->secondary_fd);	// Create gio channel by fd.
	if (gsecondaryio == NULL)
		goto errorout;

	g_io_channel_set_close_on_unref(gsecondaryio,TRUE);	// fd close on final unref
	primary_helper->secondary_fd_leased = TRUE; // The fd close automatically, do not close myself.

	gsecondarysource = g_io_create_watch(gsecondaryio,(G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL));
	if (gsecondarysource == NULL)
		goto errorout;

	g_source_set_callback(gsecondarysource, (GSourceFunc)internalchannel_socket_event,(gpointer)secondary_helper, NULL);

	id = g_source_attach(gsecondarysource, context);

	g_source_unref(gsecondarysource);

	secondary_helper->operation = config->operation;
	secondary_helper->io.gio_source = gsecondaryio;
	secondary_helper->io.event_source = gsecondarysource;
	secondary_helper->io.parent = secondary_helper;
	secondary_helper->context = context;
	secondary_helper->userdata = userdata;
	secondary_helper->socketbuf_size = config->socketbuf_size;
	secondary_helper->side = SECOUNDARY_SIDE;

	(*secondary_handle) = (glibhelper_unix_socket_internal_support)(secondary_helper);

	return TRUE;

errorout:

	if (gsecondaryio != NULL)
		g_io_channel_unref(gsecondaryio);

	g_free(secondary_helper);

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
gboolean glibhelper_terminate_internal_socket(glibhelper_unix_socket_internal_support handle)
{
	struct s_glibhelper_unix_socket_internal_support *helper = NULL;

	if (handle == NULL)
		return FALSE;// Arg error

	helper = (struct s_glibhelper_unix_socket_internal_support *)handle;

	// Destroy socket
	g_source_destroy(helper->io.event_source);
	g_io_channel_unref(helper->io.gio_source);

	if (helper->side == PRIMARY_SIDE) {
		if (helper->secondary_fd_leased == FALSE)
			(void)close(helper->secondary_fd);
	}

	g_free(helper);

	return TRUE;
}
