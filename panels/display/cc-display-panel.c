/*
 * Copyright (C) 2007, 2008  Red Hat, Inc.
 * Copyright (C) 2013 Intel, Inc.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */
#include <config.h>
#include "cc-display-panel.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include <math.h>
#ifdef GDK_WINDOWING_WAYLAND
#include <gdk/gdkwayland.h>
#endif

#include <libupower-glib/upower.h>

#include "cc-display-config-manager-dbus.h"
#include "cc-display-config.h"
#include "cc-display-arrangement.h"
#include "cc-display-resources.h"
#include "cc-display-settings.h"
#include "cc-display-labeler.h"

/* The minimum supported size for the panel
 * Note that WIDTH is assumed to be the larger size and we accept portrait
 * mode too effectively (in principle we should probably restrict the rotation
 * setting in that case). */
#define MINIMUM_WIDTH 740
#define MINIMUM_HEIGHT 530

#define PANEL_PADDING   32
#define SECTION_PADDING 32
#define HEADING_PADDING 12

#define WID(s) GTK_WIDGET (gtk_builder_get_object (self->builder, s))

typedef enum {
  CC_DISPLAY_CONFIG_JOIN,
  CC_DISPLAY_CONFIG_CLONE,

  CC_DISPLAY_CONFIG_INVALID_NONE,
} CcDisplayConfigType;

#define CC_DISPLAY_CONFIG_LAST_VALID CC_DISPLAY_CONFIG_CLONE

struct _CcDisplayPanel
{
  CcPanel parent_instance;

  CcDisplayConfigManager *manager;
  CcDisplayConfig *current_config;
  CcDisplayMonitor *current_output;

  GDBusProxy *cinnamon_proxy;

  gint                  rebuilding_counter;

  CcDisplayArrangement *arrangement;
  CcDisplaySettings    *settings;

  guint           focus_id;

  UpClient *up_client;
  gboolean lid_is_closed;

  guint       sensor_watch_id;
  GDBusProxy *iio_sensor_proxy;
  gboolean    has_accelerometer;

  GtkBuilder *builder;

  GtkWidget *apply_button;
  GtkWidget *defaults_button;
  GtkWidget *cancel_button;

  GtkListStore   *output_selection_list;
  GdkRGBA   *palette;
  gint n_outputs;

  GtkWidget *arrangement_frame;
  GtkWidget *arrangement_bin;
  GtkWidget *config_type_join;
  GtkWidget *config_type_mirror;
  GtkWidget *config_type_revealer;
  GtkWidget *display_settings_frame;
  GtkWidget *output_enabled_switch;
  GtkWidget *output_selection_combo;
  GtkWidget *primary_display_toggle;
  GtkWidget *stack_switcher;

  GCancellable   *cancellable;
  GSettings      *muffin_settings;

  CcDisplayLabeler *labeler;
};

CC_PANEL_REGISTER (CcDisplayPanel, cc_display_panel)

#ifdef GDK_WINDOWING_WAYLAND
#define WAYLAND_SESSION() (GDK_IS_WAYLAND_DISPLAY (gdk_display_get_default()))
#else
#define WAYLAND_SESSION() (FALSE)
#endif

static void update_bottom_buttons (CcDisplayPanel *panel);
static void apply_current_configuration (CcDisplayPanel *self);
static void reset_current_config (CcDisplayPanel *panel);
static void rebuild_ui (CcDisplayPanel *panel);
static void regenerate_palette (CcDisplayPanel *panel, gint n_outputs);
static void set_current_output (CcDisplayPanel   *panel,
                                CcDisplayMonitor *output,
                                gboolean          force);
static gchar *get_color_string_for_output (CcDisplayPanel *panel, gint index);
static gchar *get_output_color (GObject *source, gint index, CcDisplayPanel *self);

static CcDisplayConfigType
config_get_current_type (CcDisplayPanel *panel)
{
  guint n_active_outputs;
  GList *outputs, *l;

  outputs = cc_display_config_get_ui_sorted_monitors (panel->current_config);
  n_active_outputs = 0;
  for (l = outputs; l; l = l->next)
    {
      CcDisplayMonitor *output = l->data;

      if (cc_display_monitor_is_useful (output))
        n_active_outputs += 1;
    }

  if (n_active_outputs == 0)
    return CC_DISPLAY_CONFIG_INVALID_NONE;

  if (cc_display_config_is_cloning (panel->current_config))
    return CC_DISPLAY_CONFIG_CLONE;

  return CC_DISPLAY_CONFIG_JOIN;
}

static CcDisplayConfigType
cc_panel_get_selected_type (CcDisplayPanel *panel)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (panel->config_type_join)))
    return CC_DISPLAY_CONFIG_JOIN;
  else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (panel->config_type_mirror)))
    return CC_DISPLAY_CONFIG_CLONE;
  else
    g_assert_not_reached ();
}

