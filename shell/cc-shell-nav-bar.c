/*
 * Copyright 2012 Canonical
 *
 * The Control Center is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * The Control Center is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with the Control Center; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Aurélien Gâteau <aurelien.gateau@canonical.com>
 */

#include "cc-shell-nav-bar.h"
#include "cc-shell-marshal.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (CcShellNavBar, cc_shell_nav_bar, GTK_TYPE_BOX)

#define SHELL_NAV_BAR_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CC_TYPE_SHELL_NAV_BAR, CcShellNavBarPrivate))

struct _CcShellNavBarPrivate
{
  GtkWidget *home_button;
  GtkWidget *detail_button;
};

enum
{
  HOME_CLICKED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0,};

static void
cc_shell_nav_bar_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
cc_shell_nav_bar_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
cc_shell_nav_bar_dispose (GObject *object)
{
  G_OBJECT_CLASS (cc_shell_nav_bar_parent_class)->dispose (object);
}

static void
cc_shell_nav_bar_finalize (GObject *object)
{
  G_OBJECT_CLASS (cc_shell_nav_bar_parent_class)->finalize (object);
}

static void
home_button_clicked_cb (GtkButton *button,
                        CcShellNavBar *bar)
{
  g_signal_emit (bar, signals[HOME_CLICKED], 0);
}

static void
cc_shell_nav_bar_class_init (CcShellNavBarClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (CcShellNavBarPrivate));

  object_class->get_property = cc_shell_nav_bar_get_property;
  object_class->set_property = cc_shell_nav_bar_set_property;
  object_class->dispose = cc_shell_nav_bar_dispose;
  object_class->finalize = cc_shell_nav_bar_finalize;

  signals[HOME_CLICKED] = g_signal_new ("home-clicked",
                                        CC_TYPE_SHELL_NAV_BAR,
                                        G_SIGNAL_RUN_FIRST,
                                        0,
                                        NULL,
                                        NULL,
                                        cc_shell_marshal_VOID__VOID,
                                        G_TYPE_NONE,
                                        0);
}

static void
cc_shell_nav_bar_init (CcShellNavBar *self)
{
  self->priv = SHELL_NAV_BAR_PRIVATE (self);
  self->priv->home_button = gtk_button_new_with_mnemonic (_("_All Settings"));
  self->priv->detail_button = gtk_button_new();

  gtk_box_pack_start (GTK_BOX(self), self->priv->home_button, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(self), self->priv->detail_button, FALSE, FALSE, 0);

  gtk_widget_show (self->priv->home_button);

  g_signal_connect (self->priv->home_button, "clicked",
                    G_CALLBACK (home_button_clicked_cb), self);

  GtkStyleContext *context = gtk_widget_get_style_context (GTK_WIDGET(self));
  gtk_style_context_add_class (context, GTK_STYLE_CLASS_LINKED);
  gtk_style_context_add_class (context, "breadcrumbs");
}

GtkWidget *
cc_shell_nav_bar_new (void)
{
  return g_object_new (CC_TYPE_SHELL_NAV_BAR, NULL);
}

void
cc_shell_nav_bar_show_detail_button (CcShellNavBar *bar, const gchar *label)
{
  gtk_widget_show (bar->priv->detail_button);
  gtk_button_set_label (GTK_BUTTON (bar->priv->detail_button), label);
}

void
cc_shell_nav_bar_hide_detail_button (CcShellNavBar *bar)
{
  gtk_widget_hide (bar->priv->detail_button);
}
