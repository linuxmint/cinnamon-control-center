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
#include <libcinnamon-desktop/cdesktop-enums.h>
#include <math.h>

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
  CC_DISPLAY_CONFIG_SINGLE,
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

  gint                  rebuilding_counter;

  CcDisplayArrangement *arrangement;
  CcDisplaySettings    *settings;

  guint           focus_id;

  UpClient *up_client;
  gboolean lid_is_closed;

  GDBusProxy *shell_proxy;

  guint       sensor_watch_id;
  GDBusProxy *iio_sensor_proxy;
  gboolean    has_accelerometer;

  GtkBuilder *builder;

  GtkWidget *apply_button;
  GtkWidget *defaults_button;
  GtkWidget *cancel_button;

  GtkListStore   *primary_display_list;
  GtkListStore   *output_selection_list;

  GtkWidget *arrangement_frame;
  GtkWidget *arrangement_bin;
  GtkWidget *config_type_join;
  GtkWidget *config_type_mirror;
  GtkWidget *config_type_single;
  GtkWidget *config_type_switcher_frame;
  GtkWidget *current_output_label;
  GtkWidget *display_settings_frame;
  GtkWidget *output_enabled_switch;
  GtkWidget *output_selection_combo;
  GtkWidget *output_selection_stack;
  GtkWidget *output_selection_two_first;
  GtkWidget *output_selection_two_second;
  GtkWidget *primary_display_combo;
  GtkWidget *stack_switcher;

  GCancellable   *cancellable;
  GSettings      *muffin_settings;

  CcDisplayLabeler *labeler;
};

CC_PANEL_REGISTER (CcDisplayPanel, cc_display_panel)

static void
update_bottom_buttons (CcDisplayPanel *panel);
static void
apply_current_configuration (CcDisplayPanel *self);
static void
reset_current_config (CcDisplayPanel *panel);
static void
rebuild_ui (CcDisplayPanel *panel);
static void
set_current_output (CcDisplayPanel   *panel,
                    CcDisplayMonitor *output,
                    gboolean          force);


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

  if (n_active_outputs == 1)
    return CC_DISPLAY_CONFIG_SINGLE;

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
  else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (panel->config_type_single)))
    return CC_DISPLAY_CONFIG_SINGLE;
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
    case CC_DISPLAY_CONFIG_SINGLE:
      g_debug ("Creating new single config");
      /* Disable all but the current primary output */
      cc_display_config_set_cloning (panel->current_config, FALSE);
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
      break;

    case CC_DISPLAY_CONFIG_JOIN:
      g_debug ("Creating new join config");
      /* Enable all usable outputs
       * Note that this might result in invalid configurations as we
       * we might not be able to drive all attached monitors. */
      cc_display_config_set_cloning (panel->current_config, FALSE);
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
    case CC_DISPLAY_CONFIG_SINGLE:
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (panel->config_type_single), TRUE);
      break;
    default:
      g_assert_not_reached ();
    }

  config_ensure_of_type (panel, type);
}

static void
monitor_labeler_hide (CcDisplayPanel *self)
{
  if (!self->shell_proxy)
    return;

  g_dbus_proxy_call (self->shell_proxy,
                     "HideMonitorLabels",
                     NULL, G_DBUS_CALL_FLAGS_NONE,
                     -1, NULL, NULL, NULL);
}

static void
monitor_labeler_show (CcDisplayPanel *self)
{
  GList *outputs, *l;
  GVariantBuilder builder;
  gint number = 0;

  if (!self->shell_proxy || !self->current_config)
    return;

  outputs = cc_display_config_get_ui_sorted_monitors (self->current_config);
  if (!outputs)
    return;

  if (cc_display_config_is_cloning (self->current_config))
    return monitor_labeler_hide (self);

  g_variant_builder_init (&builder, G_VARIANT_TYPE_TUPLE);
  g_variant_builder_open (&builder, G_VARIANT_TYPE_ARRAY);

  for (l = outputs; l != NULL; l = l->next)
    {
      CcDisplayMonitor *output = l->data;

      number = cc_display_monitor_get_ui_number (output);
      if (number == 0)
        continue;

      g_variant_builder_add (&builder, "{sv}",
                             cc_display_monitor_get_connector_name (output),
                             g_variant_new_int32 (number));
    }

  g_variant_builder_close (&builder);

  if (number < 2)
    return monitor_labeler_hide (self);

  g_dbus_proxy_call (self->shell_proxy,
                     "ShowMonitorLabels",
                     g_variant_builder_end (&builder),
                     G_DBUS_CALL_FLAGS_NONE,
                     -1, NULL, NULL, NULL);
}

