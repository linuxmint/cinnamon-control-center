/*
 * Copyright (C) 2016  Red Hat, Inc.
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

#define MUFFIN_SCHEMA                     "org.cinnamon.muffin"
#define MUFFIN_EXPERIMENTAL_FEATURES_KEY  "experimental-features"
#define MUFFIN_FEATURE_FRACTIONAL_SCALING_X11 "x11-randr-fractional-scaling"
#define MUFFIN_FEATURE_FRACTIONAL_SCALING_WAYLAND "scale-monitor-framebuffer"

#include <gdk/gdk.h>
#include <gio/gio.h>
#include <math.h>
#include "cc-display-config.h"

#ifdef GDK_WINDOWING_WAYLAND
#include <gdk/gdkwayland.h>
#endif
#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif

static const double known_diagonals[] = {
  12.1,
  13.3,
  15.6
};

static char *
diagonal_to_str (double d)
{
  int i;

  for (i = 0; i < G_N_ELEMENTS (known_diagonals); i++)
    {
      double delta;

      delta = fabs(known_diagonals[i] - d);
      if (delta < 0.1)
          return g_strdup_printf ("%0.1lf\"", known_diagonals[i]);
    }

  return g_strdup_printf ("%d\"", (int) (d + 0.5));
}

static char *
make_display_size_string (int width_mm,
                          int height_mm)
{
  char *inches = NULL;

  if (width_mm > 0 && height_mm > 0)
    {
      double d = sqrt (width_mm * width_mm + height_mm * height_mm);

      inches = diagonal_to_str (d / 25.4);
    }

  return inches;
}

static char *
make_output_ui_name (CcDisplayMonitor *output)
{
  int width_mm, height_mm;
  g_autofree char *size = NULL;

  cc_display_monitor_get_physical_size (output, &width_mm, &height_mm);
  size = make_display_size_string (width_mm, height_mm);
  if (size)
    return g_strdup_printf ("%s (%s)", cc_display_monitor_get_display_name (output), size);
  else
    return g_strdup_printf ("%s", cc_display_monitor_get_display_name (output));
}



G_DEFINE_TYPE (CcDisplayMode,
               cc_display_mode,
               G_TYPE_OBJECT)

static void
cc_display_mode_init (CcDisplayMode *self)
{
}

static void
cc_display_mode_class_init (CcDisplayModeClass *klass)
{
}

void
cc_display_mode_get_resolution (CcDisplayMode *self, int *w, int *h)
{
  return CC_DISPLAY_MODE_GET_CLASS (self)->get_resolution (self, w, h);
}

const double *
cc_display_mode_get_supported_scales (CcDisplayMode *self)
{
  return CC_DISPLAY_MODE_GET_CLASS (self)->get_supported_scales (self);
}

double
cc_display_mode_get_preferred_scale (CcDisplayMode *self)
{
  return CC_DISPLAY_MODE_GET_CLASS (self)->get_preferred_scale (self);
}

gboolean
cc_display_mode_is_interlaced (CcDisplayMode *self)
{
  return CC_DISPLAY_MODE_GET_CLASS (self)->is_interlaced (self);
}

int
cc_display_mode_get_freq (CcDisplayMode *self)
{
  return CC_DISPLAY_MODE_GET_CLASS (self)->get_freq (self);
}

double
cc_display_mode_get_freq_f (CcDisplayMode *self)
{
  return CC_DISPLAY_MODE_GET_CLASS (self)->get_freq_f (self);
}


struct _CcDisplayMonitorPrivate {
  int ui_number;
  gchar *ui_name;
  gchar *ui_number_name;
  gboolean is_usable;

  GdkRectangle disabled_rect;
};
typedef struct _CcDisplayMonitorPrivate CcDisplayMonitorPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (CcDisplayMonitor,
                            cc_display_monitor,
                            G_TYPE_OBJECT)

static void
cc_display_monitor_init (CcDisplayMonitor *self)
{
  CcDisplayMonitorPrivate *priv = cc_display_monitor_get_instance_private (self);

  priv->ui_number = 0;
  priv->ui_name = NULL;
  priv->ui_number_name = NULL;
  priv->is_usable = TRUE;
}

static void
cc_display_monitor_finalize (GObject *object)
{
  CcDisplayMonitor *self = CC_DISPLAY_MONITOR (object);
  CcDisplayMonitorPrivate *priv = cc_display_monitor_get_instance_private (self);

  g_clear_pointer (&priv->ui_name, g_free);
  g_clear_pointer (&priv->ui_number_name, g_free);

  G_OBJECT_CLASS (cc_display_monitor_parent_class)->finalize (object);
}

static void
cc_display_monitor_class_init (CcDisplayMonitorClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = cc_display_monitor_finalize;

  g_signal_new ("rotation",
                CC_TYPE_DISPLAY_MONITOR,
                G_SIGNAL_RUN_LAST,
                0, NULL, NULL, NULL,
                G_TYPE_NONE, 0);
  g_signal_new ("mode",
                CC_TYPE_DISPLAY_MONITOR,
                G_SIGNAL_RUN_LAST,
                0, NULL, NULL, NULL,
                G_TYPE_NONE, 0);
  g_signal_new ("primary",
                CC_TYPE_DISPLAY_MONITOR,
                G_SIGNAL_RUN_LAST,
                0, NULL, NULL, NULL,
                G_TYPE_NONE, 0);
  g_signal_new ("active",
                CC_TYPE_DISPLAY_MONITOR,
                G_SIGNAL_RUN_LAST,
                0, NULL, NULL, NULL,
                G_TYPE_NONE, 0);
  g_signal_new ("scale",
                CC_TYPE_DISPLAY_MONITOR,
                G_SIGNAL_RUN_LAST,
                0, NULL, NULL, NULL,
                G_TYPE_NONE, 0);
  g_signal_new ("position-changed",
                CC_TYPE_DISPLAY_MONITOR,
                G_SIGNAL_RUN_LAST,
                0, NULL, NULL, NULL,
                G_TYPE_NONE, 0);
  g_signal_new ("is-usable",
                CC_TYPE_DISPLAY_MONITOR,
                G_SIGNAL_RUN_LAST,
                0, NULL, NULL, NULL,
                G_TYPE_NONE, 0);
}

const char *
cc_display_monitor_get_display_name (CcDisplayMonitor *self)
{
  return CC_DISPLAY_MONITOR_GET_CLASS (self)->get_display_name (self);
}

const char *
cc_display_monitor_get_connector_name (CcDisplayMonitor *self)
{
  return CC_DISPLAY_MONITOR_GET_CLASS (self)->get_connector_name (self);
}

gboolean
cc_display_monitor_is_builtin (CcDisplayMonitor *self)
{
  return CC_DISPLAY_MONITOR_GET_CLASS (self)->is_builtin (self);
}

gboolean
cc_display_monitor_is_primary (CcDisplayMonitor *self)
{
  return CC_DISPLAY_MONITOR_GET_CLASS (self)->is_primary (self);
}

void
cc_display_monitor_set_primary (CcDisplayMonitor *self, gboolean primary)
{
  return CC_DISPLAY_MONITOR_GET_CLASS (self)->set_primary (self, primary);
}

gboolean
cc_display_monitor_is_active (CcDisplayMonitor *self)
{
  return CC_DISPLAY_MONITOR_GET_CLASS (self)->is_active (self);
}

void
cc_display_monitor_set_active (CcDisplayMonitor *self, gboolean active)
{
  return CC_DISPLAY_MONITOR_GET_CLASS (self)->set_active (self, active);
}

CcDisplayRotation
cc_display_monitor_get_rotation (CcDisplayMonitor *self)
{
  return CC_DISPLAY_MONITOR_GET_CLASS (self)->get_rotation (self);
}

void
cc_display_monitor_set_rotation (CcDisplayMonitor *self,
                                 CcDisplayRotation rotation)
{
  return CC_DISPLAY_MONITOR_GET_CLASS (self)->set_rotation (self, rotation);
}

gboolean
cc_display_monitor_supports_rotation (CcDisplayMonitor *self, CcDisplayRotation r)
{
  return CC_DISPLAY_MONITOR_GET_CLASS (self)->supports_rotation (self, r);
}

void
cc_display_monitor_get_physical_size (CcDisplayMonitor *self, int *w, int *h)
{
  return CC_DISPLAY_MONITOR_GET_CLASS (self)->get_physical_size (self, w, h);
}

void
cc_display_monitor_get_geometry (CcDisplayMonitor *self, int *x, int *y, int *w, int *h)
{
  return CC_DISPLAY_MONITOR_GET_CLASS (self)->get_geometry (self, x, y, w, h);
}

void
cc_display_monitor_get_disabled_geometry (CcDisplayMonitor *self, int *x1, int *y1, int *x2, int *y2)
{
  CcDisplayMonitorPrivate *priv = cc_display_monitor_get_instance_private (self);

  *x1 = priv->disabled_rect.x;
  *y1 = priv->disabled_rect.y;
  *x2 = priv->disabled_rect.x + priv->disabled_rect.width;
  *y2 = priv->disabled_rect.y + priv->disabled_rect.height;
}

void
cc_display_monitor_set_disabled_geometry (CcDisplayMonitor *self, int x, int y, int w, int h)
{
  CcDisplayMonitorPrivate *priv = cc_display_monitor_get_instance_private (self);

  priv->disabled_rect.x = x;
  priv->disabled_rect.y = y;
  priv->disabled_rect.width = w;
  priv->disabled_rect.height = h;
}

CcDisplayMode *
cc_display_monitor_get_mode (CcDisplayMonitor *self)
{
  return CC_DISPLAY_MONITOR_GET_CLASS (self)->get_mode (self);
}

CcDisplayMode *
cc_display_monitor_get_preferred_mode (CcDisplayMonitor *self)
{
  return CC_DISPLAY_MONITOR_GET_CLASS (self)->get_preferred_mode (self);
}

guint32
cc_display_monitor_get_id (CcDisplayMonitor *self)
{
  return CC_DISPLAY_MONITOR_GET_CLASS (self)->get_id (self);
}

GList *
cc_display_monitor_get_modes (CcDisplayMonitor *self)
{
  return CC_DISPLAY_MONITOR_GET_CLASS (self)->get_modes (self);
}

gboolean
cc_display_monitor_supports_underscanning (CcDisplayMonitor *self)
{
  return CC_DISPLAY_MONITOR_GET_CLASS (self)->supports_underscanning (self);
}

gboolean
cc_display_monitor_get_underscanning (CcDisplayMonitor *self)
{
  return CC_DISPLAY_MONITOR_GET_CLASS (self)->get_underscanning (self);
}

void
cc_display_monitor_set_underscanning (CcDisplayMonitor *self,
                                      gboolean underscanning)
{
  return CC_DISPLAY_MONITOR_GET_CLASS (self)->set_underscanning (self, underscanning);
}

void
cc_display_monitor_set_mode (CcDisplayMonitor *self, CcDisplayMode *m)
{
  return CC_DISPLAY_MONITOR_GET_CLASS (self)->set_mode (self, m);
}

void
cc_display_monitor_set_position (CcDisplayMonitor *self, int x, int y)
{
  return CC_DISPLAY_MONITOR_GET_CLASS (self)->set_position (self, x, y);
}

double
cc_display_monitor_get_scale (CcDisplayMonitor *self)
{
  return CC_DISPLAY_MONITOR_GET_CLASS (self)->get_scale (self);
}

void
cc_display_monitor_set_scale (CcDisplayMonitor *self, double s)
{
  return CC_DISPLAY_MONITOR_GET_CLASS (self)->set_scale (self, s);
}

gboolean
cc_display_monitor_is_useful (CcDisplayMonitor *self)
{
  CcDisplayMonitorPrivate *priv = cc_display_monitor_get_instance_private (self);

  return priv->is_usable &&
         cc_display_monitor_is_active (self);
}

gboolean
cc_display_monitor_is_usable (CcDisplayMonitor *self)
{
  CcDisplayMonitorPrivate *priv = cc_display_monitor_get_instance_private (self);

  return priv->is_usable;
}

void
cc_display_monitor_set_usable (CcDisplayMonitor *self, gboolean is_usable)
{
  CcDisplayMonitorPrivate *priv = cc_display_monitor_get_instance_private (self);

  priv->is_usable = is_usable;

  g_signal_emit_by_name (self, "is-usable");
}

gint
cc_display_monitor_get_ui_number (CcDisplayMonitor *self)
{
  CcDisplayMonitorPrivate *priv = cc_display_monitor_get_instance_private (self);

  return priv->ui_number;
}

const char *
cc_display_monitor_get_ui_name (CcDisplayMonitor *self)
{
  CcDisplayMonitorPrivate *priv = cc_display_monitor_get_instance_private (self);

  return priv->ui_name;
}

const char *
cc_display_monitor_get_ui_number_name (CcDisplayMonitor *self)
{
  CcDisplayMonitorPrivate *priv = cc_display_monitor_get_instance_private (self);

  return priv->ui_number_name;
}

char *
cc_display_monitor_dup_ui_number_name (CcDisplayMonitor *self)
{
  CcDisplayMonitorPrivate *priv = cc_display_monitor_get_instance_private (self);

  return g_strdup (priv->ui_number_name);
}

static void
cc_display_monitor_set_ui_info (CcDisplayMonitor *self, gint ui_number, gchar *ui_name)
{

  CcDisplayMonitorPrivate *priv = cc_display_monitor_get_instance_private (self);

  priv->ui_number = ui_number;
  g_free (priv->ui_name);
  priv->ui_name = ui_name;
  priv->ui_number_name = g_strdup_printf ("%d\u2003%s", ui_number, ui_name);
}

struct _CcDisplayConfigPrivate {
  GList *ui_sorted_monitors;

  GSettings *muffin_settings;
  gboolean fractional_scaling;
  gboolean fractional_scaling_pending_disable;
};
typedef struct _CcDisplayConfigPrivate CcDisplayConfigPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (CcDisplayConfig,
                            cc_display_config,
                            G_TYPE_OBJECT)

static const char *
get_fractional_scaling_key (void)
{
  GdkDisplay *display;

  display = gdk_display_get_default ();

#if defined(GDK_WINDOWING_X11)
  if (GDK_IS_X11_DISPLAY (display))
    return MUFFIN_FEATURE_FRACTIONAL_SCALING_X11;
#endif /* GDK_WINDOWING_X11 */
#if defined(GDK_WINDOWING_WAYLAND)
  if (GDK_IS_WAYLAND_DISPLAY (display))
    return MUFFIN_FEATURE_FRACTIONAL_SCALING_WAYLAND;
