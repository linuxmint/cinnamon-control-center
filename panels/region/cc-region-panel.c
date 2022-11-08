/*
 * Copyright (C) 2010 Intel, Inc
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
 * Author: Sergey Udaltsov <svu@gnome.org>
 *
 */
#include "config.h"
#include "cc-region-panel.h"
#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "cinnamon-region-panel-xkb.h"

G_DEFINE_DYNAMIC_TYPE (CcRegionPanel, cc_region_panel, CC_TYPE_PANEL)

#define REGION_PANEL_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), CC_TYPE_REGION_PANEL, CcRegionPanelPrivate))

struct _CcRegionPanelPrivate {
	GtkBuilder *builder;
};

static void
cc_region_panel_finalize (GObject * object)
{
	CcRegionPanel *panel;

	panel = CC_REGION_PANEL (object);

	if (panel->priv && panel->priv->builder)
		g_object_unref (panel->priv->builder);

	G_OBJECT_CLASS (cc_region_panel_parent_class)->finalize (object);
}

static void
cc_region_panel_class_init (CcRegionPanelClass * klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (CcRegionPanelPrivate));

	object_class->finalize = cc_region_panel_finalize;
}

static void
cc_region_panel_class_finalize (CcRegionPanelClass * klass)
{
}

static void
cc_region_panel_init (CcRegionPanel * self)
{
	CcRegionPanelPrivate *priv;
	GtkWidget *prefs_widget;
	const char *desktop;
	GError *error = NULL;

	priv = self->priv = REGION_PANEL_PRIVATE (self);

	desktop = g_getenv ("XDG_CURRENT_DESKTOP");

	priv->builder = gtk_builder_new ();
	gtk_builder_add_from_file (priv->builder,
				   CINNAMONCC_UI_DIR "/cinnamon-region-panel.ui",
				   &error);
	if (error != NULL) {
		g_warning ("Error loading UI file: %s", error->message);
		g_error_free (error);
		return;
	}

    prefs_widget = (GtkWidget *) gtk_builder_get_object (priv->builder, "vbox1");

	gtk_widget_set_size_request (GTK_WIDGET (prefs_widget), -1, 400);

	gtk_widget_reparent (prefs_widget, GTK_WIDGET (self));

    setup_xkb_tabs (priv->builder);
        
}

void
cc_region_panel_register (GIOModule * module)
{
	textdomain (GETTEXT_PACKAGE);
	bindtextdomain (GETTEXT_PACKAGE, LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	cc_region_panel_register_type (G_TYPE_MODULE (module));
	g_io_extension_point_implement (CC_SHELL_PANEL_EXTENSION_POINT,
					CC_TYPE_REGION_PANEL,
					"region", 0);
}
