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
 * Author: Thomas Wood <thomas.wood@intel.com>
 *
 */

#include "config.h"
#include "cc-datetime-panel.h"

#include <sys/time.h>
#include "cc-timezone-map.h"
#include "cc-dtm-proxy.h"
#include "cc-csddtm-proxy.h"
#include "date-endian.h"
#define GNOME_DESKTOP_USE_UNSTABLE_API

#include <libcinnamon-desktop/cdesktop-enums.h>
#include <string.h>
#include <stdlib.h>
#include <libintl.h>

#include <libcinnamon-desktop/gnome-wall-clock.h>
#include <polkit/polkit.h>

/* FIXME: This should be "Etc/GMT" instead */
#define DEFAULT_TZ "Europe/London"
#define GETTEXT_PACKAGE_TIMEZONES GETTEXT_PACKAGE "-timezones"

CC_PANEL_REGISTER (CcDateTimePanel, cc_date_time_panel)

#define DATE_TIME_PANEL_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CC_TYPE_DATE_TIME_PANEL, CcDateTimePanelPrivate))

enum {
  CITY_COL_CITY,
  CITY_COL_REGION,
  CITY_COL_CITY_TRANSLATED,
  CITY_COL_REGION_TRANSLATED,
  CITY_COL_ZONE,
  CITY_NUM_COLS
};

enum {
  REGION_COL_REGION,
  REGION_COL_REGION_TRANSLATED,
  REGION_NUM_COLS
};

#define W(x) (GtkWidget*) gtk_builder_get_object (priv->builder, x)

#define CLOCK_SCHEMA "org.cinnamon.desktop.interface"
#define CLOCK_USE_24H "clock-use-24h"

struct _CcDateTimePanelPrivate
{
  GtkBuilder *builder;
  GtkWidget *map;
  GtkWidget *lock_button;

  TzLocation *current_location;

  GtkTreeModel *locations;
  GtkTreeModelFilter *city_filter;  

  GDateTime *date;

  gboolean clock_blocked;

  GSettings *settings;
  gboolean clock_use_24h;

  GnomeWallClock *clock_tracker;

  CCDtm *dtm;
  CCCsddtm *csddtm;
  gboolean use_systemd;

  GCancellable *cancellable;

  GPermission *permission;

  guint set_date_time_delay_id;
};

static void update_time (CcDateTimePanel *self);

static void on_clock_changed (GnomeWallClock  *clock,
                              GParamSpec      *pspec,
                              CcDateTimePanel *panel);