#endif /* GDK_WINDOWING_WAYLAND */
  g_return_val_if_reached (NULL);
}

static gboolean
get_fractional_scaling_active (CcDisplayConfig *self)
{
  CcDisplayConfigPrivate *priv = cc_display_config_get_instance_private (self);
  g_auto(GStrv) features = NULL;
  const char *key = get_fractional_scaling_key ();

  g_return_val_if_fail (key, FALSE);

  features = g_settings_get_strv (priv->muffin_settings, MUFFIN_EXPERIMENTAL_FEATURES_KEY);
  return g_strv_contains ((const gchar **) features, key);
}

static void
set_fractional_scaling_active (CcDisplayConfig *self,
                               gboolean         enable)
{
  CcDisplayConfigPrivate *priv = cc_display_config_get_instance_private (self);
  g_auto(GStrv) existing_features = NULL;
  gboolean have_fractional_scaling = FALSE;
  g_autoptr(GVariantBuilder) builder = NULL;
  const char *key = get_fractional_scaling_key ();

  /* Add or remove the fractional scaling feature from muffin */
  existing_features = g_settings_get_strv (priv->muffin_settings,
                                           MUFFIN_EXPERIMENTAL_FEATURES_KEY);
  builder = g_variant_builder_new (G_VARIANT_TYPE ("as"));
  for (int i = 0; existing_features[i] != NULL; i++)
    {
      if (g_strcmp0 (existing_features[i], key) == 0)
        {
          if (enable)
            have_fractional_scaling = TRUE;
          else
            continue;
        }

      g_variant_builder_add (builder, "s", existing_features[i]);
    }
  if (enable && !have_fractional_scaling && key)
    g_variant_builder_add (builder, "s", key);

  g_settings_set_value (priv->muffin_settings, MUFFIN_EXPERIMENTAL_FEATURES_KEY,
                        g_variant_builder_end (builder));
}