static void
config_ensure_of_type (CcDisplayPanel *panel, CcDisplayConfigType type)
{
  CcDisplayConfigType current_type = config_get_current_type (panel);
  GList *outputs, *l;
  CcDisplayMonitor *current_primary = NULL;
  gdouble old_primary_scale = -1;

  /* Do not do anything if the current detected configuration type is
   * identitcal to what we expect. */
  if (type == current_type)
    return;

  reset_current_config (panel);

  outputs = cc_display_config_get_ui_sorted_monitors (panel->current_config);

  for (l = outputs; l; l = l->next)
    {
      CcDisplayMonitor *output = l->data;

      if (cc_display_monitor_is_primary (output))
        {
          current_primary = output;
          old_primary_scale = cc_display_monitor_get_scale (current_primary);
          break;
        }
    }

  switch (type)
    {
    case CC_DISPLAY_CONFIG_JOIN:
      g_debug ("Creating new join or single config");
      /* Enable all usable outputs
       * Note that this might result in invalid configurations as we
       * we might not be able to drive all attached monitors. */
      cc_display_config_set_cloning (panel->current_config, FALSE);

      if (g_list_length (outputs) == 1)
        {
          for (l = outputs; l; l = l->next)
            {
              CcDisplayMonitor *output = l->data;

              /* Select the current primary output as the active one */
              if (cc_display_monitor_is_primary (output))
                {
                  cc_display_monitor_set_active (output, TRUE);
                  cc_display_monitor_set_mode (output, cc_display_monitor_get_preferred_mode (output));
                  set_current_output (panel, output, FALSE);
                }
              else
                {
                  cc_display_monitor_set_active (output, FALSE);
                  cc_display_monitor_set_mode (output, cc_display_monitor_get_preferred_mode (output));
                }
            }
          gtk_widget_set_sensitive (panel->output_enabled_switch, FALSE);
        }
      else
        {
          for (l = outputs; l; l = l->next)
            {
              CcDisplayMonitor *output = l->data;
              gdouble scale;
              CcDisplayMode *mode;

              mode = cc_display_monitor_get_preferred_mode (output);
              /* If the monitor was active, try using the current scale, otherwise
               * try picking the preferred scale. */
              if (cc_display_monitor_is_active (output))
                scale = cc_display_monitor_get_scale (output);
              else
                scale = cc_display_mode_get_preferred_scale (mode);

              /* If we cannot use the current/preferred scale, try to fall back to
               * the previously scale of the primary monitor instead.
               * This is not guaranteed to result in a valid configuration! */
              if (!cc_display_config_is_scaled_mode_valid (panel->current_config,
                                                           mode,
                                                           scale))
                {
                  if (current_primary &&
                      cc_display_config_is_scaled_mode_valid (panel->current_config,
                                                              mode,
                                                              old_primary_scale))
                    scale = old_primary_scale;
                }

              cc_display_monitor_set_active (output, cc_display_monitor_is_usable (output));
              cc_display_monitor_set_mode (output, mode);
              cc_display_monitor_set_scale (output, scale);


            }
          gtk_widget_set_sensitive (panel->output_enabled_switch, TRUE);
        }
      break;

    case CC_DISPLAY_CONFIG_CLONE:
      {
        g_debug ("Creating new clone config");

        gdouble scale;
        GList *modes = cc_display_config_get_cloning_modes (panel->current_config);
        gint bw, bh;
        CcDisplayMode *best = NULL;

        /* Turn on cloning and select the best mode we can find by default */
        cc_display_config_set_cloning (panel->current_config, TRUE);

        while (modes)
          {
            CcDisplayMode *mode = modes->data;
            gint w, h;

            cc_display_mode_get_resolution (mode, &w, &h);
            if (best == NULL || (bw*bh < w*h))
              {
                best = mode;
                cc_display_mode_get_resolution (best, &bw, &bh);
              }

            modes = modes->next;
          }

        /* Take the preferred scale by default, */
        scale = cc_display_mode_get_preferred_scale (best);
        /* but prefer the old primary scale if that is valid. */
        if (current_primary &&
            cc_display_config_is_scaled_mode_valid (panel->current_config,
                                                    best,
                                                    old_primary_scale))
          scale = old_primary_scale;

        for (l = outputs; l; l = l->next)
          {
            CcDisplayMonitor *output = l->data;

            cc_display_monitor_set_mode (output, best);
            cc_display_monitor_set_scale (output, scale);
          }

        gtk_widget_set_sensitive (panel->output_enabled_switch, FALSE);
      }
      break;

    default:
      g_assert_not_reached ();
    }

  rebuild_ui (panel);
}

static void
cc_panel_set_selected_type (CcDisplayPanel *panel, CcDisplayConfigType type)
{
  switch (type)
    {
    case CC_DISPLAY_CONFIG_JOIN:
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (panel->config_type_join), TRUE);
      break;
    case CC_DISPLAY_CONFIG_CLONE:
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (panel->config_type_mirror), TRUE);
      break;
    default:
      g_assert_not_reached ();
    }

  config_ensure_of_type (panel, type);
}

static void
hide_labels_dbus (CcDisplayPanel *self)
{
  if (!self->cinnamon_proxy)
    return;

  g_dbus_proxy_call (self->cinnamon_proxy,
                     "HideMonitorLabels",
                     NULL, G_DBUS_CALL_FLAGS_NONE,
                     -1, NULL, NULL, NULL);
}