static void
cc_date_time_panel_get_property (GObject    *object,
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
cc_date_time_panel_set_property (GObject      *object,
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
cc_date_time_panel_dispose (GObject *object)
{
  CcDateTimePanelPrivate *priv = CC_DATE_TIME_PANEL (object)->priv;

  if (priv->clock_tracker != NULL)
    {
      g_object_unref (priv->clock_tracker);
      priv->clock_tracker = NULL;
    }

  if (priv->builder)
    {
      g_object_unref (priv->builder);
      priv->builder = NULL;
    }

  if (priv->settings)
    {
      g_object_unref (priv->settings);
      priv->settings = NULL;
    }

  if (priv->date)
    {
      g_date_time_unref (priv->date);
      priv->date = NULL;
    }

  if (priv->cancellable)
    {
      g_cancellable_cancel (priv->cancellable);
      g_object_unref (priv->cancellable);
      priv->cancellable = NULL;
    }

  if (priv->dtm)
    {
      g_object_unref (priv->dtm);
      priv->dtm = NULL;
    }

  if (priv->permission)
    {
      g_object_unref (priv->permission);
      priv->permission = NULL;
    }

  G_OBJECT_CLASS (cc_date_time_panel_parent_class)->dispose (object);
}

static GPermission *
cc_date_time_panel_get_permission (CcPanel *panel)
{
  CcDateTimePanelPrivate *priv = CC_DATE_TIME_PANEL (panel)->priv;

  return priv->permission;
}

static const char *
cc_date_time_panel_get_help_uri (CcPanel *panel)
{
  if (!g_strcmp0(g_getenv("XDG_CURRENT_DESKTOP"), "Unity"))
    return "help:ubuntu-help/clock";
  else
    return "help:gnome-help/clock";
}

static void
cc_date_time_panel_class_init (CcDateTimePanelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CcPanelClass *panel_class = CC_PANEL_CLASS (klass);

  g_type_class_add_private (klass, sizeof (CcDateTimePanelPrivate));

  object_class->get_property = cc_date_time_panel_get_property;
  object_class->set_property = cc_date_time_panel_set_property;
  object_class->dispose = cc_date_time_panel_dispose;

  panel_class->get_permission = cc_date_time_panel_get_permission;
  panel_class->get_help_uri   = cc_date_time_panel_get_help_uri;
}

static void clock_settings_changed_cb (GSettings       *settings,
                                       gchar           *key,
                                       CcDateTimePanel *panel);

static void
clock_settings_changed_cb (GSettings       *settings,
                           gchar           *key,
                           CcDateTimePanel *panel)
{
  CcDateTimePanelPrivate *priv = panel->priv;  
  gboolean value;

  value = g_settings_get_boolean (settings, CLOCK_USE_24H);
  priv->clock_use_24h = value;

  g_signal_handlers_block_by_func (settings, clock_settings_changed_cb, panel);
  
  update_time (panel);

  g_signal_handlers_unblock_by_func (settings, clock_settings_changed_cb, panel);
}

static void
update_time (CcDateTimePanel *self)
{
  CcDateTimePanelPrivate *priv = self->priv;
  char *label;

  if (priv->clock_use_24h)
    {
      /* Update the hours label */
      label = g_date_time_format (priv->date, "%H");
      gtk_label_set_text (GTK_LABEL (W("hours_label")), label);
      g_free (label);
    }
  else
    {
      /* Update the hours label */
      label = g_date_time_format (priv->date, "%I");
      gtk_label_set_text (GTK_LABEL (W("hours_label")), label);
      g_free (label);      
    }  

  /* Update the minutes label */
  label = g_date_time_format (priv->date, "%M");
  gtk_label_set_text (GTK_LABEL (W("minutes_label")), label);
  g_free (label);
}

static void
set_time_cb (GObject      *source,
             GAsyncResult *res,
             gpointer      user_data)
{
  CcDateTimePanel *self = user_data;
  GError *error;
  gboolean success;
  error = NULL;

  if (self->priv->use_systemd)
    {
      success = cc_dtm_call_set_time_finish (self->priv->dtm,
                                             res,
                                             &error);
    }
  else
    {
      success = cc_csddtm_call_set_time_finish (self->priv->csddtm,
                                                res,
                                                &error);
    }

  if (success)
    {
      update_time (self);
    }
  else
    {
      /* TODO: display any error in a user friendly way */
      g_warning ("Could not set system time: %s", error->message);
      g_error_free (error);
    }
}

static void
set_timezone_cb (GObject      *source,
                 GAsyncResult *res,
                 gpointer      user_data)
{
  CcDateTimePanel *self = user_data;
  GError *error;
  gboolean success;
  error = NULL;

  if (self->priv->use_systemd)
    {
      success = cc_dtm_call_set_timezone_finish (self->priv->dtm,
                                                 res,
                                                 &error);
    }
  else
    {
      success = cc_csddtm_call_set_timezone_finish (self->priv->csddtm,
                                                    res,
                                                    &error);
    }

  if (!success)
    {
      /* TODO: display any error in a user friendly way */
      g_warning ("Could not set system timezone: %s", error->message);
      g_error_free (error);
    }
}

static void
set_using_ntp_cb (GObject      *source,
                  GAsyncResult *res,
                  gpointer      user_data)
{
  CcDateTimePanel *self = user_data;
  GError *error;
  gboolean success;
  error = NULL;

  if (self->priv->use_systemd)
    {
      success = cc_dtm_call_set_ntp_finish (self->priv->dtm,
                                            res,
                                            &error);
    }
  else
    {
      success = cc_csddtm_call_set_using_ntp_finish (self->priv->csddtm,
                                                     res,
                                                     &error);
    }

    if (!success)
      {
        /* TODO: display any error in a user friendly way */
        g_warning ("Could not set system to use NTP: %s", error->message);
        g_error_free (error);
      }
}

static gboolean
set_date_time_cb (CcDateTimePanel *panel)
{
    gint64 unixtime;
    CcDateTimePanelPrivate *priv = panel->priv;

    unixtime = g_date_time_to_unix (priv->date);

    if (priv->use_systemd)
      {
        cc_dtm_call_set_time (priv->dtm,
                              unixtime * 1000000,
                              FALSE,
                              TRUE,
                              priv->cancellable,
                              set_time_cb,
                              panel);
      }
    else
      {
        cc_csddtm_call_set_time (priv->csddtm,
                                 unixtime,
                                 priv->cancellable,
                                 set_time_cb,
                                 panel);
      }

    if (priv->clock_blocked)
      {
        g_signal_handlers_unblock_by_func (priv->clock_tracker, on_clock_changed, panel);
        priv->clock_blocked = FALSE;
      }

    priv->set_date_time_delay_id = 0;
    return FALSE;
}

static void
queue_set_datetime (CcDateTimePanel *self)
{
  CcDateTimePanelPrivate *priv = self->priv;

  if (priv->set_date_time_delay_id > 0) {
    g_source_remove (priv->set_date_time_delay_id);
    priv->set_date_time_delay_id = 0;
  }

  if (!priv->clock_blocked) {
    g_signal_handlers_block_by_func (priv->clock_tracker, on_clock_changed, self);
    priv->clock_blocked = TRUE;
  }

  priv->set_date_time_delay_id = g_timeout_add (1000, (GSourceFunc)set_date_time_cb, self);
}

static void
queue_set_ntp (CcDateTimePanel *self)
{
  CcDateTimePanelPrivate *priv = self->priv;
  gboolean using_ntp;
  /* for now just do it */
  using_ntp = gtk_switch_get_active (GTK_SWITCH (W("network_time_switch")));

  if (self->priv->use_systemd)
    {
      cc_dtm_call_set_ntp (self->priv->dtm,
                           using_ntp,
                           TRUE,
                           self->priv->cancellable,
                           set_using_ntp_cb,
                           self);
    }
  else
    {
      cc_csddtm_call_set_using_ntp (self->priv->csddtm,
                                    using_ntp,
                                    self->priv->cancellable,
                                    set_using_ntp_cb,
                                    self);
    }
}

static void
queue_set_timezone (CcDateTimePanel *self)
{
  /* for now just do it */
  if (self->priv->current_location)
    {
      if (self->priv->use_systemd)
        {
          cc_dtm_call_set_timezone (self->priv->dtm,
                                    self->priv->current_location->zone,
                                    TRUE,
                                    self->priv->cancellable,
                                    set_timezone_cb,
                                    self);
        }
      else
        {
          cc_csddtm_call_set_timezone (self->priv->csddtm,
                                       self->priv->current_location->zone,
                                       self->priv->cancellable,
                                       set_timezone_cb,
                                       self);
        }
    }
}

static void
change_date (CcDateTimePanel *self)
{
  CcDateTimePanelPrivate *priv = self->priv;
  guint mon, y, d;
  GDateTime *old_date;

  old_date = priv->date;

  mon = 1 + gtk_combo_box_get_active (GTK_COMBO_BOX (W ("month-combobox")));
  y = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (W ("year-spinbutton")));
  d = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (W ("day-spinbutton")));

  priv->date = g_date_time_new_local (y, mon, d,
                                      g_date_time_get_hour (old_date),
                                      g_date_time_get_minute (old_date),
                                      g_date_time_get_second (old_date));
  g_date_time_unref (old_date);
  queue_set_datetime (self);
}

