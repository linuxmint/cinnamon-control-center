/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*-
 *
 * Copyright (C) 2010 Red Hat, Inc
 * Copyright (C) 2008 William Jon McCann <jmccann@redhat.com>
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
#include "cc-screen-panel.h"
#include <glib/gi18n-lib.h>

CC_PANEL_REGISTER (CcScreenPanel, cc_screen_panel)

#define SCREEN_PANEL_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CC_TYPE_SCREEN_PANEL, CcScreenPanelPrivate))

#define WID(s) GTK_WIDGET (gtk_builder_get_object (self->priv->builder, s))
#define LS(s) GTK_LIST_STORE (gtk_builder_get_object (self->priv->builder, s))

struct _CcScreenPanelPrivate
{
  GCancellable  *cancellable;
  GtkBuilder    *builder;
  GDBusProxy    *proxy;
  gboolean       setting_brightness;
};


static void
cc_screen_panel_get_property (GObject    *object,
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
cc_screen_panel_set_property (GObject      *object,
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
cc_screen_panel_dispose (GObject *object)
{
  CcScreenPanelPrivate *priv = CC_SCREEN_PANEL (object)->priv;

  if (priv->cancellable != NULL)
    {
      g_cancellable_cancel (priv->cancellable);
      g_object_unref (priv->cancellable);
      priv->cancellable = NULL;
    }
  if (priv->builder != NULL)
    {
      g_object_unref (priv->builder);
      priv->builder = NULL;
    }
  if (priv->proxy != NULL)
    {
      g_object_unref (priv->proxy);
      priv->proxy = NULL;
    }

  G_OBJECT_CLASS (cc_screen_panel_parent_class)->dispose (object);
}

static const char *
cc_screen_panel_get_help_uri (CcPanel *panel)
{
  if (!g_strcmp0(g_getenv("XDG_CURRENT_DESKTOP"), "Unity"))
    return "help:ubuntu-help/prefs-display";
  else
    return "help:cinnamon-help/prefs-display";
}

static void
cc_screen_panel_class_init (CcScreenPanelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CcPanelClass *panel_class = CC_PANEL_CLASS (klass);

  g_type_class_add_private (klass, sizeof (CcScreenPanelPrivate));

  object_class->get_property = cc_screen_panel_get_property;
  object_class->set_property = cc_screen_panel_set_property;
  object_class->dispose = cc_screen_panel_dispose;

  panel_class->get_help_uri = cc_screen_panel_get_help_uri;
}

static void
set_brightness_cb (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  GError *error = NULL;
  GVariant *result;

  CcScreenPanelPrivate *priv = CC_SCREEN_PANEL (user_data)->priv;

  /* not setting, so pay attention to changed signals */
  priv->setting_brightness = FALSE;
  result = g_dbus_proxy_call_finish (G_DBUS_PROXY (source_object), res, &error);
  if (result == NULL)
    {
      g_printerr ("Error setting brightness: %s\n", error->message);
      g_error_free (error);
      return;
    }
}

static void
brightness_slider_value_changed_cb (GtkRange *range, gpointer user_data)
{
  guint percentage;
  CcScreenPanelPrivate *priv = CC_SCREEN_PANEL (user_data)->priv;

  /* do not loop */
  if (priv->setting_brightness)
    return;

  priv->setting_brightness = TRUE;

  /* push this to g-p-m */
  percentage = (guint) gtk_range_get_value (range);
  g_dbus_proxy_call (priv->proxy,
                     "SetPercentage",
                     g_variant_new ("(u)",
                                    percentage),
                     G_DBUS_CALL_FLAGS_NONE,
                     -1,
                     priv->cancellable,
                     set_brightness_cb,
                     user_data);
}

static void
get_brightness_cb (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  GError *error = NULL;
  GVariant *result;
  guint brightness;
  GtkRange *range;
  CcScreenPanel *self = CC_SCREEN_PANEL (user_data);

  result = g_dbus_proxy_call_finish (G_DBUS_PROXY (source_object), res, &error);
  if (result == NULL)
    {
      /* We got cancelled, so we're probably exiting */
      if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        {
          g_error_free (error);
          return;
	}

      gtk_widget_hide (WID ("screen_brightness_hscale"));
      gtk_widget_hide (WID ("brightness-frame"));

      if (error->message &&
      	  strstr (error->message, "No backlight devices present") == NULL)
        {
          g_warning ("No backlight devices present: %s", error->message);
        }
      g_error_free (error);
      return;
    }

  /* set the slider */
  g_variant_get (result,
                 "(u)",
                 &brightness);
  range = GTK_RANGE (WID ("screen_brightness_hscale"));
  gtk_range_set_range (range, 0, 100);
  gtk_range_set_increments (range, 1, 10);
  gtk_range_set_value (range, brightness);
  g_signal_connect (range,
                    "value-changed",
                    G_CALLBACK (brightness_slider_value_changed_cb),
                    user_data);
  g_variant_unref (result);
}

static void
on_signal (GDBusProxy *proxy,
           gchar      *sender_name,
           gchar      *signal_name,
           GVariant   *parameters,
           gpointer    user_data)
{
  CcScreenPanel *self = CC_SCREEN_PANEL (user_data);

  if (g_strcmp0 (signal_name, "Changed") == 0)
    {
      /* changed, but ignoring */
      if (self->priv->setting_brightness)
        return;

      /* retrieve the value again from g-s-d */
      g_dbus_proxy_call (self->priv->proxy,
                         "GetPercentage",
                         NULL,
                         G_DBUS_CALL_FLAGS_NONE,
                         200, /* we don't want to randomly move the bar */
                         self->priv->cancellable,
                         get_brightness_cb,
                         user_data);
    }
}

static void
got_power_proxy_cb (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  GError *error = NULL;
  CcScreenPanelPrivate *priv = CC_SCREEN_PANEL (user_data)->priv;

  priv->proxy = g_dbus_proxy_new_for_bus_finish (res, &error);
  if (priv->proxy == NULL)
    {
      g_printerr ("Error creating proxy: %s\n", error->message);
      g_error_free (error);
      return;
    }

  /* we want to change the bar if the user presses brightness buttons */
  g_signal_connect (priv->proxy,
                    "g-signal",
                    G_CALLBACK (on_signal),
                    user_data);

  /* get the initial state */
  g_dbus_proxy_call (priv->proxy,
                     "GetPercentage",
                     NULL,
                     G_DBUS_CALL_FLAGS_NONE,
                     200, /* we don't want to randomly move the bar */
                     priv->cancellable,
                     get_brightness_cb,
                     user_data);
}

static void
cc_screen_panel_init (CcScreenPanel *self)
{
  GError     *error;
  GtkWidget  *widget;

  self->priv = SCREEN_PANEL_PRIVATE (self);

  self->priv->builder = gtk_builder_new ();
  gtk_builder_set_translation_domain (self->priv->builder, GETTEXT_PACKAGE);
  error = NULL;
  gtk_builder_add_from_file (self->priv->builder,
                             CINNAMONCC_UI_DIR "/screen.ui",
                             &error);

  if (error != NULL)
    {
      g_warning ("Could not load interface file: %s", error->message);
      g_error_free (error);
      return;
    }

  self->priv->cancellable = g_cancellable_new ();

  /* get initial brightness version */
  g_dbus_proxy_new_for_bus (G_BUS_TYPE_SESSION,
                            G_DBUS_PROXY_FLAGS_NONE,
                            NULL,
                            "org.cinnamon.SettingsDaemon",
                            "/org/cinnamon/SettingsDaemon/Power",
                            "org.cinnamon.SettingsDaemon.Power.Screen",
                            self->priv->cancellable,
                            got_power_proxy_cb,
                            self);

  widget = WID ("screen_vbox");
  gtk_widget_reparent (widget, (GtkWidget *) self);
  g_object_set (self, "valign", GTK_ALIGN_START, NULL);
}

void
cc_screen_panel_register (GIOModule *module)
{
  bindtextdomain (GETTEXT_PACKAGE, "/usr/share/cinnamon/locale");
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  cc_screen_panel_register_type (G_TYPE_MODULE (module));
  g_io_extension_point_implement (CC_SHELL_PANEL_EXTENSION_POINT,
                                  CC_TYPE_SCREEN_PANEL,
                                  "screen", 0);
}