static void
show_labels_dbus (CcDisplayPanel *self)
{
  GList *outputs, *l;
  GVariantBuilder builder;
  gint number = 0;

  if (!self->cinnamon_proxy || !self->current_config)
    return;

  outputs = cc_display_config_get_ui_sorted_monitors (self->current_config);
  if (!outputs)
    return;

  g_variant_builder_init (&builder, G_VARIANT_TYPE_TUPLE);
  g_variant_builder_open (&builder, G_VARIANT_TYPE_ARRAY);

  gint color_index = 0;

  for (l = outputs, color_index = 0; l != NULL; l = l->next, color_index++)
    {
      CcDisplayMonitor *output = l->data;

      number = cc_display_monitor_get_ui_number (output);
      if (number == 0)
        continue;

      GVariant *var;
      g_autofree gchar *color_str = get_color_string_for_output (self, color_index);

      var = g_variant_new ("(ibss)",
                           number,
                           cc_display_config_is_cloning (self->current_config),
                           cc_display_monitor_get_display_name (output),
                           color_str);

      g_variant_builder_add (&builder, "{sv}",
                             cc_display_monitor_get_connector_name (output),
                             var);
    }

  g_variant_builder_close (&builder);

  if (number < 1)
    {
      g_variant_builder_clear (&builder);
      return;
    }

  g_dbus_proxy_call (self->cinnamon_proxy,
                     "ShowMonitorLabels",
                     g_variant_builder_end (&builder),
                     G_DBUS_CALL_FLAGS_NONE,
                     -1, NULL, NULL, NULL);
}

static void
hide_labels (CcDisplayPanel *self)
{
  if (WAYLAND_SESSION ())
    {
      hide_labels_dbus (self);
    }
  else
    {
      if (self->labeler) {
        cc_display_labeler_hide (self->labeler);
      }
    }
}

static void
show_labels (CcDisplayPanel *self)
{
  if (WAYLAND_SESSION ())
    {
      show_labels_dbus (self);
    }
  else
    {
      if (self->labeler) {
        cc_display_labeler_hide (self->labeler);
        g_object_unref (self->labeler);
      }

      self->labeler = cc_display_labeler_new (self->current_config);

      g_signal_connect_object (self->labeler, "get-output-color",
                               G_CALLBACK (get_output_color), self, 0);

      cc_display_labeler_show (self->labeler);
    }
}

static void
widget_visible_changed (GtkWidget *widget,
                        gpointer user_data)
{
  CcDisplayPanel *self = CC_DISPLAY_PANEL (widget);
  if (gtk_widget_get_visible (widget) && self->current_config != NULL) {
    show_labels (self);
  } else {
    hide_labels (self);
  }
}

static void
ensure_monitor_labels (CcDisplayPanel *self)
{
    widget_visible_changed (GTK_WIDGET (self), self);
}

static void
cc_display_panel_dispose (GObject *object)
{
  CcDisplayPanel *self = CC_DISPLAY_PANEL (object);

  if (self->sensor_watch_id > 0)
    {
      g_bus_unwatch_name (self->sensor_watch_id);
      self->sensor_watch_id = 0;
    }

  g_clear_object (&self->iio_sensor_proxy);

  g_clear_object (&self->manager);
  g_clear_object (&self->current_config);
  g_clear_object (&self->up_client);
  g_clear_object (&self->cinnamon_proxy);

  g_clear_object (&self->muffin_settings);
  g_clear_object (&self->labeler);
  g_clear_pointer (&self->palette, g_free);

  g_clear_object (&self->output_selection_list);
  g_clear_object (&self->builder);

  g_signal_handlers_disconnect_by_func (self, widget_visible_changed, NULL);

  G_OBJECT_CLASS (cc_display_panel_parent_class)->dispose (object);
}

static void
on_arrangement_selected_ouptut_changed_cb (CcDisplayPanel *panel)
{
  set_current_output (panel, cc_display_arrangement_get_selected_output (panel->arrangement), FALSE);
}

static void
on_monitor_settings_updated_cb (CcDisplayPanel    *panel,
                                CcDisplayMonitor  *monitor,
                                CcDisplaySettings *settings)
{
  if (monitor)
    cc_display_config_snap_output (panel->current_config, monitor);
  update_bottom_buttons (panel);
}

static void
on_config_type_toggled_cb (CcDisplayPanel *panel,
                           GtkRadioButton *btn)
{
  CcDisplayConfigType type;

  if (panel->rebuilding_counter > 0)
    return;

  if (!panel->current_config)
    return;

  if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (btn)))
    return;

  type = cc_panel_get_selected_type (panel);
  config_ensure_of_type (panel, type);
}

static void
on_output_enabled_active_changed_cb (CcDisplayPanel *panel)
{
  gboolean active;

  if (!panel->current_output)
    return;

  active = gtk_switch_get_active (GTK_SWITCH (panel->output_enabled_switch));

  if (cc_display_monitor_is_active (panel->current_output) == active)
    return;

  cc_display_monitor_set_active (panel->current_output, active);

  /* Prevent the invalid configuration of disabling the last monitor
   * by switching on a different one. */
  if (config_get_current_type (panel) == CC_DISPLAY_CONFIG_INVALID_NONE)
    {
      GList *outputs, *l;

      outputs = cc_display_config_get_ui_sorted_monitors (panel->current_config);
      for (l = outputs; l; l = l->next)
        {
          CcDisplayMonitor *output = CC_DISPLAY_MONITOR (l->data);

          if (output == panel->current_output)
            continue;

          if (!cc_display_monitor_is_usable (output))
            continue;

          cc_display_monitor_set_active (output, TRUE);
          cc_display_monitor_set_primary (output, TRUE);
          break;
        }
    }

  /* Changing the active state requires a UI rebuild. */
  rebuild_ui (panel);
}