static void
region_changed_cb (GtkComboBox     *box,
                   CcDateTimePanel *self)
{
  GtkTreeModelFilter *modelfilter;

  modelfilter = GTK_TREE_MODEL_FILTER (gtk_builder_get_object (self->priv->builder, "city-modelfilter"));

  gtk_tree_model_filter_refilter (modelfilter);
}

static void
city_changed_cb (GtkComboBox     *box,
                 CcDateTimePanel *self)
{
  static gboolean inside = FALSE;
  GtkTreeIter iter;
  gchar *zone;

  /* prevent re-entry from location changed callback */
  if (inside)
    return;

  inside = TRUE;

  if (gtk_combo_box_get_active_iter (box, &iter))
    {
      gtk_tree_model_get (gtk_combo_box_get_model (box), &iter,
                          CITY_COL_ZONE, &zone, -1);

      cc_timezone_map_set_timezone (CC_TIMEZONE_MAP (self->priv->map), zone);

      g_free (zone);
    }

  inside = FALSE;
}

static void
update_timezone (CcDateTimePanel *self)
{
  CcDateTimePanelPrivate *priv = self->priv;
  GtkWidget *widget;
  gchar **split;
  GtkTreeIter iter;
  GtkTreeModel *model;

  /* tz.c updates the local timezone, which means the spin buttons can be
   * updated with the current time of the new location */

  split = g_strsplit (priv->current_location->zone, "/", 2);

  /* remove underscores */
  g_strdelimit (split[1], "_", ' ');

  /* update region combo */
  widget = (GtkWidget *) gtk_builder_get_object (priv->builder,
                                                 "region_combobox");
  model = gtk_combo_box_get_model (GTK_COMBO_BOX (widget));
  gtk_tree_model_get_iter_first (model, &iter);

  do
    {
      gchar *string;

      gtk_tree_model_get (model, &iter, CITY_COL_CITY, &string, -1);

      if (!g_strcmp0 (string, split[0]))
        {
          g_free (string);
          gtk_combo_box_set_active_iter (GTK_COMBO_BOX (widget), &iter);
          break;
        }
      g_free (string);
    }
  while (gtk_tree_model_iter_next (model, &iter));


  /* update city combo */
  widget = (GtkWidget *) gtk_builder_get_object (priv->builder,
                                                 "city_combobox");
  model = gtk_combo_box_get_model (GTK_COMBO_BOX (widget));
  gtk_tree_model_filter_refilter ((GtkTreeModelFilter *) gtk_builder_get_object (priv->builder, "city-modelfilter"));
  gtk_tree_model_get_iter_first (model, &iter);

  do
    {
      gchar *string;

      gtk_tree_model_get (model, &iter, CITY_COL_CITY, &string, -1);

      if (!g_strcmp0 (string, split[1]))
        {
          g_free (string);
          gtk_combo_box_set_active_iter (GTK_COMBO_BOX (widget), &iter);
          break;
        }
      g_free (string);
    }
  while (gtk_tree_model_iter_next (model, &iter));

  g_strfreev (split);
}

