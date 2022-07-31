/**
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file	glibhelper-signal.c
 * @brief	signal handling helper.
 */
#include <glib.h>
#include <gio/gio.h>
#include <glib-unix.h>

#include <signal.h>
#include <errno.h>

#include "glibhelper-signal.h"

/**
 * Callback function for SIGTERM handling
 *
 * @param [in]	user_data	It is GMainLoop.
 *
 * @return gboolean
 * @retval TRUE
 */
static gboolean glibhelper_sigterm_handler(gpointer user_data)
{
	g_main_loop_quit(user_data);

	return TRUE;
}

/**
 * Signal initialize operation.
 * This finction add SIGTERM handler to glib context.
 * After this funcgtion, you can use terminate operation after event lopp exit.
 *
 * @param [in]	loop	Main event loop created by g_main_loop_new
 *
 * @return gboolean
 * @retval TRUE Success to initialization.
 * @retval FALSE Fail to initialization. This error never occurs by usage.
 */
gboolean glibhelper_siganl_initialize(GMainLoop *loop)
{
	sigset_t ss;
	guint sgnalid = 0;

	(void)sigemptyset(&ss);
	//(void)sigaddset(&ss, SIGTERM); When SIGTERM handle by glib,  that shall not mask it.
	(void)sigaddset(&ss, SIGPIPE);

	// Block SIGPIPE
	if (sigprocmask(SIG_BLOCK, &ss, NULL) < 0)
		return FALSE;

	sgnalid = g_unix_signal_add(SIGTERM, glibhelper_sigterm_handler, loop);
	if (sgnalid == 0)
		return FALSE;

	return TRUE;
}