static void
on_output_selection_combo_changed_cb (CcDisplayPanel *panel)
{
  GtkTreeIter iter;
  g_autoptr(CcDisplayMonitor) output = NULL;

  if (!panel->current_config)
    return;

  if (!gtk_combo_box_get_active_iter (GTK_COMBO_BOX (panel->output_selection_combo), &iter))
    return;

  gtk_tree_model_get (GTK_TREE_MODEL (panel->output_selection_list), &iter,
                      1, &output,
                      -1);

  set_current_output (panel, output, FALSE);
}

static void
primary_display_toggle_toggled_cb (GtkToggleButton *button,
                                   CcDisplayPanel *panel)
{
  if (panel->rebuilding_counter > 0)
    return;

  if (cc_display_monitor_is_primary (panel->current_output))
    {
      return;
    }

  cc_display_monitor_set_primary (panel->current_output, TRUE);

  // rebuild_ui (panel);
  update_bottom_buttons (panel);

  gtk_widget_set_sensitive (GTK_WIDGET (button), FALSE);
}

static void
cc_display_panel_constructed (GObject *object)
{
  G_OBJECT_CLASS (cc_display_panel_parent_class)->constructed (object);

}

static void
cc_display_panel_class_init (CcDisplayPanelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = cc_display_panel_constructed;
  object_class->dispose = cc_display_panel_dispose;
}

static void
set_current_output (CcDisplayPanel   *panel,
                    CcDisplayMonitor *output,
                    gboolean          force)
{
  GtkTreeIter iter;
  gboolean changed;
  GList *outputs;

  /* Note, this function is also called if the internal UI needs updating after a rebuild. */
  changed = (output != panel->current_output);

  if (!changed && !force)
    return;

  panel->rebuilding_counter++;

  panel->current_output = output;

  outputs = cc_display_config_get_ui_sorted_monitors (panel->current_config);

  if (panel->current_output)
    {
      gtk_switch_set_active (GTK_SWITCH (panel->output_enabled_switch), cc_display_monitor_is_active (panel->current_output));
      gtk_widget_set_sensitive (GTK_WIDGET (panel->output_enabled_switch), cc_display_monitor_is_usable (panel->current_output) &&
                                                                           config_get_current_type (panel) == CC_DISPLAY_CONFIG_JOIN &&
                                                                           g_list_length (outputs) > 1);

      gtk_widget_set_sensitive (GTK_WIDGET (panel->primary_display_toggle), !cc_display_monitor_is_primary (panel->current_output) &&
                                                                             cc_display_monitor_is_active (panel->current_output));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (panel->primary_display_toggle), cc_display_monitor_is_primary (panel->current_output));
    }
  else
    {
      gtk_switch_set_active (GTK_SWITCH (panel->output_enabled_switch), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (panel->output_enabled_switch), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (panel->primary_display_toggle), FALSE);
    }

  gtk_tree_model_get_iter_first (GTK_TREE_MODEL (panel->output_selection_list), &iter);
  while (gtk_list_store_iter_is_valid (panel->output_selection_list, &iter))
    {
      g_autoptr(CcDisplayMonitor) o = NULL;

      gtk_tree_model_get (GTK_TREE_MODEL (panel->output_selection_list), &iter,
                          1, &o,
                          -1);

      if (o == panel->current_output)
        {
          gtk_combo_box_set_active_iter (GTK_COMBO_BOX (panel->output_selection_combo), &iter);
          break;
        }

      gtk_tree_model_iter_next (GTK_TREE_MODEL (panel->output_selection_list), &iter);
    }

  if (changed)
    {
      cc_display_settings_set_selected_output (panel->settings, panel->current_output);
      cc_display_arrangement_set_selected_output (panel->arrangement, panel->current_output);
    }

  panel->rebuilding_counter--;
}