static void
location_changed_cb (CcTimezoneMap   *map,
                     TzLocation      *location,
                     CcDateTimePanel *self)
{
  CcDateTimePanelPrivate *priv = self->priv;
  GtkWidget *region_combo, *city_combo;

  g_debug ("location changed to %s/%s", location->country, location->zone);

  self->priv->current_location = location;

  /* Update the combo boxes */
  region_combo = W("region_combobox");
  city_combo = W("city_combobox");

  g_signal_handlers_block_by_func (region_combo, region_changed_cb, self);
  g_signal_handlers_block_by_func (city_combo, city_changed_cb, self);

  update_timezone (self);

  g_signal_handlers_unblock_by_func (region_combo, region_changed_cb, self);
  g_signal_handlers_unblock_by_func (city_combo, city_changed_cb, self);

  queue_set_timezone (self);
}

static void
get_timezone (CcDateTimePanel *self)
{
  GtkWidget *widget;
  gchar *timezone;
  GError *error;

  error = NULL;

  if (self->priv->use_systemd) {
    timezone = cc_dtm_dup_timezone (self->priv->dtm);
  } else {
    cc_csddtm_call_get_timezone_sync (self->priv->csddtm,
                                      &timezone,
                                      NULL,
                                      &error);
  }

  if (error != NULL || timezone == NULL || !cc_timezone_map_set_timezone (CC_TIMEZONE_MAP (self->priv->map), timezone))
    {
      if (error)
        {
          g_warning ("Problem getting the current timezone: %s", error->message);
          g_error_free (error);
        }

      g_warning ("Timezone '%s' is unhandled, setting %s as default", timezone, DEFAULT_TZ);
      cc_timezone_map_set_timezone (CC_TIMEZONE_MAP (self->priv->map), DEFAULT_TZ);
    }

  self->priv->current_location = cc_timezone_map_get_location (CC_TIMEZONE_MAP (self->priv->map));
  update_timezone (self);

  /* now that the initial state is loaded set connect the signals */
  widget = (GtkWidget*) gtk_builder_get_object (self->priv->builder,
                                                "region_combobox");
  g_signal_connect (widget, "changed", G_CALLBACK (region_changed_cb), self);

  widget = (GtkWidget*) gtk_builder_get_object (self->priv->builder,
                                                "city_combobox");
  g_signal_connect (widget, "changed", G_CALLBACK (city_changed_cb), self);

  g_signal_connect (self->priv->map, "location-changed",
                    G_CALLBACK (location_changed_cb), self);

  g_free (timezone);
}

/* load region and city tree models */
struct get_region_data
{
  GtkListStore *region_store;
  GtkListStore *city_store;
  GHashTable *table;
};