static void
cc_display_config_init (CcDisplayConfig *self)
{
  CcDisplayConfigPrivate *priv = cc_display_config_get_instance_private (self);

  priv->ui_sorted_monitors = NULL;

  /* No need to connect to the setting, as we'll get notified by mutter */
  priv->muffin_settings = g_settings_new (MUFFIN_SCHEMA);
  priv->fractional_scaling = get_fractional_scaling_active (self);
}

static void
cc_display_config_constructed (GObject *object)
{
  CcDisplayConfig *self = CC_DISPLAY_CONFIG (object);
  CcDisplayConfigPrivate *priv = cc_display_config_get_instance_private (self);
  GList *monitors = cc_display_config_get_monitors (self);
  GList *item;
  gint ui_number = 1;

  for (item = monitors; item != NULL; item = item->next)
    {
      CcDisplayMonitor *monitor = item->data;

      if (cc_display_monitor_is_builtin (monitor))
        priv->ui_sorted_monitors = g_list_prepend (priv->ui_sorted_monitors, monitor);
      else
        priv->ui_sorted_monitors = g_list_append (priv->ui_sorted_monitors, monitor);
    }

  for (item = priv->ui_sorted_monitors; item != NULL; item = item->next)
    {
      CcDisplayMonitor *monitor = item->data;
      char *ui_name;
      ui_name = make_output_ui_name (monitor);

      cc_display_monitor_set_ui_info (monitor, ui_number, ui_name);

      ui_number += 1;
    }
}