static void
ensure_monitor_labels (CcDisplayPanel *self)
{
  if (self->labeler) {
    cc_display_labeler_hide (self->labeler);
    g_object_unref (self->labeler);
  }

  self->labeler = cc_display_labeler_new (self->current_config);

  cc_display_labeler_hide (self->labeler);
  cc_display_labeler_show (self->labeler);
}

static void
dialog_toplevel_focus_changed (CcDisplayPanel *self)
{
  ensure_monitor_labels (self);
}

static void
cc_display_panel_dispose (GObject *object)
{
  CcDisplayPanel *self = CC_DISPLAY_PANEL (object);
  CcShell *shell;
  GtkWidget *toplevel;

  if (self->sensor_watch_id > 0)
    {
      g_bus_unwatch_name (self->sensor_watch_id);
      self->sensor_watch_id = 0;
    }

  g_clear_object (&self->iio_sensor_proxy);

  g_clear_object (&self->manager);
  g_clear_object (&self->current_config);
  g_clear_object (&self->up_client);

  g_clear_object (&self->shell_proxy);

  g_clear_object (&self->muffin_settings);

  cc_display_labeler_hide (self->labeler);
  g_object_unref (self->labeler);

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
on_output_selection_two_toggled_cb (CcDisplayPanel *panel, GtkRadioButton *btn)
{
  CcDisplayMonitor *output;

  if (panel->rebuilding_counter > 0)
    return;

  if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (btn)))
    return;

  output = g_object_get_data (G_OBJECT (btn), "display");

  /* Stay in single mode when we are in single mode.
   * This UI must never cause a switch between the configuration type.
   * this is in contrast to the combobox monitor selection, which may
   * switch to a disabled output both in SINGLE/MULTI mode without
   * anything changing.
   */
  if (cc_panel_get_selected_type (panel) == CC_DISPLAY_CONFIG_SINGLE)
    {
      if (panel->current_output)
        cc_display_monitor_set_active (panel->current_output, FALSE);
      if (output)
        cc_display_monitor_set_active (output, TRUE);

      update_bottom_buttons (panel);
    }

  set_current_output (panel, g_object_get_data (G_OBJECT (btn), "display"), FALSE);
}

static void
on_primary_display_selected_index_changed_cb (GtkComboBox *box,
                                              CcDisplayPanel *panel)
{
  GtkTreeIter iter;
  if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (box), &iter))
    {
      g_autoptr(CcDisplayMonitor) output = NULL;

      if (panel->rebuilding_counter > 0)
        return;

      gtk_tree_model_get (GTK_TREE_MODEL (panel->primary_display_list),
                          &iter,
                          1, &output,
                          -1);

      if (cc_display_monitor_is_primary (output))
        return;

      cc_display_monitor_set_primary (output, TRUE);
      update_bottom_buttons (panel);
    }
}

static void
cc_display_panel_constructed (GObject *object)
{
  G_OBJECT_CLASS (cc_display_panel_parent_class)->constructed (object);
  // g_signal_connect_object (cc_panel_get_shell (CC_PANEL (object)), "notify::active-panel",
  //                          G_CALLBACK (active_panel_changed), object, 0);

}

static GtkWidget *
cc_display_panel_get_title_widget (CcPanel *panel)
{
  CcDisplayPanel *self = CC_DISPLAY_PANEL (panel);

  return self->stack_switcher;
}

