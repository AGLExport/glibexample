/**
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file	glibhelper-signal.h
 * @brief	header for glibhelper-signal
 */
#ifndef GLIBHELPER_SIGNAL_H
#define GLIBHELPER_SIGNAL_H
//-----------------------------------------------------------------------------
#include <glib.h>
#include <gio/gio.h>

//-----------------------------------------------------------------------------
gboolean glibhelper_siganl_initialize(GMainLoop *loop);

//-----------------------------------------------------------------------------
#endif //#ifndef GLIBHELPER_SIGNAL_H
