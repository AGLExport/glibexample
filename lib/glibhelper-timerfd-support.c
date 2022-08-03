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
#include <unistd.h>
#include <errno.h>
#include <sys/timerfd.h>

#include "glibhelper-timerfd-support.h"


struct s_gelibhelper_io_channel {
	GIOChannel *gio_source;
	GSource *event_source;
	guint id;
};

struct s_glibhelper_timerfd_support {
	struct s_gelibhelper_io_channel timer;
	struct s_glibhelper_timerfd_operation operation;
	GMainContext *context;
	void *userdata;
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
void* glibhelper_timerfd_get_userdata(glibhelper_timerfd_support_handle handle)
{
	struct s_glibhelper_timerfd_support *helper = NULL;

	if ( handle == NULL)
		return NULL;

	helper = (struct s_glibhelper_timerfd_support*)handle;

	return helper->userdata;
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
static gboolean timerfd_event (	GIOChannel *source,
								GIOCondition condition,
								gpointer data)
{
	int timerfd = -1;
	ssize_t readret = -1;
	gboolean ret = FALSE;
	uint64_t timerinfo = 0;
	struct s_glibhelper_timerfd_support *helper = NULL;

	if (data == NULL)
		return FALSE;// Arg error

	helper = (struct s_glibhelper_timerfd_support *)data;

	if ((condition & G_IO_IN) != 0) {// timeout
		timerfd = g_io_channel_unix_get_fd (source);
		readret = read(timerfd, &timerinfo, sizeof(timerinfo));

		if (helper->operation.timeout != NULL && readret > 0)
			ret = helper->operation.timeout(helper);

	} else // Undefined error -> stop callback
		ret = FALSE;

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
gboolean glibhelper_create_timerfd(glibhelper_timerfd_support_handle *handle, GMainContext *context, glibhelper_timerfd_config *config, void *userdata)
{
	int timerfd = -1;
	int ret = -1;
	GIOChannel *gtimerfdio = NULL;
	GSource *gtimerfdsource = NULL;
	struct s_glibhelper_timerfd_support *helper = NULL;
	guint id = 0;

	struct itimerspec timersetting;

	if (handle == NULL || config == NULL)
		return FALSE;
	
	helper = (struct s_glibhelper_timerfd_support*)g_malloc(sizeof(struct s_glibhelper_timerfd_support));
	if (helper == NULL)
		return FALSE;

	timerfd = timerfd_create(CLOCK_MONOTONIC, (TFD_NONBLOCK | TFD_CLOEXEC));
	if (timerfd < 0)
		goto errorout;

	memset(&timersetting, 0, sizeof(timersetting));
	
	// Do not use initial expiration
	timersetting.it_value.tv_sec  = 0;
	timersetting.it_value.tv_nsec = 1;

	// Set a interval time
	timersetting.it_interval.tv_sec = config->interval / (1000 * 1000 *1000);
	timersetting.it_interval.tv_nsec = config->interval % (1000 * 1000 * 1000);
	
	ret = timerfd_settime(timerfd, 0, &timersetting, NULL);
	if (ret < 0)
		goto errorout;

	gtimerfdio = g_io_channel_unix_new (timerfd);
	g_io_channel_set_close_on_unref(gtimerfdio,TRUE);	// fd close on final unref
	timerfd = -1; // The fd close automatically, do not close myself.

	gtimerfdsource = g_io_create_watch(gtimerfdio,(G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL));
	if (gtimerfdsource == NULL)
		goto errorout;

	g_source_set_callback(gtimerfdsource, (GSourceFunc)timerfd_event,(gpointer)helper, NULL);

	id = g_source_attach(gtimerfdsource, context);
	if (id == -1) {
		goto errorout;
	}

	g_source_unref(gtimerfdsource);

	helper->operation = config->operation;
	helper->timer.gio_source = gtimerfdio;
	helper->timer.event_source = gtimerfdsource;
	helper->timer.id = id;
	helper->context = context;
	helper->userdata = userdata;

	(*handle) = (glibhelper_timerfd_support_handle)(helper);

	return TRUE;

errorout:

	if (gtimerfdio != NULL)
		g_io_channel_unref(gtimerfdio);

	if (timerfd >= 0)
		close(timerfd);

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
gboolean glibhelper_terminate_timerfd(glibhelper_timerfd_support_handle handle)
{
	struct s_glibhelper_timerfd_support *helper = NULL;

	if (handle == NULL)
		return FALSE;// Arg error

	helper = (struct s_glibhelper_timerfd_support *)handle;

	// Destroy timerfd
	g_source_destroy(helper->timer.event_source);
	g_io_channel_unref(helper->timer.gio_source);
	g_free(helper);

	return TRUE;
}