static void
cc_display_config_finalize (GObject *object)
{
  CcDisplayConfig *self = CC_DISPLAY_CONFIG (object);
  CcDisplayConfigPrivate *priv = cc_display_config_get_instance_private (self);

  g_list_free (priv->ui_sorted_monitors);
  g_clear_object (&priv->muffin_settings);

  G_OBJECT_CLASS (cc_display_config_parent_class)->finalize (object);
}

static void
cc_display_config_class_init (CcDisplayConfigClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  g_signal_new ("primary",
                CC_TYPE_DISPLAY_CONFIG,
                G_SIGNAL_RUN_LAST,
                0, NULL, NULL, NULL,
                G_TYPE_NONE, 0);

  gobject_class->constructed = cc_display_config_constructed;
  gobject_class->finalize = cc_display_config_finalize;
}

GList *
cc_display_config_get_monitors (CcDisplayConfig *self)
{
  g_return_val_if_fail (CC_IS_DISPLAY_CONFIG (self), NULL);
  return CC_DISPLAY_CONFIG_GET_CLASS (self)->get_monitors (self);
}

GList *
cc_display_config_get_ui_sorted_monitors (CcDisplayConfig *self)
{
  CcDisplayConfigPrivate *priv = cc_display_config_get_instance_private (self);

  g_return_val_if_fail (CC_IS_DISPLAY_CONFIG (self), NULL);
  return priv->ui_sorted_monitors;
}