/* Slash look-alikes that might be used in translations */
#define TRANSLATION_SPLIT                                                        \
        "\342\201\204"        /* FRACTION SLASH */                               \
        "\342\210\225"        /* DIVISION SLASH */                               \
        "\342\247\270"        /* BIG SOLIDUS */                                  \
        "\357\274\217"        /* FULLWIDTH SOLIDUS */                            \
        "/"

static void
get_regions (TzLocation             *loc,
             struct get_region_data *data)
{
  gchar *zone;
  gchar **split;
  gchar **split_translated;
  gchar *translated_city;

  zone = g_strdup (loc->zone);
  g_strdelimit (zone, "_", ' ');
  split = g_strsplit (zone, "/", 2);
  g_free (zone);

  /* Load the translation for it */
  zone = g_strdup (dgettext (GETTEXT_PACKAGE_TIMEZONES, loc->zone));
  g_strdelimit (zone, "_", ' ');
  split_translated = g_regex_split_simple ("[\\x{2044}\\x{2215}\\x{29f8}\\x{ff0f}/]", zone, 0, 0);
  g_free (zone);

  if (!g_hash_table_lookup_extended (data->table, split[0], NULL, NULL))
    {
      g_hash_table_insert (data->table, g_strdup (split[0]),
                           GINT_TO_POINTER (1));
      gtk_list_store_insert_with_values (data->region_store, NULL, 0,
                                         REGION_COL_REGION, split[0],
                                         REGION_COL_REGION_TRANSLATED, split_translated[0], -1);
    }

  /* g_regex_split_simple() splits too much for us, and would break
   * America/Argentina/Buenos_Aires into 3 strings, so rejoin the city part */
  translated_city = g_strjoinv ("/", split_translated + 1);

  gtk_list_store_insert_with_values (data->city_store, NULL, 0,
                                     CITY_COL_CITY, split[1],
                                     CITY_COL_CITY_TRANSLATED, translated_city,
                                     CITY_COL_REGION, split[0],
                                     CITY_COL_REGION_TRANSLATED, split_translated[0],
                                     CITY_COL_ZONE, loc->zone,
                                     -1);

  g_free (translated_city);
  g_strfreev (split);
  g_strfreev (split_translated);
}

static gboolean
city_model_filter_func (GtkTreeModel *model,
                        GtkTreeIter  *iter,
                        GtkComboBox  *combo)
{
  GtkTreeModel *combo_model;
  GtkTreeIter combo_iter;
  gchar *active_region = NULL;
  gchar *city_region = NULL;
  gboolean result;

  if (gtk_combo_box_get_active_iter (combo, &combo_iter) == FALSE)
    return FALSE;

  combo_model = gtk_combo_box_get_model (combo);
  gtk_tree_model_get (combo_model, &combo_iter,
                      CITY_COL_CITY, &active_region, -1);

  gtk_tree_model_get (model, iter,
                      CITY_COL_REGION, &city_region, -1);

  if (g_strcmp0 (active_region, city_region) == 0)
    result = TRUE;
  else
    result = FALSE;

  g_free (city_region);

  g_free (active_region);

  return result;
}


static void
load_regions_model (GtkListStore *regions, GtkListStore *cities)
{
  struct get_region_data data;
  TzDB *db;
  GHashTable *table;


  db = tz_load_db ();
  table = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

  data.table = table;
  data.region_store = regions;
  data.city_store = cities;

  g_ptr_array_foreach (db->locations, (GFunc) get_regions, &data);

  g_hash_table_destroy (table);

  tz_db_free (db);

  /* sort the models */
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (regions),
                                        REGION_COL_REGION_TRANSLATED,
                                        GTK_SORT_ASCENDING);
}

static void
update_widget_state_for_ntp (CcDateTimePanel *panel,
                             gboolean         using_ntp)
{
  CcDateTimePanelPrivate *priv = panel->priv;

  gtk_widget_set_sensitive (W("table1"), !using_ntp);
  gtk_widget_set_sensitive (W("table2"), !using_ntp);
}

static void
day_changed (GtkWidget       *widget,
             CcDateTimePanel *panel)
{
  change_date (panel);
}

