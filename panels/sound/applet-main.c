/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street - Suite 500, Boston, MA
 * 02110-1335, USA.
 */

#include "config.h"

#include <libintl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <glib/gi18n-lib.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gio/gio.h>

#include "gvc-applet.h"
#include "gvc-log.h"

#define GVCA_DBUS_NAME "org.gnome.VolumeControlApplet"

static gboolean show_version = FALSE;
static gboolean debug = FALSE;

int
main (int argc, char **argv)
{
        GError             *error;
        GvcApplet          *applet;
        GApplication       *app = NULL;
        static GOptionEntry entries[] = {
                { "debug", 0, 0, G_OPTION_ARG_NONE, &debug, N_("Enable debugging code"), NULL },
                { "version", 0, 0, G_OPTION_ARG_NONE, &show_version, N_("Version of this application"), NULL },
                { NULL, 0, 0, 0, NULL, NULL, NULL }
        };

        bindtextdomain (GETTEXT_PACKAGE, LOCALE_DIR);
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        textdomain (GETTEXT_PACKAGE);

        gvc_log_init ();

        error = NULL;
        gtk_init_with_args (&argc, &argv,
                            (char *) _(" — Cinnamon Volume Control Applet"),
                            entries, GETTEXT_PACKAGE,
                            &error);

        if (error != NULL) {
                g_warning ("%s", error->message);
                exit (1);
        }

        if (show_version) {
                g_print ("%s %s\n", argv [0], VERSION);
                exit (1);
        }

        gvc_log_set_debug (debug);

        if (debug == FALSE) {
                GError *error = NULL;

                app = g_application_new (GVCA_DBUS_NAME,
                                         G_APPLICATION_FLAGS_NONE);
                if (!g_application_register (app, NULL, &error)) {
                       g_warning ("%s", error->message);
                       g_error_free (error);
                       return 1;
                }
                if (g_application_get_is_remote (app)) {
                        g_warning ("Applet is already running, exiting");
                        return 0;
                }
        }

        gtk_icon_theme_append_search_path (gtk_icon_theme_get_default (),
                                           ICON_DATA_DIR);

        applet = gvc_applet_new ();
        gvc_applet_start (applet);

        gtk_main ();

        if (applet != NULL) {
                g_object_unref (applet);
        }
        if (app != NULL) {
                g_object_unref (app);
        }

        return 0;
}