int
cc_display_config_count_useful_monitors (CcDisplayConfig *self)
{
  CcDisplayConfigPrivate *priv = cc_display_config_get_instance_private (self);
  GList *outputs, *l;
  guint count = 0;

  g_return_val_if_fail (CC_IS_DISPLAY_CONFIG (self), 0);

  outputs = priv->ui_sorted_monitors;
  for (l = outputs; l != NULL; l = l->next)
    {
      CcDisplayMonitor *output = l->data;
      if (!cc_display_monitor_is_useful (output))
        continue;
      else
        count++;
    }
  return count;

}

gboolean
cc_display_config_is_applicable (CcDisplayConfig *self)
{
  g_return_val_if_fail (CC_IS_DISPLAY_CONFIG (self), FALSE);
  return CC_DISPLAY_CONFIG_GET_CLASS (self)->is_applicable (self);
}

void
cc_display_config_set_mode_on_all_outputs (CcDisplayConfig *config,
                                           CcDisplayMode   *mode)
{
  GList *outputs, *l;

  g_return_if_fail (CC_IS_DISPLAY_CONFIG (config));

  outputs = cc_display_config_get_monitors (config);
  for (l = outputs; l; l = l->next)
    {
      CcDisplayMonitor *output = l->data;
      cc_display_monitor_set_mode (output, mode);
      cc_display_monitor_set_position (output, 0, 0);
    }
}