static void
rebuild_ui (CcDisplayPanel *panel)
{
  guint n_outputs, n_active_outputs, n_usable_outputs;
  GList *outputs, *l;
  CcDisplayConfigType type;
  gboolean cloned = FALSE;
  gint index = 0;

  panel->rebuilding_counter++;

  gtk_list_store_clear (panel->output_selection_list);

  if (!panel->current_config)
    {
      panel->rebuilding_counter--;
      return;
    }

  cloned = config_get_current_type (panel);

  n_active_outputs = 0;
  n_usable_outputs = 0;
  outputs = cc_display_config_get_ui_sorted_monitors (panel->current_config);

  regenerate_palette (panel, g_list_length (outputs));
  ensure_monitor_labels (panel);

  for (l = outputs, index = 0; l; l = l->next, index++)
    {
      GtkTreeIter iter;
      CcDisplayMonitor *output = l->data;
      GdkPixbuf *pixbuf;
      GdkRGBA color;
      guint32 pixel = 0;
      const gchar *label;

      pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, 20, 20);

      gchar *color_string = get_color_string_for_output (panel, index);

      gdk_rgba_parse (&color, color_string);
      g_free (color_string);

      pixel = pixel + ((int) (color.red * 255) << 24);
      pixel = pixel + ((int) (color.green * 255) << 16);
      pixel = pixel + ((int) (color.blue * 255) << 8);
      pixel = pixel + ((int) (color.alpha * 255));


      if (cloned)
        {
          label = _("Mirrored Displays");
        }
      else
        {
          label = cc_display_monitor_get_ui_number_name (output);
        }

      gdk_pixbuf_fill (pixbuf, pixel);

      gtk_list_store_append (panel->output_selection_list, &iter);
      gtk_list_store_set (panel->output_selection_list,
                          &iter,
                          0, label,
                          1, output,
                          2, pixbuf,
                          -1);

      g_object_unref (pixbuf);

      if (!cc_display_monitor_is_usable (output))
        continue;

      n_usable_outputs += 1;

      if (cc_display_monitor_is_active (output))
        {
          /* Ensure that an output is selected; note that this doesn't ensure
           * the selected output is any useful (i.e. when switching types).
           */
          if (!panel->current_output || !cc_display_monitor_is_active (panel->current_output))
            set_current_output (panel, output, FALSE);

          n_active_outputs++;
        }
    }

  /* Sync the rebuild lists/buttons */
  set_current_output (panel, panel->current_output, TRUE);

  n_outputs = g_list_length (outputs);
  type = config_get_current_type (panel);

  if (n_outputs == 2 && n_usable_outputs == 2 && n_active_outputs == 2)
    {
      /* We only show the top chooser with two monitors that are
       * both usable (i.e. two monitors incl. internal and lid not closed).
       * In this case, the arrangement widget is shown if we are in JOIN mode.
       */
      if (type > CC_DISPLAY_CONFIG_LAST_VALID)
        type = CC_DISPLAY_CONFIG_JOIN;

      gtk_revealer_set_reveal_child (GTK_REVEALER (panel->config_type_revealer), TRUE);
    }
  else
    {
      /* We have 1 monitor, or 2+ including active and inactive. In this case there is no chooser,
       */
      gtk_revealer_set_reveal_child (GTK_REVEALER (panel->config_type_revealer), FALSE);

      /* Mirror is also invalid as it cannot be configured using this UI. */
      if (type == CC_DISPLAY_CONFIG_CLONE || type > CC_DISPLAY_CONFIG_LAST_VALID)
        type = CC_DISPLAY_CONFIG_JOIN;
    }

  cc_panel_set_selected_type (panel, type);

  gtk_widget_set_sensitive (GTK_WIDGET (panel->output_selection_combo), n_outputs > 1 && !cloned);

  panel->rebuilding_counter--;
  update_bottom_buttons (panel);
}

static void
reset_current_config (CcDisplayPanel *panel)
{
  CcDisplayConfig *current;
  CcDisplayConfig *old;
  GList *outputs, *l;

  g_debug ("Resetting current config!");

  /* We need to hold on to the config until all display references are dropped. */
  old = panel->current_config;
  panel->current_output = NULL;

  current = cc_display_config_manager_get_current (panel->manager);

  // Cinnamon could be restarting
  if (current == NULL)
    return;

  cc_display_config_set_minimum_size (current, MINIMUM_WIDTH, MINIMUM_HEIGHT);
  panel->current_config = current;

  gtk_list_store_clear (panel->output_selection_list);

  if (panel->current_config)
    {
      outputs = cc_display_config_get_ui_sorted_monitors (panel->current_config);
      for (l = outputs; l; l = l->next)
        {
          CcDisplayMonitor *output = l->data;

          /* Mark any builtin monitor as unusable if the lid is closed. */
          if (cc_display_monitor_is_builtin (output) && panel->lid_is_closed)
            cc_display_monitor_set_usable (output, FALSE);
        }
    }

  cc_display_arrangement_set_config (panel->arrangement, panel->current_config);
  cc_display_settings_set_config (panel->settings, panel->current_config);
  set_current_output (panel, NULL, FALSE);

  g_clear_object (&old);

  update_bottom_buttons (panel);
}

static void
on_screen_changed (CcDisplayPanel *panel)
{
  if (!panel->manager)
    return;

  reset_current_config (panel);
  rebuild_ui (panel);

  if (!panel->current_config)
    return;
}

static void
update_bottom_buttons (CcDisplayPanel *panel)
{
  gboolean config_equal;
  g_autoptr(CcDisplayConfig) applied_config = NULL;

  if (!panel->current_config)
    {
      return;
    }

  applied_config = cc_display_config_manager_get_current (panel->manager);

  config_equal = cc_display_config_equal (panel->current_config,
                                          applied_config);

  if (config_equal)
    {
      gtk_widget_set_sensitive (panel->apply_button, FALSE);
      gtk_widget_set_sensitive (panel->cancel_button, FALSE);
    }
  else
    {
      gtk_widget_set_sensitive (panel->apply_button, cc_display_config_is_applicable (panel->current_config));
      gtk_widget_set_sensitive (panel->cancel_button, TRUE);
    }
}

static void
apply_current_configuration (CcDisplayPanel *self)
{
  g_autoptr(GError) error = NULL;

  cc_display_config_apply (self->current_config, &error);

  /* re-read the configuration */
  on_screen_changed (self);

  if (error)
    g_warning ("Error applying configuration: %s", error->message);
}