static void
month_year_changed (GtkWidget       *widget,
                    CcDateTimePanel *panel)
{
  CcDateTimePanelPrivate *priv = panel->priv;
  guint mon, y;
  guint num_days;
  GtkAdjustment *adj;
  GtkSpinButton *day_spin;

  mon = 1 + gtk_combo_box_get_active (GTK_COMBO_BOX (W ("month-combobox")));
  y = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (W ("year-spinbutton")));

  /* Check the number of days in that month */
  num_days = g_date_get_days_in_month (mon, y);

  day_spin = GTK_SPIN_BUTTON (W("day-spinbutton"));
  adj = GTK_ADJUSTMENT (gtk_spin_button_get_adjustment (day_spin));
  gtk_adjustment_set_upper (adj, num_days + 1);

  if (gtk_spin_button_get_value_as_int (day_spin) > num_days)
    gtk_spin_button_set_value (day_spin, num_days);

  change_date (panel);
}

static void
on_clock_changed (GnomeWallClock  *clock,
		  GParamSpec      *pspec,
		  CcDateTimePanel *panel)
{
  CcDateTimePanelPrivate *priv = panel->priv;

  g_date_time_unref (priv->date);
  priv->date = g_date_time_new_now_local ();
  update_time (panel);
}

static void
change_time (GtkButton       *button,
             CcDateTimePanel *panel)
{
  CcDateTimePanelPrivate *priv = panel->priv;
  const gchar *widget_name;
  gint direction;
  GDateTime *old_date;

  old_date = priv->date;

  widget_name = gtk_buildable_get_name (GTK_BUILDABLE (button));

  if (strstr (widget_name, "up"))
    direction = 1;
  else
    direction = -1;

  if (widget_name[0] == 'h')
    {
      priv->date = g_date_time_add_hours (old_date, direction);
    }
  else if (widget_name[0] == 'm')
    {
      priv->date = g_date_time_add_minutes (old_date, direction);
    }
  else
    {
      int hour;
      hour = g_date_time_get_hour (old_date);
      if (hour >= 12)
        priv->date = g_date_time_add_hours (old_date, -12);
      else
        priv->date = g_date_time_add_hours (old_date, 12);
    }
  g_date_time_unref (old_date);

  update_time (panel);
  queue_set_datetime (panel);
}

static void
change_ntp (GObject         *gobject,
            GParamSpec      *pspec,
            CcDateTimePanel *self)
{
  update_widget_state_for_ntp (self, gtk_switch_get_active (GTK_SWITCH (gobject)));
  queue_set_ntp (self);
}

static void
on_permission_changed (GPermission *permission,
                       GParamSpec  *pspec,
                       gpointer     data)
{
  CcDateTimePanelPrivate *priv = CC_DATE_TIME_PANEL (data)->priv;
  gboolean allowed, using_ntp;

  allowed = g_permission_get_allowed (permission);
  using_ntp = gtk_switch_get_active (GTK_SWITCH (W("network_time_switch")));

  /* All the widgets but the lock button and the 24h setting */
  gtk_widget_set_sensitive (W("map-vbox"), allowed);
  gtk_widget_set_sensitive (W("hbox2"), allowed);
  gtk_widget_set_sensitive (W("alignment2"), allowed);
  gtk_widget_set_sensitive (W("table1"), allowed && !using_ntp);
}

static void
reorder_date_widget (DateEndianess           endianess,
		     CcDateTimePanelPrivate *priv)
{
  GtkWidget *month, *day, *year;
  GtkBox *box;

  if (endianess == DATE_ENDIANESS_MIDDLE)
    return;

  month = W ("month-combobox");
  day = W ("day-spinbutton");
  year = W("year-spinbutton");

  box = GTK_BOX (W("table1"));

  switch (endianess) {
  case DATE_ENDIANESS_LITTLE:
    gtk_box_reorder_child (box, month, 0);
    gtk_box_reorder_child (box, day, 0);
    gtk_box_reorder_child (box, year, -1);
    break;
  case DATE_ENDIANESS_BIG:
    gtk_box_reorder_child (box, month, 0);
    gtk_box_reorder_child (box, year, 0);
    gtk_box_reorder_child (box, day, -1);
    break;
  case DATE_ENDIANESS_MIDDLE:
    /* Let's please GCC */
    g_assert_not_reached ();
    break;
  }
}