static void
cc_display_panel_class_init (CcDisplayPanelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  // CcPanelClass *panel_class = CC_PANEL_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  // panel_class->get_help_uri = cc_display_panel_get_help_uri;
  // panel_class->get_title_widget = cc_display_panel_get_title_widget;

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

  /* Note, this function is also called if the internal UI needs updating after a rebuild. */
  changed = (output != panel->current_output);

  if (!changed && !force)
    return;

  panel->rebuilding_counter++;

  panel->current_output = output;

  if (panel->current_output)
    {
      gtk_label_set_text (GTK_LABEL (panel->current_output_label), cc_display_monitor_get_ui_name (panel->current_output));
      gtk_switch_set_active (GTK_SWITCH (panel->output_enabled_switch), cc_display_monitor_is_active (panel->current_output));
      gtk_widget_set_sensitive (GTK_WIDGET (panel->output_enabled_switch), cc_display_monitor_is_usable (panel->current_output));
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (panel->current_output_label), "");
      gtk_switch_set_active (GTK_SWITCH (panel->output_enabled_switch), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (panel->output_enabled_switch), FALSE);
    }

  if (g_object_get_data (G_OBJECT (panel->output_selection_two_first), "display") == output)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (panel->output_selection_two_first), TRUE);
  if (g_object_get_data (G_OBJECT (panel->output_selection_two_second), "display") == output)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (panel->output_selection_two_second), TRUE);

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

  panel->rebuilding_counter++;

  gtk_list_store_clear (panel->primary_display_list);
  gtk_list_store_clear (panel->output_selection_list);

  if (!panel->current_config)
    {
      panel->rebuilding_counter--;
      return;
    }

  n_active_outputs = 0;
  n_usable_outputs = 0;
  outputs = cc_display_config_get_ui_sorted_monitors (panel->current_config);
  for (l = outputs; l; l = l->next)
    {
      GtkTreeIter iter;
      CcDisplayMonitor *output = l->data;

      gtk_list_store_append (panel->output_selection_list, &iter);
      gtk_list_store_set (panel->output_selection_list,
                          &iter,
                          0, cc_display_monitor_get_ui_number_name (output),
                          1, output,
                          -1);

      if (!cc_display_monitor_is_usable (output))
        continue;

      n_usable_outputs += 1;

      if (n_usable_outputs == 1)
        {
          gtk_button_set_label (GTK_BUTTON (panel->output_selection_two_first),
                                cc_display_monitor_get_ui_name (output));
          g_object_set_data (G_OBJECT (panel->output_selection_two_first),
                             "display",
                             output);
        }
      else if (n_usable_outputs == 2)
        {
          gtk_button_set_label (GTK_BUTTON (panel->output_selection_two_second),
                                cc_display_monitor_get_ui_name (output));
          g_object_set_data (G_OBJECT (panel->output_selection_two_second),
                             "display",
                             output);
        }

      if (cc_display_monitor_is_active (output))
        {
          GtkTreeIter iter;

          n_active_outputs += 1;

          gtk_list_store_append (panel->primary_display_list, &iter);
          gtk_list_store_set (panel->primary_display_list,
                              &iter,
                              0, cc_display_monitor_get_ui_number_name (output),
                              1, output,
                              -1);

          if (cc_display_monitor_is_primary (output))
            {
              gtk_combo_box_set_active_iter (GTK_COMBO_BOX (panel->primary_display_combo), &iter);
            }

          /* Ensure that an output is selected; note that this doesn't ensure
           * the selected output is any useful (i.e. when switching types).
           */
          if (!panel->current_output)
            set_current_output (panel, output, FALSE);
        }
    }

  /* Sync the rebuild lists/buttons */
  set_current_output (panel, panel->current_output, TRUE);

  n_outputs = g_list_length (outputs);
  type = config_get_current_type (panel);

  if (n_outputs == 2 && n_usable_outputs == 2)
    {
      /* We only show the top chooser with two monitors that are
       * both usable (i.e. two monitors incl. internal and lid not closed).
       * In this case, the arrangement widget is shown if we are in JOIN mode.
       */
      if (type > CC_DISPLAY_CONFIG_LAST_VALID)
        type = CC_DISPLAY_CONFIG_JOIN;

      gtk_widget_set_sensitive (panel->config_type_switcher_frame, TRUE);
      gtk_widget_set_visible (panel->arrangement_frame, type == CC_DISPLAY_CONFIG_JOIN);

      /* We need a switcher except in CLONE mode */
      if (type == CC_DISPLAY_CONFIG_CLONE)
        gtk_stack_set_visible_child_name (GTK_STACK (panel->output_selection_stack), "no-selection");
      else
        gtk_stack_set_visible_child_name (GTK_STACK (panel->output_selection_stack), "two-selection");
    }
  else if (n_usable_outputs > 1)
    {
      /* We have more than one usable monitor. In this case there is no chooser,
       * and we always show the arrangement widget even if we are in SINGLE mode.
       */
      gtk_widget_set_sensitive (panel->config_type_switcher_frame, FALSE);
      gtk_widget_set_visible (panel->arrangement_frame, TRUE);

      /* Mirror is also invalid as it cannot be configured using this UI. */
      if (type == CC_DISPLAY_CONFIG_CLONE || type > CC_DISPLAY_CONFIG_LAST_VALID)
        type = CC_DISPLAY_CONFIG_JOIN;

      gtk_stack_set_visible_child_name (GTK_STACK (panel->output_selection_stack), "multi-selection");
    }
  else
    {
      /* We only have a single usable monitor, show neither configuration type
       * switcher nor arrangement widget and ensure we really are in SINGLE
       * mode (and not e.g. mirroring across one display) */
      type = CC_DISPLAY_CONFIG_SINGLE;

      gtk_widget_set_sensitive (panel->config_type_switcher_frame, FALSE);
      gtk_widget_set_visible (panel->arrangement_frame, FALSE);

      gtk_stack_set_visible_child_name (GTK_STACK (panel->output_selection_stack), "no-selection");
    }

  cc_panel_set_selected_type (panel, type);

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
  cc_display_config_set_minimum_size (current, MINIMUM_WIDTH, MINIMUM_HEIGHT);
  panel->current_config = current;

  gtk_list_store_clear (panel->primary_display_list);
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

  ensure_monitor_labels (panel);
}