static void
cc_display_panel_up_client_changed (UpClient       *client,
                                    GParamSpec     *pspec,
                                    CcDisplayPanel *self)
{
  gboolean lid_is_closed;

  lid_is_closed = up_client_get_lid_is_closed (client);

  if (lid_is_closed != self->lid_is_closed)
    {
      self->lid_is_closed = lid_is_closed;

      on_screen_changed (self);
    }
}

static void
update_has_accel (CcDisplayPanel *self)
{
  g_autoptr(GVariant) v = NULL;

  if (self->iio_sensor_proxy == NULL)
    {
      g_debug ("Has no accelerometer");
      self->has_accelerometer = FALSE;
      cc_display_settings_set_has_accelerometer (self->settings, self->has_accelerometer);
      return;
    }

  v = g_dbus_proxy_get_cached_property (self->iio_sensor_proxy, "HasAccelerometer");
  if (v)
    {
      self->has_accelerometer = g_variant_get_boolean (v);
    }
  else
    {
      self->has_accelerometer = FALSE;
    }

  cc_display_settings_set_has_accelerometer (self->settings, self->has_accelerometer);

  g_debug ("Has %saccelerometer", self->has_accelerometer ? "" : "no ");
}

static void
sensor_proxy_properties_changed_cb (GDBusProxy     *proxy,
                                    GVariant       *changed_properties,
                                    GStrv           invalidated_properties,
                                    CcDisplayPanel *self)
{
  GVariantDict dict;

  g_variant_dict_init (&dict, changed_properties);

  if (g_variant_dict_contains (&dict, "HasAccelerometer"))
    update_has_accel (self);
}

static void
sensor_proxy_appeared_cb (GDBusConnection *connection,
                          const gchar     *name,
                          const gchar     *name_owner,
                          gpointer         user_data)
{
  CcDisplayPanel *self = user_data;

  g_debug ("SensorProxy appeared");

  self->iio_sensor_proxy = g_dbus_proxy_new_sync (connection,
                                                        G_DBUS_PROXY_FLAGS_NONE,
                                                        NULL,
                                                        "net.hadess.SensorProxy",
                                                        "/net/hadess/SensorProxy",
                                                        "net.hadess.SensorProxy",
                                                        NULL,
                                                        NULL);
  g_return_if_fail (self->iio_sensor_proxy);

  g_signal_connect (self->iio_sensor_proxy, "g-properties-changed",
                    G_CALLBACK (sensor_proxy_properties_changed_cb), self);
  update_has_accel (self);
}

static void
sensor_proxy_vanished_cb (GDBusConnection *connection,
                          const gchar     *name,
                          gpointer         user_data)
{
  CcDisplayPanel *self = user_data;

  g_debug ("SensorProxy vanished");

  g_clear_object (&self->iio_sensor_proxy);
  update_has_accel (self);
}

static void
cinnamon_proxy_ready (GObject *source_object,
                      GAsyncResult *res,
                      gpointer user_data)
{
    CcDisplayPanel *self = CC_DISPLAY_PANEL (user_data);
    GError *error = NULL;

    self->cinnamon_proxy = g_dbus_proxy_new_finish (res, &error);

    if (self->cinnamon_proxy == NULL)
      {
        g_critical ("Can't connect to Cinnamon, monitor labeler will be unavailable: %s", error->message);
        g_clear_error (&error);
      }
}

static void
session_bus_ready (GObject        *source,
                   GAsyncResult   *res,
                   CcDisplayPanel *self)
{
  GDBusConnection *bus;
  g_autoptr(GError) error = NULL;

  bus = g_bus_get_finish (res, &error);
  if (!bus)
    {
      if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        {
          g_warning ("Failed to get session bus: %s", error->message);
        }
      return;
    }

  self->manager = cc_display_config_manager_dbus_new ();
  g_signal_connect_object (self->manager, "changed",
                           G_CALLBACK (on_screen_changed),
                           self,
                           G_CONNECT_SWAPPED);

  if (WAYLAND_SESSION ())
    {
      g_dbus_proxy_new (bus,
                        G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START |
                            G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES |
                            G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS,
                        NULL,
                        "org.Cinnamon",
                        "/org/Cinnamon",
                        "org.Cinnamon",
                        NULL,
                        (GAsyncReadyCallback) cinnamon_proxy_ready,
                        self);
    }
}

static void
experimental_features_changed (CcDisplayPanel *self)
{
  on_monitor_settings_updated_cb (self, self->current_output, self->settings);
}

static void
apply_button_clicked_cb (GtkWidget *widget,
                         gpointer   user_data)
{
    apply_current_configuration (CC_DISPLAY_PANEL (user_data));
}

static void
cancel_button_clicked_cb (GtkWidget *widget,
                         gpointer   user_data)
{
    on_screen_changed (CC_DISPLAY_PANEL (user_data));
}

static void
config_file_deleted (GObject *xml,
                     GAsyncResult *res,
                     gpointer   user_data)
{
  GError *error = NULL;

  if (!g_file_delete_finish (G_FILE (xml), res, &error))
    {
      if (error != NULL)
        {
          g_warning ("Problem deleting ~/.config/cinnamon-monitors.xml: %s", error->message);
          g_clear_error (&error);
        }

      return;
    }

  g_spawn_command_line_async ("cinnamon-dbus-command RestartCinnamon 0", NULL);
}