gboolean
cc_display_config_equal (CcDisplayConfig *self,
                         CcDisplayConfig *other)
{
  CcDisplayConfigPrivate *spriv = cc_display_config_get_instance_private (self);
  CcDisplayConfigPrivate *opriv = cc_display_config_get_instance_private (other);

  g_return_val_if_fail (CC_IS_DISPLAY_CONFIG (self), FALSE);
  g_return_val_if_fail (CC_IS_DISPLAY_CONFIG (other), FALSE);

  if (spriv->fractional_scaling_pending_disable !=
      opriv->fractional_scaling_pending_disable)
    return FALSE;

  return CC_DISPLAY_CONFIG_GET_CLASS (self)->equal (self, other);
}

gboolean
cc_display_config_apply (CcDisplayConfig *self,
                         GError **error)
{
  CcDisplayConfigPrivate *priv = cc_display_config_get_instance_private (self);

  if (!CC_IS_DISPLAY_CONFIG (self))
    {
      g_warning ("Cannot apply invalid configuration");
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_FAILED,
                   "Cannot apply invalid configuration");
      return FALSE;
    }

  if (priv->fractional_scaling_pending_disable)
    {
      set_fractional_scaling_active (self, FALSE);
      priv->fractional_scaling_pending_disable = FALSE;
    }

  return CC_DISPLAY_CONFIG_GET_CLASS (self)->apply (self, error);
}

gboolean
cc_display_config_is_cloning (CcDisplayConfig *self)
{
  g_return_val_if_fail (CC_IS_DISPLAY_CONFIG (self), FALSE);
  return CC_DISPLAY_CONFIG_GET_CLASS (self)->is_cloning (self);
}

void
cc_display_config_set_cloning (CcDisplayConfig *self,
                               gboolean clone)
{
  g_return_if_fail (CC_IS_DISPLAY_CONFIG (self));
  return CC_DISPLAY_CONFIG_GET_CLASS (self)->set_cloning (self, clone);
}

GList *
cc_display_config_get_cloning_modes (CcDisplayConfig *self)
{
  g_return_val_if_fail (CC_IS_DISPLAY_CONFIG (self), NULL);
  return CC_DISPLAY_CONFIG_GET_CLASS (self)->get_cloning_modes (self);
}

gboolean
cc_display_config_is_layout_logical (CcDisplayConfig *self)
{
  g_return_val_if_fail (CC_IS_DISPLAY_CONFIG (self), FALSE);
  return CC_DISPLAY_CONFIG_GET_CLASS (self)->is_layout_logical (self);
}

void
cc_display_config_set_minimum_size (CcDisplayConfig *self,
                                    int              width,
                                    int              height)
{
  g_return_if_fail (CC_IS_DISPLAY_CONFIG (self));
  CC_DISPLAY_CONFIG_GET_CLASS (self)->set_minimum_size (self, width, height);
}

gint
cc_display_config_get_legacy_ui_scale (CcDisplayConfig *self)
{
  return CC_DISPLAY_CONFIG_GET_CLASS (self)->get_legacy_ui_scale (self);
}

static gboolean
scale_value_is_fractional (double scale)
{
  return (int) scale != scale;
}

gboolean
cc_display_config_is_scaled_mode_valid (CcDisplayConfig *self,
                                        CcDisplayMode   *mode,
                                        double           scale)
{
  CcDisplayConfigPrivate *priv = cc_display_config_get_instance_private (self);

  g_return_val_if_fail (CC_IS_DISPLAY_CONFIG (self), FALSE);
  g_return_val_if_fail (CC_IS_DISPLAY_MODE (mode), FALSE);

  if (priv->fractional_scaling_pending_disable && scale_value_is_fractional (scale))
    return FALSE;

  return CC_DISPLAY_CONFIG_GET_CLASS (self)->is_scaled_mode_valid (self, mode, scale);
}

