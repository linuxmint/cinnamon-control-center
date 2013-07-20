/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2009 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street - Suite 500, Boston, MA 02110-1335, USA.
 *
 */


#include "config.h"

#include <glib.h>
#include <glib/gstdio.h>

#include "cc-shell-log.h"

static int log_levels = G_LOG_LEVEL_CRITICAL |
                        G_LOG_LEVEL_ERROR    |
                        G_LOG_LEVEL_WARNING  |
                        G_LOG_LEVEL_MESSAGE  |
                        G_LOG_LEVEL_INFO     |
                        G_LOG_LEVEL_DEBUG;

static void
cc_shell_log_default_handler (const gchar    *log_domain,
                         GLogLevelFlags  log_level,
                         const gchar    *message,
                         gpointer        unused_data)
{
        if ((log_level & log_levels) == 0)
                return;

        g_log_default_handler (log_domain, log_level, message, unused_data);
}

void
cc_shell_log_init (void)
{
        g_log_set_default_handler (cc_shell_log_default_handler, NULL);
}

void
cc_shell_log_set_debug (gboolean debug)
{
        g_setenv ("G_MESSAGES_DEBUG", "all", TRUE);

        if (debug) {
                log_levels |= (G_LOG_LEVEL_DEBUG | G_LOG_LEVEL_INFO);
                g_debug ("Enabling debugging");
        } else {
                log_levels &= ~ (G_LOG_LEVEL_DEBUG | G_LOG_LEVEL_INFO);
        }
}