static gboolean
reset_to_defaults (gpointer data)
{
  CcDisplayPanel *panel = CC_DISPLAY_PANEL (data);

  gchar *path = g_build_filename (g_get_user_config_dir (), "cinnamon-monitors.xml", NULL);
  GFile *xml = g_file_new_for_path (path);
  g_free (path);

  g_file_delete_async (xml,
                       G_PRIORITY_DEFAULT,
                       NULL,
                       (GAsyncReadyCallback) (config_file_deleted),
                       panel);

  g_object_unref (xml);

  return G_SOURCE_REMOVE;
}

static void
defaults_button_clicked_cb (GtkWidget *widget,
                            gpointer   user_data)
{
  GtkWidget *dialog_w, *w;
  GtkBox *box;
  GtkDialog *dialog;
  gint response;

  dialog_w = gtk_dialog_new ();
  dialog = GTK_DIALOG (dialog_w);

  gtk_window_set_transient_for (GTK_WINDOW (dialog_w), GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (user_data))));
  gtk_window_set_default_size (GTK_WINDOW (dialog_w), 300, -1);
  gtk_window_set_title (GTK_WINDOW (dialog_w), _("Confirm reset to defaults"));

  gtk_dialog_add_buttons(dialog, _("Cancel"), GTK_RESPONSE_NO, _("Continue"), GTK_RESPONSE_YES, NULL);
  gtk_dialog_set_default_response(dialog, GTK_RESPONSE_NO);

  w = gtk_dialog_get_widget_for_response(dialog, GTK_RESPONSE_YES);
  gtk_style_context_add_class (gtk_widget_get_style_context (w), GTK_STYLE_CLASS_DESTRUCTIVE_ACTION);

  box = GTK_BOX (gtk_dialog_get_content_area (dialog));
  gtk_container_set_border_width (GTK_CONTAINER (box), 15);
  gtk_box_set_spacing (box, 6);

  w = gtk_image_new_from_icon_name ("dialog-warning", GTK_ICON_SIZE_DIALOG);
  gtk_box_pack_start (box, w, FALSE, FALSE, 6);

  w = gtk_label_new (_("This will remove all existing display configurations and reset the current layout."));
  gtk_box_pack_start (box, w, FALSE, FALSE, 0);

  gtk_widget_show_all (dialog_w);

  response = gtk_dialog_run (dialog);

  gtk_widget_destroy (dialog_w);

  if (response == GTK_RESPONSE_YES)
    {
      g_timeout_add (500, (GSourceFunc) reset_to_defaults, user_data);
    }
}

static void
regenerate_palette (CcDisplayPanel *panel, gint n_outputs)
{
    /* The idea is that we go around an hue color wheel.  We want to start
     * at red, go around to green/etc. and stop at blue.
     *
     */
    double start_hue;
    double end_hue;
    int i;

    g_clear_pointer (&panel->palette, g_free);
    panel->palette = g_new (GdkRGBA, n_outputs);

    start_hue = 0.0; /* red */
    end_hue   = 2.0/3; /* blue */

    for (i = 0; i < n_outputs; i++) {
        double h, s, v;
        double r, g, b;

        h = start_hue + (end_hue - start_hue) / n_outputs * i;
        s = 0.6;
        v = 1.0;

        gtk_hsv_to_rgb (h, s, v, &r, &g, &b);

        panel->palette[i].red   = r;
        panel->palette[i].green = g;
        panel->palette[i].blue  = b;
        panel->palette[i].alpha  = 1.0;
    }

    panel->n_outputs = n_outputs;
}

static gchar *
get_color_string_for_output (CcDisplayPanel *panel, gint index)
{
  if (index < 0 || index > panel->n_outputs - 1)
  {
    return g_strdup ("white");
  }

  return gdk_rgba_to_string (&panel->palette[index]);
}

static gchar *
get_output_color (GObject *source, gint index, CcDisplayPanel *self)
{
  return get_color_string_for_output (self, index);
}