gboolean
cc_display_config_layout_use_ui_scale (CcDisplayConfig *self)
{
  return CC_DISPLAY_CONFIG_GET_CLASS (self)->layout_use_ui_scale (self);
}

double
cc_display_config_get_maximum_scaling (CcDisplayConfig *self)
{
  GList *outputs, *l;
  double max_scale = 1.0;
  outputs = cc_display_config_get_monitors (self);

  for (l = outputs; l; l = l->next)
    {
      CcDisplayMonitor *output = l->data;

      if (!cc_display_monitor_is_useful (output))
        continue;

      max_scale = MAX (max_scale, cc_display_monitor_get_scale (output));
    }

  return max_scale;
}

static gboolean
set_monitors_scaling_to_preferred_integers (CcDisplayConfig *self)
{
  GList *l;
  gboolean any_changed = FALSE;

  for (l = cc_display_config_get_monitors (self); l; l = l->next)
    {
      CcDisplayMonitor *monitor = l->data;
      gdouble monitor_scale = cc_display_monitor_get_scale (monitor);

      if (scale_value_is_fractional (monitor_scale))
        {
          CcDisplayMode *preferred_mode;
          double preferred_scale;
          double *saved_scale;

          preferred_mode = cc_display_monitor_get_preferred_mode (monitor);
          preferred_scale = cc_display_mode_get_preferred_scale (preferred_mode);
          cc_display_monitor_set_scale (monitor, preferred_scale);
          any_changed = TRUE;

          saved_scale = g_new (double, 1);
          *saved_scale = monitor_scale;
          g_object_set_data_full (G_OBJECT (monitor),
                                  "previous-fractional-scale",
                                  saved_scale, g_free);
        }
      else
        {
          g_signal_emit_by_name (monitor, "scale");
        }
    }

  return any_changed;
}

static void
reset_monitors_scaling_to_selected_values (CcDisplayConfig *self)
{
  GList *l;

  for (l = cc_display_config_get_monitors (self); l; l = l->next)
    {
      CcDisplayMonitor *monitor = l->data;
      gdouble *saved_scale;

      saved_scale = g_object_get_data (G_OBJECT (monitor),
                                       "previous-fractional-scale");

      if (saved_scale)
        {
          cc_display_monitor_set_scale (monitor, *saved_scale);
          g_object_set_data (G_OBJECT (monitor), "previous-fractional-scale", NULL);
        }
      else
        {
          g_signal_emit_by_name (monitor, "scale");
        }
    }
}

void
cc_display_config_set_fractional_scaling (CcDisplayConfig *self,
                                          gboolean         enabled)
{
  CcDisplayConfigPrivate *priv = cc_display_config_get_instance_private (self);

  if (priv->fractional_scaling == enabled)
    return;

  priv->fractional_scaling = enabled;

  if (priv->fractional_scaling)
    {
      if (priv->fractional_scaling_pending_disable)
        {
          priv->fractional_scaling_pending_disable = FALSE;
          reset_monitors_scaling_to_selected_values (self);
        }

      if (!get_fractional_scaling_active (self))
        set_fractional_scaling_active (self, enabled);
    }
  else
    {
      priv->fractional_scaling_pending_disable = TRUE;

      if (!set_monitors_scaling_to_preferred_integers (self))
        {
          gboolean disable_now = FALSE;

          if (cc_display_config_layout_use_ui_scale (self))
            {
              disable_now =
                G_APPROX_VALUE (cc_display_config_get_legacy_ui_scale (self),
                                cc_display_config_get_maximum_scaling (self),
                                DBL_EPSILON);
            }

          if (disable_now)
            {
              priv->fractional_scaling_pending_disable = FALSE;
              reset_monitors_scaling_to_selected_values (self);
              set_fractional_scaling_active (self, enabled);
            }
        }
    }
}

gboolean
cc_display_config_get_fractional_scaling (CcDisplayConfig *self)
{
  CcDisplayConfigPrivate *priv = cc_display_config_get_instance_private (self);

  return priv->fractional_scaling;
}