static void
cc_date_time_panel_init (CcDateTimePanel *self)
{
  CcDateTimePanelPrivate *priv;
  gchar *objects[] = { "datetime-panel", "region-liststore", "city-liststore",
      "month-liststore", "city-modelfilter", "city-modelsort", NULL };
  char *buttons[] = { "hour_up_button", "hour_down_button", "min_up_button",
          "min_down_button"};
  GtkWidget *widget;
  GtkAdjustment *adjustment;
  GError *err = NULL;
  GtkTreeModelFilter *city_modelfilter;
  GtkTreeModelSort *city_modelsort;
  guint i, num_days;
  gboolean using_ntp;
  gboolean can_use_ntp;
  int ret;
  DateEndianess endianess;
  GError *error;
  gboolean have_ntpd;

  priv = self->priv = DATE_TIME_PANEL_PRIVATE (self);

  priv->set_date_time_delay_id = 0;
  priv->clock_blocked = FALSE;

  priv->cancellable = g_cancellable_new ();
  error = NULL;

  have_ntpd = g_file_test ("/usr/sbin/ntpd", G_FILE_TEST_EXISTS);

  if (!have_ntpd)
    {
      priv->dtm = cc_dtm_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
                                                 G_DBUS_PROXY_FLAGS_NONE,
                                                 "org.freedesktop.timedate1",
                                                 "/org/freedesktop/timedate1",
                                                 priv->cancellable,
                                                 &error);
      priv->use_systemd = TRUE;
    }

  if (have_ntpd || priv->dtm == NULL)
    {
      if (error)
        {
          g_warning ("could not get proxy for systemd timedate1 service: %s", error->message);
          g_clear_error (&error);

          g_warning ("trying to use csd DateTimeMechanism instead, though network time may be unavailable");
        }

      priv->csddtm = cc_csddtm_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
                                                       G_DBUS_PROXY_FLAGS_NONE,
                                                       "org.cinnamon.SettingsDaemon.DateTimeMechanism",
                                                       "/",
                                                       priv->cancellable,
                                                       &error);

      if (priv->csddtm == NULL)
        {
          g_warning ("could not get proxy for csd DateTimeMechanism: %s", error->message);
          g_error_free (error);
        }

      priv->use_systemd = FALSE;
    }

  priv->builder = gtk_builder_new ();
  ret = gtk_builder_add_objects_from_file (priv->builder, DATADIR"/datetime.ui",
                                           objects, &err);

  if (ret == 0)
    {
      g_warning ("Could not load ui: %s", err ? err->message : "No reason");
      if (err)
        g_error_free (err);
      return;
    }

  error = NULL;
  using_ntp = can_use_ntp = FALSE;

  if (priv->use_systemd)
    {
      using_ntp = cc_dtm_get_ntp (priv->dtm);
    }
  else
    {
      if (!cc_csddtm_call_get_using_ntp_sync (priv->csddtm,
                                              &can_use_ntp,
                                              &using_ntp,
                                              priv->cancellable,
                                              &error))
        {
          g_warning ("Failed to get using ntp: %s", error->message);
          g_error_free (error);
        }
    }

  gtk_switch_set_active (GTK_SWITCH (W("network_time_switch")), using_ntp);
  update_widget_state_for_ntp (self, using_ntp);
  g_signal_connect (W("network_time_switch"), "notify::active",
                    G_CALLBACK (change_ntp), self);

  /* set up time editing widgets */
  for (i = 0; i < G_N_ELEMENTS (buttons); i++)
    {
      g_signal_connect (W(buttons[i]), "clicked",
                        G_CALLBACK (change_time), self);
    }

  /* set up date editing widgets */
  priv->date = g_date_time_new_now_local ();
  endianess = date_endian_get_default (FALSE);
  reorder_date_widget (endianess, priv);

  /* Force the direction for the time, so that the time
   * is presented correctly for RTL languages */
  gtk_widget_set_direction (W("table2"), GTK_TEXT_DIR_LTR);

  gtk_combo_box_set_active (GTK_COMBO_BOX (W ("month-combobox")),
                            g_date_time_get_month (priv->date) - 1);
  g_signal_connect (G_OBJECT (W("month-combobox")), "changed",
                    G_CALLBACK (month_year_changed), self);

  num_days = g_date_get_days_in_month (g_date_time_get_month (priv->date),
                                       g_date_time_get_year (priv->date));
  adjustment = (GtkAdjustment*) gtk_adjustment_new (g_date_time_get_day_of_month (priv->date), 1,
                                                    num_days + 1, 1, 10, 1);
  gtk_spin_button_set_adjustment (GTK_SPIN_BUTTON (W ("day-spinbutton")),
                                  adjustment);
  g_signal_connect (G_OBJECT (W("day-spinbutton")), "value-changed",
                    G_CALLBACK (day_changed), self);

  adjustment = (GtkAdjustment*) gtk_adjustment_new (g_date_time_get_year (priv->date),
                                                    G_MINDOUBLE, G_MAXDOUBLE, 1,
                                                    10, 1);
  gtk_spin_button_set_adjustment (GTK_SPIN_BUTTON (W ("year-spinbutton")),
                                  adjustment);
  g_signal_connect (G_OBJECT (W("year-spinbutton")), "value-changed",
                    G_CALLBACK (month_year_changed), self);

  /* set up timezone map */
  priv->map = widget = (GtkWidget *) cc_timezone_map_new ();
  gtk_widget_show (widget);

  gtk_container_add (GTK_CONTAINER (gtk_builder_get_object (priv->builder,
                                                            "aspectmap")),
                     widget);

  gtk_container_add (GTK_CONTAINER (self),
                     GTK_WIDGET (gtk_builder_get_object (priv->builder,
                                                         "datetime-panel")));


  /* setup the time itself */
  priv->clock_tracker = g_object_new (GNOME_TYPE_WALL_CLOCK, NULL);
  g_signal_connect (priv->clock_tracker, "notify::clock", G_CALLBACK (on_clock_changed), self);

  priv->settings = g_settings_new (CLOCK_SCHEMA);

  g_signal_connect (priv->settings, "changed::" CLOCK_USE_24H,
                    G_CALLBACK (clock_settings_changed_cb), self);
  clock_settings_changed_cb (priv->settings, CLOCK_USE_24H, self);

  priv->locations = (GtkTreeModel*) gtk_builder_get_object (priv->builder,
                                                            "region-liststore");

  load_regions_model (GTK_LIST_STORE (priv->locations),
                      GTK_LIST_STORE (gtk_builder_get_object (priv->builder,
                                                              "city-liststore")));

  city_modelfilter = GTK_TREE_MODEL_FILTER (gtk_builder_get_object (priv->builder, "city-modelfilter"));

  widget = (GtkWidget*) gtk_builder_get_object (priv->builder,
                                                "region_combobox");
  city_modelsort = GTK_TREE_MODEL_SORT (gtk_builder_get_object (priv->builder, "city-modelsort"));
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (city_modelsort), CITY_COL_CITY_TRANSLATED,
                                        GTK_SORT_ASCENDING);

  gtk_tree_model_filter_set_visible_func (city_modelfilter,
                                          (GtkTreeModelFilterVisibleFunc) city_model_filter_func,
                                          widget,
                                          NULL);

  /* After the initial setup, so we can be sure that
   * the model is filled up */
  get_timezone (self);

  /* add the lock button */
  if (priv->use_systemd)
    {
      priv->permission = polkit_permission_new_sync ("org.cinnamon.controlcenter.datetime.configure",
                                                     NULL, NULL, NULL);
    }
  else
    {
      priv->permission = polkit_permission_new_sync ("org.cinnamon.settingsdaemon.datetimemechanism.configure",
                                                     NULL, NULL, NULL);
    }

  if (priv->permission == NULL)
    {
      g_warning ("Your system does not have the '%s' PolicyKit files installed. Please check your installation",
                 "org.cinnamon.settingsdaemon.datetimemechanism.configure");
      return;
    }

  if (priv->dtm == NULL && priv->csddtm == NULL)
    {
      g_warning ("No mechanism available for setting time or NTP state.  Either install ntpd/ntpdate (if using"
                 "consolekit) or make sure the systemd-timedated1 and systemd-timesync services are enabled.");
      return;
    }

  priv->lock_button = widget = (GtkWidget *) W("lock_button");
  gtk_lock_button_set_permission ( GTK_LOCK_BUTTON (priv->lock_button), priv->permission );
  gtk_widget_show (widget);

  g_signal_connect (priv->permission, "notify", G_CALLBACK (on_permission_changed), self);
  on_permission_changed (priv->permission, NULL, self);
}

void
cc_date_time_panel_register (GIOModule *module)
{
  textdomain (GETTEXT_PACKAGE);
  bindtextdomain (GETTEXT_PACKAGE, "/usr/share/locale");
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

  cc_date_time_panel_register_type (G_TYPE_MODULE (module));
  g_io_extension_point_implement (CC_SHELL_PANEL_EXTENSION_POINT,
                                  CC_TYPE_DATE_TIME_PANEL,
                                  "datetime", 0);
}