static void
cc_display_panel_init (CcDisplayPanel *self)
{
  g_autoptr(GtkCssProvider) provider = NULL;
  GtkCellRenderer *renderer;

  g_resources_register (cc_display_get_resource ());

  self->builder = gtk_builder_new_from_resource ("/org/cinnamon/control-center/display/cc-display-panel.ui");

  gtk_container_add (GTK_CONTAINER (self), WID ("toplevel"));

  self->arrangement_frame = WID ("arrangement_frame");
  self->arrangement_bin = WID ("arrangement_bin");
  self->config_type_revealer = WID ("config_type_revealer");
  self->config_type_join = WID ("config_type_join");
  self->config_type_mirror = WID ("config_type_mirror");
  self->display_settings_frame = WID ("display_settings_frame");
  self->output_enabled_switch = WID ("output_enabled_switch");
  self->output_selection_combo = WID ("output_selection_combo");
  self->primary_display_toggle = WID ("primary_display_toggle");
  self->stack_switcher = WID ("stack_switcher");
  self->apply_button = WID ("apply_button");
  self->cancel_button = WID ("cancel_button");
  self->defaults_button = WID ("defaults_button");

  gtk_builder_add_callback_symbol (self->builder, "on_config_type_toggled_cb", G_CALLBACK (on_config_type_toggled_cb));
  gtk_builder_add_callback_symbol (self->builder, "on_output_enabled_active_changed_cb", G_CALLBACK (on_output_enabled_active_changed_cb));
  gtk_builder_add_callback_symbol (self->builder, "on_output_selection_combo_changed_cb", G_CALLBACK (on_output_selection_combo_changed_cb));
  gtk_builder_add_callback_symbol (self->builder, "primary_display_toggle_toggled_cb", G_CALLBACK (primary_display_toggle_toggled_cb));
  gtk_builder_add_callback_symbol (self->builder, "apply_button_clicked_cb", G_CALLBACK (apply_button_clicked_cb));
  gtk_builder_add_callback_symbol (self->builder, "cancel_button_clicked_cb", G_CALLBACK (cancel_button_clicked_cb));
  gtk_builder_add_callback_symbol (self->builder, "defaults_button_clicked_cb", G_CALLBACK (defaults_button_clicked_cb));

  gtk_builder_connect_signals (self->builder, self);

  self->muffin_settings = g_settings_new ("org.cinnamon.muffin");
  g_signal_connect_swapped (self->muffin_settings, "changed::experimental-features", G_CALLBACK (experimental_features_changed), self);

  self->arrangement = cc_display_arrangement_new (NULL);

  gtk_widget_show (GTK_WIDGET (self->arrangement));
  gtk_widget_set_size_request (GTK_WIDGET (self->arrangement), 400, 175);
  gtk_container_add (GTK_CONTAINER (self->arrangement_bin), GTK_WIDGET (self->arrangement));

  g_signal_connect_object (self->arrangement, "updated",
			   G_CALLBACK (update_bottom_buttons), self,
			   G_CONNECT_SWAPPED);
  g_signal_connect_object (self->arrangement, "notify::selected-output",
			   G_CALLBACK (on_arrangement_selected_ouptut_changed_cb), self,
			   G_CONNECT_SWAPPED);
  g_signal_connect_object (self->arrangement, "get-output-color",
               G_CALLBACK (get_output_color), self, 0);

  self->settings = cc_display_settings_new ();
  gtk_widget_show (GTK_WIDGET (self->settings));
  gtk_container_add (GTK_CONTAINER (self->display_settings_frame), GTK_WIDGET (self->settings));
  g_signal_connect_object (self->settings, "updated",
                           G_CALLBACK (on_monitor_settings_updated_cb), self,
                           G_CONNECT_SWAPPED);

  self->output_selection_list = gtk_list_store_new (3, G_TYPE_STRING, CC_TYPE_DISPLAY_MONITOR, GDK_TYPE_PIXBUF);
  gtk_combo_box_set_model (GTK_COMBO_BOX (self->output_selection_combo), GTK_TREE_MODEL (self->output_selection_list));
  gtk_cell_layout_clear (GTK_CELL_LAYOUT (self->output_selection_combo));

  renderer = gtk_cell_renderer_pixbuf_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (self->output_selection_combo),
                              renderer,
                              FALSE);
  gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (self->output_selection_combo),
                                 renderer,
                                 "pixbuf",
                                 2);
  gtk_cell_renderer_set_visible (renderer, TRUE);

  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (self->output_selection_combo),
                              renderer,
                              TRUE);
  gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (self->output_selection_combo),
                                 renderer,
                                 "text",
                                 0);
  gtk_cell_renderer_set_visible (renderer, TRUE);
  g_object_set (renderer, "xpad", 6, NULL);


  self->up_client = up_client_new ();
  if (up_client_get_lid_is_present (self->up_client))
    {
      g_signal_connect (self->up_client, "notify::lid-is-closed",
                        G_CALLBACK (cc_display_panel_up_client_changed), self);
      cc_display_panel_up_client_changed (self->up_client, NULL, self);
    }
  else
    g_clear_object (&self->up_client);

  self->cancellable = g_cancellable_new ();

  g_bus_get (G_BUS_TYPE_SESSION,
             self->cancellable,
             (GAsyncReadyCallback) session_bus_ready,
             self);

  self->sensor_watch_id = g_bus_watch_name (G_BUS_TYPE_SYSTEM,
                                            "net.hadess.SensorProxy",
                                            G_BUS_NAME_WATCHER_FLAGS_NONE,
                                            sensor_proxy_appeared_cb,
                                            sensor_proxy_vanished_cb,
                                            self,
                                            NULL);

  provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_resource (provider, "/org/cinnamon/control-center/display/display-arrangement.css");
  gtk_style_context_add_provider_for_screen (gdk_screen_get_default (),
                                             GTK_STYLE_PROVIDER (provider),
                                             GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  g_signal_connect (GTK_WIDGET (self), "show",
                    G_CALLBACK (widget_visible_changed), NULL);
  g_signal_connect (GTK_WIDGET (self), "hide",
                    G_CALLBACK (widget_visible_changed), NULL);
}

void
cc_display_panel_register (GIOModule *module)
{
  textdomain (GETTEXT_PACKAGE);
  bindtextdomain (GETTEXT_PACKAGE, LOCALE_DIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

  cc_display_panel_register_type (G_TYPE_MODULE (module));
  g_io_extension_point_implement (CC_SHELL_PANEL_EXTENSION_POINT,
                                  CC_TYPE_DISPLAY_PANEL,
                                  "display", 0);
}