static gboolean
on_toplevel_key_press (GtkWidget   *button,
                       GdkEventKey *event)
{
  if (event->keyval != GDK_KEY_Escape)
    return GDK_EVENT_PROPAGATE;

  g_signal_emit_by_name (button, "activate");
  return GDK_EVENT_STOP;
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
mapped_cb (CcDisplayPanel *panel)
{
  CcShell *shell;
  GtkWidget *toplevel;

  shell = cc_panel_get_shell (CC_PANEL (panel));
  toplevel = cc_shell_get_toplevel (shell);
  if (toplevel && !panel->focus_id)
    panel->focus_id = g_signal_connect_swapped (toplevel, "notify::has-toplevel-focus",
                                               G_CALLBACK (dialog_toplevel_focus_changed), panel);
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

// static void
// shell_proxy_ready (GObject        *source,
//                    GAsyncResult   *res,
//                    CcDisplayPanel *self)
// {
//   GDBusProxy *proxy;
//   g_autoptr(GError) error = NULL;

//   proxy = cc_object_storage_create_dbus_proxy_finish (res, &error);
//   if (!proxy)
//     {
//       if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
//         g_warning ("Failed to contact gnome-shell: %s", error->message);
//       return;
//     }

//   self->shell_proxy = proxy;

//   ensure_monitor_labels (self);
// }

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
}

static void
get_primary_display_label (GtkCellLayout   *cell_layout,
                           GtkCellRenderer *cell,
                           GtkTreeModel    *model,
                           GtkTreeIter     *iter,
                           gpointer         data)
{
    CcDisplayMonitor *monitor;
    const gchar *text;

    gtk_tree_model_get (model,
                        iter,
                        1, &monitor,
                        -1);

    text = cc_display_monitor_get_ui_number_name (monitor);

    g_object_set (G_OBJECT (cell),
                  "text", text,
                  NULL);
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
defaults_button_clicked_cb (GtkWidget *widget,
                            gpointer   user_data)
{
  // gchar *path = g_build_filename (g_get_user_config_dir (), "monitors.xml", NULL);
  // GFile *xml = g_file_new_for_path (path);
  // g_free (path);

  // g_file_delete_async (xml,
  //                      G_PRIORITY_DEFAULT,
  //                      NULL,
  //                      (GAsyncReadyCallback) (on_screen_changed),
  //                      CcDisplayPanel (user_data));

  // g_object_unref (xml);
}

static gchar *
get_output_color (CcDisplayArrangement *arrangement, CcDisplayMonitor *output, CcDisplayPanel *self)
{
  GdkRGBA color;

  if (self->labeler != NULL)
    {
      cc_display_labeler_get_rgba_for_output (self->labeler, output, &color);
      return gdk_rgba_to_string (&color);
    }

  return g_strdup ("white");
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
  self->config_type_switcher_frame = WID ("config_type_switcher_frame");
  self->config_type_join = WID ("config_type_join");
  self->config_type_mirror = WID ("config_type_mirror");
  self->config_type_single = WID ("config_type_single");
  self->current_output_label = WID ("current_output_label");
  self->display_settings_frame = WID ("display_settings_frame");
  self->output_enabled_switch = WID ("output_enabled_switch");
  self->output_selection_combo = WID ("output_selection_combo");
  self->output_selection_stack = WID ("output_selection_stack");
  self->output_selection_two_first = WID ("output_selection_two_first");
  self->output_selection_two_second = WID ("output_selection_two_second");
  self->primary_display_combo = WID ("primary_display_combo");
  self->stack_switcher = WID ("stack_switcher");
  self->apply_button = WID ("apply_button");
  self->cancel_button = WID ("cancel_button");
  self->defaults_button = WID ("defaults_button");

  gtk_builder_add_callback_symbol (self->builder, "on_config_type_toggled_cb", G_CALLBACK (on_config_type_toggled_cb));
  gtk_builder_add_callback_symbol (self->builder, "on_output_enabled_active_changed_cb", G_CALLBACK (on_output_enabled_active_changed_cb));
  gtk_builder_add_callback_symbol (self->builder, "on_output_selection_combo_changed_cb", G_CALLBACK (on_output_selection_combo_changed_cb));
  gtk_builder_add_callback_symbol (self->builder, "on_output_selection_two_toggled_cb", G_CALLBACK (on_output_selection_two_toggled_cb));
  gtk_builder_add_callback_symbol (self->builder, "on_primary_display_selected_index_changed_cb", G_CALLBACK (on_primary_display_selected_index_changed_cb));
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

  self->primary_display_list = gtk_list_store_new (2, G_TYPE_STRING, CC_TYPE_DISPLAY_MONITOR);
  gtk_combo_box_set_model (GTK_COMBO_BOX (self->primary_display_combo), GTK_TREE_MODEL (self->primary_display_list));
  gtk_cell_layout_clear (GTK_CELL_LAYOUT (self->primary_display_combo));
  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (self->primary_display_combo),
                              renderer,
                              TRUE);
  gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (self->primary_display_combo),
                                      renderer,
                                      get_primary_display_label,
                                      self,
                                      NULL);
  gtk_cell_renderer_set_visible (renderer, TRUE);

  self->output_selection_list = gtk_list_store_new (2, G_TYPE_STRING, CC_TYPE_DISPLAY_MONITOR);
  gtk_combo_box_set_model (GTK_COMBO_BOX (self->output_selection_combo), GTK_TREE_MODEL (self->output_selection_list));
  gtk_cell_layout_clear (GTK_CELL_LAYOUT (self->output_selection_combo));
  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (self->output_selection_combo),
                              renderer,
                              TRUE);
  gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (self->output_selection_combo),
                                 renderer,
                                 "text",
                                 0);
  gtk_cell_renderer_set_visible (renderer, TRUE);

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

  // g_signal_connect (self, "map", G_CALLBACK (mapped_cb), NULL);

  // cc_object_storage_create_dbus_proxy (G_BUS_TYPE_SESSION,
  //                                      G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES |
  //                                      G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS |
  //                                      G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
  //                                      "org.gnome.Shell",
  //                                      "/org/gnome/Shell",
  //                                      "org.gnome.Shell",
  //                                      cc_panel_get_cancellable (CC_PANEL (self)),
  //                                      (GAsyncReadyCallback) shell_proxy_ready,
  //                                      self);

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
}


void
cc_display_panel_register (GIOModule *module)
{
  textdomain (GETTEXT_PACKAGE);
  bindtextdomain (GETTEXT_PACKAGE, "/usr/share/locale");
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

  cc_display_panel_register_type (G_TYPE_MODULE (module));
  g_io_extension_point_implement (CC_SHELL_PANEL_EXTENSION_POINT,
                                  CC_TYPE_DISPLAY_PANEL,
                                  "display", 0);
}
