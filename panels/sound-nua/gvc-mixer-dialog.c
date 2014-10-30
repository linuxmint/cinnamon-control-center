/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 William Jon McCann
 * Copyright (C) 2012 Conor Curran
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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <pulse/pulseaudio.h>

#include <canberra.h>

#include "gvc-channel-bar.h"
#include "gvc-balance-bar.h"
#include "gvc-combo-box.h"
#include "gvc-mixer-control.h"
#include "gvc-mixer-card.h"
#include "gvc-mixer-ui-device.h"
#include "gvc-mixer-sink.h"
#include "gvc-mixer-source.h"
#include "gvc-mixer-source-output.h"
#include "gvc-mixer-dialog.h"
#include "gvc-level-bar.h"
#include "gvc-speaker-test.h"
#include "gvc-mixer-control-private.h"

#define SCALE_SIZE 128

#define GVC_MIXER_DIALOG_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GVC_TYPE_MIXER_DIALOG, GvcMixerDialogPrivate))

struct GvcMixerDialogPrivate
{
        GvcMixerControl *mixer_control;
        GHashTable      *bars;        
        GtkWidget       *notebook;
        GtkWidget       *output_bar;
        GtkWidget       *input_bar;
        GtkWidget       *input_level_bar;
        GtkWidget       *effects_bar;
        GtkWidget       *output_stream_box;
        GtkWidget       *sound_effects_box;
        GtkWidget       *hw_box;
        GtkWidget       *hw_treeview;
        GtkWidget       *hw_settings_box;
        GtkWidget       *hw_profile_combo;
        GtkWidget       *input_box;
        GtkWidget       *output_box;
        GtkWidget       *applications_box;
        GtkWidget       *no_apps_label;
        GtkWidget       *output_treeview;
        GtkWidget       *output_settings_box;
        GtkWidget       *output_balance_bar;
        GtkWidget       *output_fade_bar;
        GtkWidget       *output_lfe_bar;
        GtkWidget       *output_profile_combo;
        GtkWidget       *input_profile_combo;
        GtkWidget       *input_treeview;
        GtkWidget       *input_settings_box;
        GtkWidget       *click_feedback_button;
        GtkWidget       *audible_bell_button;
        GtkSizeGroup    *size_group;
        GtkWidget       *selected_output_label;
        GtkWidget       *selected_input_label;
        GtkWidget       *test_output_button;

        GSettings       *sound_settings;

        gdouble          last_input_peak;
        guint            num_apps;

        ca_context      *sound_context;
};

enum {
        NAME_COLUMN,
        DEVICE_COLUMN,
        ACTIVE_COLUMN,
        ID_COLUMN,
        SPEAKERS_COLUMN,
        ICON_COLUMN,
        NUM_COLUMNS
};

enum {
        HW_ID_COLUMN,
        HW_ICON_COLUMN,
        HW_NAME_COLUMN,
        HW_STATUS_COLUMN,
        HW_PROFILE_COLUMN,
        HW_PROFILE_HUMAN_COLUMN,
        HW_SENSITIVE_COLUMN,
        HW_NUM_COLUMNS
};

enum
{
        PROP_0,
        PROP_MIXER_CONTROL
};

static void     gvc_mixer_dialog_class_init (GvcMixerDialogClass *klass);
static void     gvc_mixer_dialog_init       (GvcMixerDialog      *mixer_dialog);
static void     gvc_mixer_dialog_finalize   (GObject             *object);

static void     bar_set_stream              (GvcMixerDialog      *dialog,
                                             GtkWidget           *bar,
                                             GvcMixerStream      *stream);

static void     on_adjustment_value_changed (GtkAdjustment  *adjustment,
                                             GvcMixerDialog *dialog);
static void  	on_control_output_added (GvcMixerControl *control,
                                       guint            id,
                                       GvcMixerDialog  *dialog);
static void   on_control_active_output_update (GvcMixerControl *control,
                                               guint            id,
                                               GvcMixerDialog  *dialog);
                                       
static void   on_control_active_input_update (GvcMixerControl *control,
                                              guint            id,
                                              GvcMixerDialog  *dialog);

G_DEFINE_TYPE (GvcMixerDialog, gvc_mixer_dialog, GTK_TYPE_VBOX)


static void
update_description (GvcMixerDialog *dialog,
                    guint column,
                    const char *value,
                    GvcMixerStream *stream)
{
        GtkTreeModel *model;
        GtkTreeIter   iter;
        guint         id;

        if (GVC_IS_MIXER_SOURCE (stream))
                model = gtk_tree_view_get_model (GTK_TREE_VIEW (dialog->priv->input_treeview));
        else if (GVC_IS_MIXER_SINK (stream))
                model = gtk_tree_view_get_model (GTK_TREE_VIEW (dialog->priv->output_treeview));
        else
                g_assert_not_reached ();

        if (gtk_tree_model_get_iter_first (model, &iter) == FALSE){
                g_warning ("The tree is empty => Cannot update the description");
                return;        
        }

        id = gvc_mixer_stream_get_id (stream);
        do {
                guint       current_id;

                gtk_tree_model_get (model, &iter,
                                    ID_COLUMN, &current_id,
                                    -1);
                if (id != current_id)
                        continue;

                gtk_list_store_set (GTK_LIST_STORE (model),
                                    &iter,
                                    column, value,
                                    -1);
                break;
        } while (gtk_tree_model_iter_next (model, &iter));
}

static void
profile_selection_changed (GvcComboBox *combo_box,
                           const char  *profile,
                           GvcMixerDialog *dialog)
{	
        g_debug ("profile_selection_changed - %s", profile);
        GvcMixerUIDevice *out;
        out = g_object_get_data (G_OBJECT (combo_box), "uidevice");
        
        if (out == NULL) {
                g_warning ("Could not find Output for profile combo box");
                return;
        }

        g_debug (" \n on profile selection changed on output with \n description %s \n origin %s \n id %i \n \n",
                gvc_mixer_ui_device_get_description (out),
                gvc_mixer_ui_device_get_origin (out),
                gvc_mixer_ui_device_get_id (out));

        if (gvc_mixer_control_change_profile_on_selected_device (dialog->priv->mixer_control, out, profile) == FALSE) {
                g_warning ("Could not change profile on device %s",
                           gvc_mixer_ui_device_get_description (out));
        }
}

#define DECAY_STEP .15

static void
update_input_peak (GvcMixerDialog *dialog,
                   gdouble         v)
{
        GtkAdjustment *adj;

        if (dialog->priv->last_input_peak >= DECAY_STEP) {
                if (v < dialog->priv->last_input_peak - DECAY_STEP) {
                        v = dialog->priv->last_input_peak - DECAY_STEP;
                }
        }

        dialog->priv->last_input_peak = v;

        adj = gvc_level_bar_get_peak_adjustment (GVC_LEVEL_BAR (dialog->priv->input_level_bar));
        if (v >= 0) {
                gtk_adjustment_set_value (adj, v);
        } else {
                gtk_adjustment_set_value (adj, 0.0);
        }
}

static void
update_input_meter (GvcMixerDialog *dialog,
                    uint32_t        source_index,
                    uint32_t        sink_input_idx,
                    double          v)
{
        update_input_peak (dialog, v);
}

static void
on_monitor_suspended_callback (pa_stream *s,
                               void      *userdata)
{
        GvcMixerDialog *dialog;

        dialog = userdata;

        if (pa_stream_is_suspended (s)) {
                g_debug ("Stream suspended");
                update_input_meter (dialog,
                                    pa_stream_get_device_index (s),
                                    PA_INVALID_INDEX,
                                    -1);
        }
}

static void
on_monitor_read_callback (pa_stream *s,
                          size_t     length,
                          void      *userdata)
{
        GvcMixerDialog *dialog;
        const void     *data;
        double          v;

        dialog = userdata;

        if (pa_stream_peek (s, &data, &length) < 0) {
                g_warning ("Failed to read data from stream");
                return;
        }

        assert (length > 0);
        assert (length % sizeof (float) == 0);

        v = ((const float *) data)[length / sizeof (float) -1];

        pa_stream_drop (s);

        if (v < 0) {
                v = 0;
        }
        if (v > 1) {
                v = 1;
        }

        update_input_meter (dialog,
                            pa_stream_get_device_index (s),
                            pa_stream_get_monitor_stream (s),
                            v);
}

static void
create_monitor_stream_for_source (GvcMixerDialog *dialog,
                                  GvcMixerStream *stream)
{
        pa_stream     *s;
        char           t[16];
        pa_buffer_attr attr;
        pa_sample_spec ss;
        pa_context    *context;
        int            res;
        pa_proplist   *proplist;
        gboolean       has_monitor;

        if (stream == NULL) {
                g_debug ("\n create_monitor_stream_for_source - stream is null - returning\n");
                return;
        }
        has_monitor = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (stream), "has-monitor"));
        if (has_monitor != FALSE) {
                g_debug ("\n create_monitor_stream_for_source, has monitor is not false - returning \n");
                return;
        }

        g_debug ("Create monitor for %u",
                 gvc_mixer_stream_get_index (stream));

        context = gvc_mixer_control_get_pa_context (dialog->priv->mixer_control);

        if (pa_context_get_server_protocol_version (context) < 13) {
                g_debug ("\n create_monitor_stream_for_source - protocol version is less 13 \n");
                return;
        }

        ss.channels = 1;
        ss.format = PA_SAMPLE_FLOAT32;
        ss.rate = 25;

        memset (&attr, 0, sizeof (attr));
        attr.fragsize = sizeof (float);
        attr.maxlength = (uint32_t) -1;

        snprintf (t, sizeof (t), "%u", gvc_mixer_stream_get_index (stream));

        proplist = pa_proplist_new ();
        pa_proplist_sets (proplist, PA_PROP_APPLICATION_ID, "org.gnome.VolumeControl");
        s = pa_stream_new_with_proplist (context, _("Peak detect"), &ss, NULL, proplist);
        pa_proplist_free (proplist);
        if (s == NULL) {
                g_warning ("Failed to create monitoring stream");
                return;
        }

        pa_stream_set_read_callback (s, on_monitor_read_callback, dialog);
        pa_stream_set_suspended_callback (s, on_monitor_suspended_callback, dialog);

        res = pa_stream_connect_record (s,
                                        t,
                                        &attr,
                                        (pa_stream_flags_t) (PA_STREAM_DONT_MOVE
                                                             |PA_STREAM_PEAK_DETECT
                                                             |PA_STREAM_ADJUST_LATENCY));
        if (res < 0) {
                g_warning ("Failed to connect monitoring stream");
                pa_stream_unref (s);
        } else {
                g_object_set_data (G_OBJECT (stream), "has-monitor", GINT_TO_POINTER (TRUE));
                g_object_set_data (G_OBJECT (dialog->priv->input_level_bar), "pa_stream", s);
                g_object_set_data (G_OBJECT (dialog->priv->input_level_bar), "stream", stream);
        }
}


static void
stop_monitor_stream_for_source (GvcMixerDialog *dialog)
{
        pa_stream      *s;
        pa_context     *context;
        int             res = 0;
        GvcMixerStream *stream;

        stream = g_object_get_data (G_OBJECT (dialog->priv->input_level_bar), "stream");

        if (stream == NULL){
                g_debug ("\n stop_monitor_stream_for_source - gvcstream is null - returning \n");                
                return;
        }
        else{
                g_debug ("\n stop_monitor_stream_for_source - gvcstream is not null - continue \n");                                
        }

        s = g_object_get_data (G_OBJECT (dialog->priv->input_level_bar), "pa_stream");

        if (s != NULL){
                res = pa_stream_disconnect (s);
                if (res == 0) {
                        g_debug("stream has been disconnected");
                        pa_stream_unref (s);
                }
                g_object_set_data (G_OBJECT (dialog->priv->input_level_bar), "pa_stream", NULL);        
        }
                
        context = gvc_mixer_control_get_pa_context (dialog->priv->mixer_control);

        if (pa_context_get_server_protocol_version (context) < 13) {
                return;
        }
        if (res == 0) {
                g_object_set_data (G_OBJECT (stream), "has-monitor", GINT_TO_POINTER (FALSE));
        }
        g_debug ("Stopping monitor for %u", pa_stream_get_index (s));
        g_object_set_data (G_OBJECT (dialog->priv->input_level_bar), "stream", NULL);
}

static void
gvc_mixer_dialog_set_mixer_control (GvcMixerDialog  *dialog,
                                    GvcMixerControl *control)
{
        g_return_if_fail (GVC_MIXER_DIALOG (dialog));
        g_return_if_fail (GVC_IS_MIXER_CONTROL (control));

        g_object_ref (control);

        if (dialog->priv->mixer_control != NULL) {
                g_signal_handlers_disconnect_by_func (dialog->priv->mixer_control,
                                                      G_CALLBACK (on_control_active_output_update),
                                                      dialog);
                g_signal_handlers_disconnect_by_func (dialog->priv->mixer_control,
                                                      G_CALLBACK (on_control_active_input_update),
                                                      dialog);
                g_object_unref (dialog->priv->mixer_control);
        }

        dialog->priv->mixer_control = control;

        g_signal_connect (dialog->priv->mixer_control,
                          "active-output-update",
                          G_CALLBACK (on_control_active_output_update),
                          dialog);
        g_signal_connect (dialog->priv->mixer_control,
                          "active-input-update",
                          G_CALLBACK (on_control_active_input_update),
                          dialog);

        g_object_notify (G_OBJECT (dialog), "mixer-control");
}

static GvcMixerControl *
gvc_mixer_dialog_get_mixer_control (GvcMixerDialog *dialog)
{
        g_return_val_if_fail (GVC_IS_MIXER_DIALOG (dialog), NULL);

        return dialog->priv->mixer_control;
}

static void
gvc_mixer_dialog_set_property (GObject       *object,
                               guint          prop_id,
                               const GValue  *value,
                               GParamSpec    *pspec)
{
        GvcMixerDialog *self = GVC_MIXER_DIALOG (object);

        switch (prop_id) {
        case PROP_MIXER_CONTROL:
                gvc_mixer_dialog_set_mixer_control (self, g_value_get_object (value));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
gvc_mixer_dialog_get_property (GObject     *object,
                               guint        prop_id,
                               GValue      *value,
                               GParamSpec  *pspec)
{
        GvcMixerDialog *self = GVC_MIXER_DIALOG (object);

        switch (prop_id) {
        case PROP_MIXER_CONTROL:
                g_value_set_object (value, gvc_mixer_dialog_get_mixer_control (self));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
on_adjustment_value_changed (GtkAdjustment  *adjustment,
                             GvcMixerDialog *dialog)
{
        GvcMixerStream *stream;

        stream = g_object_get_data (G_OBJECT (adjustment), "gvc-mixer-dialog-stream");
        if (stream != NULL) {
                GObject *bar;
                gdouble volume, rounded;
                char *name;

                volume = gtk_adjustment_get_value (adjustment);
                rounded = round (volume);

                bar = g_object_get_data (G_OBJECT (adjustment), "gvc-mixer-dialog-bar");
                g_object_get (bar, "name", &name, NULL);
                g_debug ("Setting stream volume %lf (rounded: %lf) for bar '%s'", volume, rounded, name);
                g_free (name);

                /* FIXME would need to do that in the balance bar really... */
                /* Make sure we do not unmute muted streams, there's a button for that */
                if (volume == 0.0)
                        gvc_mixer_stream_set_is_muted (stream, TRUE);
                /* Only push the volume if it's actually changed */
                if (gvc_mixer_stream_set_volume(stream, (pa_volume_t) rounded) != FALSE)
                        gvc_mixer_stream_push_volume (stream);
        }
}

static void
on_bar_is_muted_notify (GObject        *object,
                        GParamSpec     *pspec,
                        GvcMixerDialog *dialog)
{
        gboolean        is_muted;
        GvcMixerStream *stream;

        is_muted = gvc_channel_bar_get_is_muted (GVC_CHANNEL_BAR (object));

        stream = g_object_get_data (object, "gvc-mixer-dialog-stream");
        if (stream != NULL) {
                gvc_mixer_stream_change_is_muted (stream, is_muted);
        } else {
                char *name;
                g_object_get (object, "name", &name, NULL);
                g_warning ("Unable to find stream for bar '%s'", name);
                g_free (name);
        }
}

static GtkWidget *
lookup_bar_for_stream (GvcMixerDialog *dialog,
                       GvcMixerStream *stream)
{
        GtkWidget *bar;

        bar = g_hash_table_lookup (dialog->priv->bars, GUINT_TO_POINTER (gvc_mixer_stream_get_id (stream)));

        return bar;
}

// TODO
// Do we need this ?
// UI devices now pull description material mainly for the card ports.
// Therefore the need for stream description dynamic changes more than ever seems unneccessary. 
static void
on_stream_description_notify (GvcMixerStream *stream,
                              GParamSpec     *pspec,
                              GvcMixerDialog *dialog)
{
        update_description (dialog, NAME_COLUMN,
                            gvc_mixer_stream_get_description (stream),
                            stream);
}

static void
on_stream_volume_notify (GObject        *object,
                         GParamSpec     *pspec,
                         GvcMixerDialog *dialog)
{
        GvcMixerStream *stream;
        GtkWidget      *bar;
        GtkAdjustment  *adj;
        stream = GVC_MIXER_STREAM (object);

        bar = lookup_bar_for_stream (dialog, stream);

        if (bar == NULL) {
                if (stream == gvc_mixer_control_get_default_sink(dialog->priv->mixer_control)) {
                        bar = dialog->priv->output_bar;
                }
                else if(stream == gvc_mixer_control_get_default_source(dialog->priv->mixer_control)) {
                        bar = dialog->priv->input_bar;
                }
                else{
                        g_warning ("Unable to find bar for stream %s in on_stream_volume_notify()",
                                   gvc_mixer_stream_get_name (stream));
                        return;
                }
        }

        adj = GTK_ADJUSTMENT (gvc_channel_bar_get_adjustment (GVC_CHANNEL_BAR (bar)));

        g_signal_handlers_block_by_func (adj,
                                         on_adjustment_value_changed,
                                         dialog);

        gtk_adjustment_set_value (adj,
                                  gvc_mixer_stream_get_volume (stream));

        g_signal_handlers_unblock_by_func (adj,
                                           on_adjustment_value_changed,
                                           dialog);
}

static void
on_stream_is_muted_notify (GObject        *object,
                           GParamSpec     *pspec,
                           GvcMixerDialog *dialog)
{
        GvcMixerStream *stream;
        GtkWidget      *bar;
        gboolean        is_muted;

        stream = GVC_MIXER_STREAM (object);

        bar = lookup_bar_for_stream (dialog, stream);

        if (bar == NULL) {
                if (stream == gvc_mixer_control_get_default_sink(dialog->priv->mixer_control)) {
                        bar = dialog->priv->output_bar;
                }
                else if(stream == gvc_mixer_control_get_default_source(dialog->priv->mixer_control)) {
                        bar = dialog->priv->input_bar;
                }                
                else{
                        g_warning ("Unable to find bar for stream %s in on_stream_muted_notify()",
                                   gvc_mixer_stream_get_name (stream));
                        return;
                }
        }


        is_muted = gvc_mixer_stream_get_is_muted (stream);
        gvc_channel_bar_set_is_muted (GVC_CHANNEL_BAR (bar),
                                      is_muted);

        if (stream == gvc_mixer_control_get_default_sink (dialog->priv->mixer_control)) {
                gtk_widget_set_sensitive (dialog->priv->applications_box,
                                          !is_muted);
        }

}

static void
save_bar_for_stream (GvcMixerDialog *dialog,
                     GvcMixerStream *stream,
                     GtkWidget      *bar)
{
        g_debug ("\n saving bar for stream %s",
                 gvc_mixer_stream_get_name (stream));
        g_hash_table_insert (dialog->priv->bars,
                             GUINT_TO_POINTER (gvc_mixer_stream_get_id (stream)),
                             bar);
}

static GtkWidget *
create_bar (GvcMixerDialog *dialog,
            gboolean        add_to_size_group,
            gboolean        symmetric)
{
        GtkWidget *bar;

        bar = gvc_channel_bar_new ();
        gtk_widget_set_sensitive (bar, FALSE);
        if (add_to_size_group && dialog->priv->size_group != NULL) {
                gvc_channel_bar_set_size_group (GVC_CHANNEL_BAR (bar),
                                                dialog->priv->size_group,
                                                symmetric);
        }
        gvc_channel_bar_set_orientation (GVC_CHANNEL_BAR (bar),
                                         GTK_ORIENTATION_HORIZONTAL);
        gvc_channel_bar_set_show_mute (GVC_CHANNEL_BAR (bar),
                                       TRUE);
        g_signal_connect (bar,
                          "notify::is-muted",
                          G_CALLBACK (on_bar_is_muted_notify),
                          dialog);
        return bar;
}

static GtkWidget *
create_app_bar (GvcMixerDialog *dialog,
                const char     *name,
                const char     *icon_name)
{
        GtkWidget *bar;

        bar = create_bar (dialog, FALSE, FALSE);
        gvc_channel_bar_set_ellipsize (GVC_CHANNEL_BAR (bar), TRUE);
        gvc_channel_bar_set_icon_name (GVC_CHANNEL_BAR (bar), icon_name);
        if (name == NULL || strchr (name, '_') == NULL) {
                gvc_channel_bar_set_name (GVC_CHANNEL_BAR (bar), name);
        } else {
                char **tokens, *escaped;

                tokens = g_strsplit (name, "_", -1);
                escaped = g_strjoinv ("__", tokens);
                g_strfreev (tokens);
                gvc_channel_bar_set_name (GVC_CHANNEL_BAR (bar), escaped);
                g_free (escaped);
        }

        return bar;
}

static gint test_it = 0;

/* active_input_update
 * Handle input update change from the backend (control). 
 * Trust the backend whole-heartedly to deliver the correct input
 * i.e. keep it MVC.
 */
static void
active_input_update (GvcMixerDialog *dialog,
                     GvcMixerUIDevice *active_input)
{         
        g_debug ("\n active_input_update %s \n", gvc_mixer_ui_device_get_description (active_input));
        // First make sure the correct UI device is selected.
        GtkTreeModel *model;
        GtkTreeIter   iter;

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (dialog->priv->input_treeview));

        if (gtk_tree_model_get_iter_first (model, &iter) == FALSE){
                g_warning ("The tree is empty => we have no devices so cannot set the active input");
                return;        
        }
        
        do {
                gboolean         is_selected = FALSE;
                gint             id;
                        
                gtk_tree_model_get (model, &iter,
                                    ID_COLUMN, &id,
                                    -1);

                is_selected = id == gvc_mixer_ui_device_get_id (active_input);
                
                gtk_list_store_set (GTK_LIST_STORE (model),
                                    &iter,
                                    ACTIVE_COLUMN, is_selected,
                                    -1);

                if (is_selected) {
                        GtkTreeSelection *selection;
                        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->priv->input_treeview));
                        gtk_tree_selection_select_iter (selection, &iter);
                }
                
        }while (gtk_tree_model_iter_next (model, &iter));


          // Not ideal but for now destroy the combo and recreate below.
        if (dialog->priv->input_profile_combo != NULL) {
                gtk_container_remove (GTK_CONTAINER (dialog->priv->input_settings_box),
                                      dialog->priv->input_profile_combo);
                dialog->priv->input_profile_combo = NULL;
        }

        GvcMixerStream       *stream;  
        GtkAdjustment        *adj;
  
        stream = gvc_mixer_control_get_stream_from_device (dialog->priv->mixer_control, 
                                                           active_input); 
        if (stream == NULL) {
                g_warning ("active_input_update - couldn't find a stream from the supposed active input");
                gtk_widget_set_sensitive (dialog->priv->input_bar, 
                                          FALSE);                
                return;
        }

        // Set the label accordingly
        adj = GTK_ADJUSTMENT (gvc_channel_bar_get_adjustment (GVC_CHANNEL_BAR (dialog->priv->input_bar)));
        g_signal_handlers_disconnect_by_func(adj, on_adjustment_value_changed, dialog);

        gchar *str = g_strdup_printf(_("Settings for %s"),
                                      gvc_mixer_ui_device_get_description (active_input));
        gtk_label_set_label (GTK_LABEL(dialog->priv->selected_input_label), str);
        g_free (str);

        gvc_channel_bar_set_base_volume (GVC_CHANNEL_BAR (dialog->priv->input_bar),
                                         gvc_mixer_stream_get_base_volume (stream));
        gvc_channel_bar_set_is_amplified (GVC_CHANNEL_BAR (dialog->priv->input_bar),
                                          gvc_mixer_stream_get_can_decibel (stream));
        /* Update the adjustment in case the previous bar wasn't decibel
         * capable, and we clipped it */
        adj = GTK_ADJUSTMENT (gvc_channel_bar_get_adjustment (GVC_CHANNEL_BAR (dialog->priv->input_bar)));
        gtk_adjustment_set_value (adj,
                                  gvc_mixer_stream_get_volume (stream));        
        
        stop_monitor_stream_for_source (dialog);
        //if (test_it < 6){
        create_monitor_stream_for_source (dialog, stream);
        test_it += 1;
        //}
        bar_set_stream (dialog, dialog->priv->input_bar, stream);   
        // remove any previous stream that might have been pointed at 
        // the static input bar and connect new signals from new stream.

        const GList* profiles = gvc_mixer_ui_device_get_profiles (active_input);

        if (profiles != NULL && !gvc_mixer_ui_device_should_profiles_be_hidden (active_input)){
                const gchar *active_profile;
                dialog->priv->input_profile_combo = gvc_combo_box_new (_("Mode:"));                
                gvc_combo_box_set_profiles (GVC_COMBO_BOX (dialog->priv->input_profile_combo),
                                            profiles);
                                
                active_profile = gvc_mixer_ui_device_get_active_profile (active_input);
                if (active_profile)
                        gvc_combo_box_set_active (GVC_COMBO_BOX (dialog->priv->input_profile_combo), active_profile);

                g_object_set_data (G_OBJECT (dialog->priv->input_profile_combo),
                                   "uidevice",
                                   active_input);

                g_signal_connect (G_OBJECT (dialog->priv->input_profile_combo), "changed",
                                  G_CALLBACK (profile_selection_changed), dialog);

                gtk_box_pack_start (GTK_BOX (dialog->priv->input_settings_box),
                                    dialog->priv->input_profile_combo,
                                    TRUE, FALSE, 0);

                if (dialog->priv->size_group != NULL) {
                        gvc_combo_box_set_size_group (GVC_COMBO_BOX (dialog->priv->input_profile_combo),
                                                      dialog->priv->size_group, FALSE);
                }
                gtk_widget_show (dialog->priv->input_profile_combo);                
        }
}

/* active_output_update
 * Handle output update change from the backend (control). 
 * Trust the backend whole heartedly to deliver the correct output
 * i.e. keep it MVC.
 */
static void
active_output_update (GvcMixerDialog *dialog,
                      GvcMixerUIDevice *active_output)
{                 
        // First make sure the correct UI device is selected.
        GtkTreeModel *model;
        GtkTreeIter   iter;
        g_debug ("\n\n active output update - device id = %i \n\n",
                 gvc_mixer_ui_device_get_id (active_output));

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (dialog->priv->output_treeview));

        if (gtk_tree_model_get_iter_first (model, &iter) == FALSE){
                g_warning ("The tree is empty => we have no devices in the tree => cannot set the active output");
                return;        
        }
        
        do {
                gboolean         is_selected;
                gint             id;
                        
                gtk_tree_model_get (model, &iter,
                                    ID_COLUMN, &id,
                                    ACTIVE_COLUMN, &is_selected,                  
                                    -1);

                if (is_selected && id == gvc_mixer_ui_device_get_id (active_output)) {
                        g_debug ("\n\n unneccessary active output update unless it was a profile change on the same device ? \n\n");
                }

                is_selected = id == gvc_mixer_ui_device_get_id (active_output);
                
                gtk_list_store_set (GTK_LIST_STORE (model),
                                    &iter,
                                    ACTIVE_COLUMN, is_selected,
                                    -1);

                if (is_selected) {
                        GtkTreeSelection *selection;
                        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->priv->output_treeview));
                        gtk_tree_selection_select_iter (selection, &iter);
                }
                
        }while (gtk_tree_model_iter_next (model, &iter));
        
          // Not ideal but for now destroy the combo and recreate below.
        if (dialog->priv->output_profile_combo != NULL) {
                gtk_container_remove (GTK_CONTAINER (dialog->priv->output_settings_box),
                                      dialog->priv->output_profile_combo);
                dialog->priv->output_profile_combo = NULL;
        }

        GvcMixerStream       *stream;  
        const GvcChannelMap  *map;
        GtkAdjustment        *adj;
  
        stream = gvc_mixer_control_get_stream_from_device (dialog->priv->mixer_control, 
                                                           active_output); 

        if (stream == NULL) {
                g_warning ("active_output_update - couldn't find a stream from the supposed active output");
                return;
        }

        gboolean is_muted = gvc_mixer_stream_get_is_muted (stream);
        gtk_widget_set_sensitive (dialog->priv->applications_box,
                                  !is_muted);
        adj = GTK_ADJUSTMENT (gvc_channel_bar_get_adjustment (GVC_CHANNEL_BAR (dialog->priv->output_bar)));
        g_signal_handlers_disconnect_by_func(adj, on_adjustment_value_changed, dialog);

        bar_set_stream (dialog, dialog->priv->output_bar, stream);
        gvc_channel_bar_set_base_volume (GVC_CHANNEL_BAR (dialog->priv->output_bar),
                                         gvc_mixer_stream_get_base_volume (stream));
        gvc_channel_bar_set_is_amplified (GVC_CHANNEL_BAR (dialog->priv->output_bar),
                                          gvc_mixer_stream_get_can_decibel (stream));
        /* Update the adjustment in case the previous bar wasn't decibel
         * capable, and we clipped it */
        adj = GTK_ADJUSTMENT (gvc_channel_bar_get_adjustment (GVC_CHANNEL_BAR (dialog->priv->output_bar)));
        gtk_adjustment_set_value (adj,
                                  gvc_mixer_stream_get_volume (stream));

        map = gvc_mixer_stream_get_channel_map (stream);

        if (map == NULL) {
                g_warning ("Active output stream has no channel map");
                gtk_widget_set_sensitive (dialog->priv->output_bar, FALSE);
                gtk_widget_set_sensitive (dialog->priv->output_balance_bar, FALSE);
                gtk_widget_set_sensitive (dialog->priv->output_lfe_bar, FALSE);
                gtk_widget_set_sensitive (dialog->priv->output_fade_bar, FALSE);                
                return;
        }


        // Swap bars to the active map
        gvc_balance_bar_set_map (GVC_BALANCE_BAR (dialog->priv->output_balance_bar), 
                                 map);  
        gvc_balance_bar_set_map (GVC_BALANCE_BAR (dialog->priv->output_fade_bar), 
                                 map);
        gvc_balance_bar_set_map (GVC_BALANCE_BAR (dialog->priv->output_lfe_bar), 
                                 map);
                           
        // Set sensitivities accordingly.
        gtk_widget_set_sensitive (dialog->priv->output_balance_bar,
                                  gvc_channel_map_can_balance (map));                                    
        gtk_widget_set_sensitive (dialog->priv->output_fade_bar, 
                                  gvc_channel_map_can_fade (map));
        gtk_widget_set_sensitive (dialog->priv->output_lfe_bar, 
                                  gvc_channel_map_has_lfe (map));
        gtk_widget_set_sensitive (dialog->priv->output_bar, 
                                  TRUE);
        // Set the label accordingly
        gchar *str = g_strdup_printf(_("Settings for %s"),
                                     gvc_mixer_ui_device_get_description (active_output));
        gtk_label_set_label (GTK_LABEL(dialog->priv->selected_output_label), str);
        g_free (str);

        g_debug ("\n active_output_update %s \n", gvc_mixer_ui_device_get_description (active_output));

        GList* profiles = gvc_mixer_ui_device_get_profiles (active_output);

        if (profiles != NULL && !gvc_mixer_ui_device_should_profiles_be_hidden (active_output)) {
                const gchar *active_profile;

                dialog->priv->output_profile_combo = gvc_combo_box_new (_("Mode:"));                
                gvc_combo_box_set_profiles (GVC_COMBO_BOX (dialog->priv->output_profile_combo),
                                            profiles);
                                
                gtk_box_pack_start (GTK_BOX (dialog->priv->output_settings_box),
                                    dialog->priv->output_profile_combo,
                                    FALSE, FALSE, 3);

                if (dialog->priv->size_group != NULL) {
                        gvc_combo_box_set_size_group (GVC_COMBO_BOX (dialog->priv->output_profile_combo),
                                                      dialog->priv->size_group, FALSE);
                }

                active_profile = gvc_mixer_ui_device_get_active_profile (active_output);
                if (active_profile)
                        gvc_combo_box_set_active (GVC_COMBO_BOX (dialog->priv->output_profile_combo), active_profile);
                                          
                
                g_object_set_data (G_OBJECT (dialog->priv->output_profile_combo),
                                   "uidevice",
                                   active_output);
                g_signal_connect (G_OBJECT (dialog->priv->output_profile_combo), "changed",
                                  G_CALLBACK (profile_selection_changed), dialog);

                gtk_widget_show (dialog->priv->output_profile_combo);                
        }

}

static void
bar_set_stream (GvcMixerDialog *dialog,
                GtkWidget      *bar,
                GvcMixerStream *stream)
{
        GtkAdjustment  *adj;

        g_assert (bar != NULL);

        gtk_widget_set_sensitive (bar, (stream != NULL));

        adj = GTK_ADJUSTMENT (gvc_channel_bar_get_adjustment (GVC_CHANNEL_BAR (bar)));
        
        g_signal_handlers_disconnect_by_func (adj, on_adjustment_value_changed, dialog);

        g_object_set_data (G_OBJECT (bar), "gvc-mixer-dialog-stream", stream);
        g_object_set_data (G_OBJECT (adj), "gvc-mixer-dialog-stream", stream);
        g_object_set_data (G_OBJECT (adj), "gvc-mixer-dialog-bar", bar);

        if (stream != NULL) {
                gboolean is_muted;

                is_muted = gvc_mixer_stream_get_is_muted (stream);
                gvc_channel_bar_set_is_muted (GVC_CHANNEL_BAR (bar), is_muted);

                gtk_adjustment_set_value (adj,
                                          gvc_mixer_stream_get_volume (stream));
                g_signal_connect (stream,
                                  "notify::is-muted",
                                  G_CALLBACK (on_stream_is_muted_notify),
                                  dialog);
                g_signal_connect (stream,
                                  "notify::volume",
                                  G_CALLBACK (on_stream_volume_notify),
                                  dialog);
                g_signal_connect (adj,
                                  "value-changed",
                                  G_CALLBACK (on_adjustment_value_changed),
                                  dialog);
        }
}

/**
* This method handles all streams that are not an input or output
* i.e. effects streams and application streams
* TODO rename to truly reflect its usage. 
**/
static void
add_stream (GvcMixerDialog *dialog,
            GvcMixerStream *stream)
{
  
        GtkWidget     *bar;
        bar = NULL;

        if (stream == gvc_mixer_control_get_event_sink_input (dialog->priv->mixer_control)) {
                bar = dialog->priv->effects_bar;
                g_debug ("Adding effects stream");
        } else {
                // Must be a sink/source input/output
                const char *name;
                name = gvc_mixer_stream_get_name (stream);
                g_debug ("\n Add bar for application stream : %s",
                             name);

                bar = create_app_bar (dialog, name,
                                      gvc_mixer_stream_get_icon_name (stream));

                gtk_box_pack_start (GTK_BOX (dialog->priv->applications_box), bar, FALSE, FALSE, 12);
                dialog->priv->num_apps++;
                gtk_widget_hide (dialog->priv->no_apps_label);
        }
        // We should have a bar by now.
        g_assert (bar != NULL);
        GvcMixerStream *old_stream;

        if (bar != NULL) {
                old_stream = g_object_get_data (G_OBJECT (bar), "gvc-mixer-dialog-stream");
                if (old_stream != NULL) {
                        char *name;
                        g_object_get (bar, "name", &name, NULL);
                        g_debug ("Disconnecting old stream '%s' from bar '%s'",
                                 gvc_mixer_stream_get_name (old_stream), name);
                        g_free (name);

                        g_signal_handlers_disconnect_by_func (old_stream, on_stream_is_muted_notify, dialog);
                        g_signal_handlers_disconnect_by_func (old_stream, on_stream_volume_notify, dialog);

                        g_hash_table_remove (dialog->priv->bars, GUINT_TO_POINTER (gvc_mixer_stream_get_id (old_stream)));
                }
                save_bar_for_stream (dialog, stream, bar);
                bar_set_stream (dialog, bar, stream);
                gtk_widget_show (bar);
        }
}

static void
remove_stream (GvcMixerDialog  *dialog,
               guint            id)
{        
        GtkWidget *bar;
        bar = g_hash_table_lookup (dialog->priv->bars, GUINT_TO_POINTER (id));
        if (bar != NULL) {
                g_hash_table_remove (dialog->priv->bars, GUINT_TO_POINTER (id));
                gtk_container_remove (GTK_CONTAINER (gtk_widget_get_parent (bar)),
                                      bar);
                dialog->priv->num_apps--;
                if (dialog->priv->num_apps == 0) {
                        gtk_widget_show (dialog->priv->no_apps_label);
                }
        }

}

static void
on_control_stream_added (GvcMixerControl *control,
                         guint            id,
                         GvcMixerDialog  *dialog)
{
        GvcMixerStream *stream;
        stream = gvc_mixer_control_lookup_stream_id (control, id);
        
        if (stream == NULL) 
                return;
        
        const char    *app_id;
        app_id = gvc_mixer_stream_get_application_id (stream);

        if (stream == gvc_mixer_control_get_event_sink_input (dialog->priv->mixer_control) || (!GVC_IS_MIXER_SOURCE (stream) &&
                !GVC_IS_MIXER_SINK (stream)
                && !gvc_mixer_stream_is_virtual (stream)
                && g_strcmp0 (app_id, "org.gnome.VolumeControl") != 0
                && g_strcmp0 (app_id, "org.PulseAudio.pavucontrol") != 0)
                && g_strcmp0 (app_id, "org.Cinnamon") != 0 /* FIXME: org.Cinnamon.xx.xx should be unified */
                && g_strcmp0 (app_id, "org.cinnamon") != 0) {

                GtkWidget      *bar;

                bar = g_hash_table_lookup (dialog->priv->bars, GUINT_TO_POINTER (id));
                if (bar != NULL) {
                        g_debug ("GvcMixerDialog: Stream %u already added", id);
                        return;
                }
                add_stream (dialog, stream);
        } 
}

static void
on_control_stream_removed (GvcMixerControl *control,
                           guint            id,
                           GvcMixerDialog  *dialog)
{
        remove_stream (dialog, id);
}

static gboolean
find_item_by_id (GtkTreeModel *model,
                 guint         id,
                 guint         column,
                 GtkTreeIter  *iter)
{
        gboolean found_item;

        found_item = FALSE;

        if (!gtk_tree_model_get_iter_first (model, iter)) {
                return FALSE;
        }

        do {
                guint t_id;

                gtk_tree_model_get (model, iter,
                                    column, &t_id, -1);

                if (id == t_id) {
                        found_item = TRUE;
                }
        } while (!found_item && gtk_tree_model_iter_next (model, iter));

        return found_item;
}

static void
add_input_ui_entry (GvcMixerDialog *dialog,
                    GvcMixerUIDevice *input)
{
        g_debug ("\n Add input ui entry with id : %u \n",
                  gvc_mixer_ui_device_get_id (input));

        gchar    *port_name;
        gchar    *origin;
        gchar    *description;
        gboolean active;
        gboolean available;
        gint     stream_id;
        GvcMixerCard  *card;

        g_object_get (G_OBJECT (input),
                     "stream-id", &stream_id,
                     "card", &card,
                     "origin", &origin,
                     "description", &description,
                     "port-name", &port_name,
                     "port-available", &available,
                      NULL);
        GtkTreeModel        *model;
        GtkTreeIter          iter;
        const GvcChannelMap *map;
        GIcon               *icon;

        if (card == NULL) {
                GvcMixerStream *stream;
                g_debug ("just detected a network source");
                stream = gvc_mixer_control_get_stream_from_device (dialog->priv->mixer_control, input);
                if (stream == NULL) {
                        g_warning ("tried to add the network source but the stream was null - fail ?!");
                        g_free (port_name);					
                        g_free (origin);							
                        g_free (description);		  				
                        return;
                }
                icon = gvc_mixer_stream_get_gicon (stream);			
        }
        else
                icon = gvc_mixer_card_get_gicon (card);                 

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (dialog->priv->input_treeview));
        gtk_list_store_append (GTK_LIST_STORE (model), &iter);

        gtk_list_store_set (GTK_LIST_STORE (model),
                            &iter,
                            NAME_COLUMN, description,
                            DEVICE_COLUMN, origin,
                            ACTIVE_COLUMN, FALSE,
                            ICON_COLUMN, icon,
                            ID_COLUMN, gvc_mixer_ui_device_get_id (input),
                            SPEAKERS_COLUMN,origin,
                            -1);

        if (icon != NULL)
                g_object_unref (icon);

        // TODO check this.
        /*g_signal_connect (output,
                          "notify::description",
                          G_CALLBACK (on_output_description_notify),
                          dialog);*/
              
        g_free (port_name);                                        
        g_free (origin);                                                        
        g_free (description);                   
}                                   

static void
add_output_ui_entry (GvcMixerDialog *dialog,
                     GvcMixerUIDevice *output)
{
        g_debug ("\n Add output ui entry with id : %u \n",
                  gvc_mixer_ui_device_get_id (output));

        gchar    *sink_port_name;
        gchar    *origin;
        gchar    *description;
        gboolean active;
        gboolean available;
        gint     sink_stream_id;
        GvcMixerCard  *card;

        g_object_get (G_OBJECT (output),
                     "stream-id", &sink_stream_id,
                     "card", &card,
                     "origin", &origin,
                     "description", &description,
                     "port-name", &sink_port_name,
                     "port-available", &available,
                      NULL);
	
        GtkTreeModel        *model;
        GtkTreeIter          iter;
        const GvcChannelMap *map;
        GIcon               *icon;

        if (card == NULL) {
                g_debug ("just detected a network sink");
                
                GvcMixerStream *stream;
                stream = gvc_mixer_control_get_stream_from_device (dialog->priv->mixer_control, output);
                if (stream == NULL) {
                        g_warning ("tried to add the network sink but the stream was null - fail ?!");
                        g_free (sink_port_name);					
                        g_free (origin);							
                        g_free (description);		  				
                        return;
                }
                icon = gvc_mixer_stream_get_gicon (stream);			
        }
        else
                icon = gvc_mixer_card_get_gicon (card);			


        model = gtk_tree_view_get_model (GTK_TREE_VIEW (dialog->priv->output_treeview));
        gtk_list_store_append (GTK_LIST_STORE (model), &iter);

        gtk_list_store_set (GTK_LIST_STORE (model),
                            &iter,
                            NAME_COLUMN, description,
                            DEVICE_COLUMN, origin,
                            ACTIVE_COLUMN, FALSE,
                            ICON_COLUMN, icon,
                            ID_COLUMN, gvc_mixer_ui_device_get_id (output),
                            SPEAKERS_COLUMN,origin,
                            -1);

        if (icon != NULL)
                g_object_unref (icon);

        // TODO check this.
        /*g_signal_connect (output,
                          "notify::description",
                          G_CALLBACK (on_output_description_notify),
                          dialog);*/
              
        g_free (sink_port_name);					
        g_free (origin);							
        g_free (description);		  	
}				    

static void
on_control_output_added (GvcMixerControl *control,
                         guint            id,
                         GvcMixerDialog  *dialog)
{
	GvcMixerUIDevice* out = NULL;
	out = gvc_mixer_control_lookup_output_id (control, id);

	if (out == NULL) {
		g_warning ("on_control_output_added - tried to fetch an output of id %u but got nothing", id);
		return;
	}

	add_output_ui_entry (dialog, out);
}

static void
on_control_active_output_update (GvcMixerControl *control,
                                 guint            id,
                                 GvcMixerDialog  *dialog)
{
	GvcMixerUIDevice* out = NULL;
	out = gvc_mixer_control_lookup_output_id (control, id);

	if (out == NULL) {
		g_warning ("\n on_control_active_output_update - tried to fetch an output of id %u but got nothing", id);
		return;
	}
        active_output_update (dialog, out);
}

static void
on_control_active_input_update (GvcMixerControl *control,
                                 guint            id,
                                 GvcMixerDialog  *dialog)
{
	GvcMixerUIDevice* in = NULL;
	in = gvc_mixer_control_lookup_input_id (control, id);

	if (in == NULL) {
		g_warning ("on_control_active_input_update - tried to fetch an input of id %u but got nothing", id);
		return;
	}
        active_input_update (dialog, in);
}

static void
on_control_input_added (GvcMixerControl *control,
                        guint            id,
                        GvcMixerDialog  *dialog)
{
	GvcMixerUIDevice* in = NULL;
	in = gvc_mixer_control_lookup_input_id (control, id);

	if (in == NULL) {
		g_warning ("on_control_input_added - tried to fetch an input of id %u but got nothing", id);
		return;
	}
	add_input_ui_entry (dialog, in);
}

static void
on_control_output_removed (GvcMixerControl *control,
                           guint	    id,
                           GvcMixerDialog  *dialog)
{
        GtkWidget    *bar;
        gboolean      found;
        GtkTreeIter   iter;
        GtkTreeModel *model;

        GvcMixerUIDevice* out = NULL;
        out = gvc_mixer_control_lookup_output_id (control, id);
        
        gint sink_stream_id;
        
        g_object_get (G_OBJECT (out),
                     "stream-id", &sink_stream_id,
                      NULL);
		      
        g_debug ("Remove output from dialog \n id : %u \n sink stream id : %i \n",
                  id,
                  sink_stream_id);

        /* remove from any models */
        model = gtk_tree_view_get_model (GTK_TREE_VIEW (dialog->priv->output_treeview));
        found = find_item_by_id (GTK_TREE_MODEL (model), id, ID_COLUMN, &iter);
        if (found) {
                gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
        }
}     


                 
static void
on_control_input_removed (GvcMixerControl *control,
			   guint	    id,
			   GvcMixerDialog  *dialog)
{
        GtkWidget    *bar;
        gboolean      found;
        GtkTreeIter   iter;
        GtkTreeModel *model;

        GvcMixerUIDevice* in = NULL;
        in = gvc_mixer_control_lookup_input_id (control, id);
        
        gint stream_id;
        
        g_object_get (G_OBJECT (in),
                     "stream-id", &stream_id,
                      NULL);
                      
        g_debug ("Remove input from dialog \n id : %u \n stream id : %i \n",
                  id,
                  stream_id);

        /* remove from any models */
        model = gtk_tree_view_get_model (GTK_TREE_VIEW (dialog->priv->input_treeview));
        found = find_item_by_id (GTK_TREE_MODEL (model), id, ID_COLUMN, &iter);
        if (found) {
                gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
        }        
}

static void
_gtk_label_make_bold (GtkLabel *label)
{
        PangoFontDescription *font_desc;

        font_desc = pango_font_description_new ();

        pango_font_description_set_weight (font_desc,
                                           PANGO_WEIGHT_BOLD);

        /* This will only affect the weight of the font, the rest is
         * from the current state of the widget, which comes from the
         * theme or user prefs, since the font desc only has the
         * weight flag turned on.
         */
        gtk_widget_modify_font (GTK_WIDGET (label), font_desc);

        pango_font_description_free (font_desc);
}


static void
on_input_selection_changed (GtkTreeSelection *selection,
                             GvcMixerDialog   *dialog)
{
        GtkTreeModel *model;
        GtkTreeIter   iter;
        gboolean      toggled;
        guint         id;

        if (gtk_tree_selection_get_selected (selection, &model, &iter) == FALSE) {
                g_debug ("Could not get default input from selection");
                return;
        }

        gtk_tree_model_get (model, &iter,
                            ID_COLUMN, &id,
                            ACTIVE_COLUMN, &toggled,
                            -1);

        toggled ^= 1;
        GvcMixerUIDevice *input;
        //g_debug ("on_input_selection_changed - try swap to input with id %u", id); 
        input = gvc_mixer_control_lookup_input_id (dialog->priv->mixer_control, id);
        
        if (input == NULL) {
                g_warning ("on_input_selection_changed - Unable to find input with id: %u", id);
                return;
        }

        gvc_mixer_control_change_input (dialog->priv->mixer_control, input);
}

static void
on_output_selection_changed (GtkTreeSelection *selection,
                             GvcMixerDialog   *dialog)
{
        GtkTreeModel *model;
        GtkTreeIter   iter;
        gboolean      active;
        guint         id;

        if (gtk_tree_selection_get_selected (selection, &model, &iter) == FALSE) {
                g_debug ("Could not get default output from selection");
                return;
        }

        gtk_tree_model_get (model, &iter,
                            ID_COLUMN, &id,
                            ACTIVE_COLUMN, &active,
                            -1);
        
        g_debug ("\n\n on_output_selection_changed - active %i \n\n", active); 
        if (active){
                return;
        }

        GvcMixerUIDevice *output;
        g_debug ("\n on_output_selection_changed - try swap to output with id %u", id); 
	output = gvc_mixer_control_lookup_output_id (dialog->priv->mixer_control, id);
        
	if (output == NULL) {
		g_warning ("on_output_selection_changed - Unable to find output with id: %u", id);
		return;
	}

        gvc_mixer_control_change_output (dialog->priv->mixer_control, output);
}

static void
name_to_text (GtkTreeViewColumn *column,
              GtkCellRenderer *cell,
              GtkTreeModel *model,
              GtkTreeIter *iter,
              gpointer user_data)
{
        char *name, *mapping;

        gtk_tree_model_get(model, iter,
                           NAME_COLUMN, &name,
                           SPEAKERS_COLUMN, &mapping,
                           -1);

        if (mapping == NULL) {
                g_object_set (cell, "text", name, NULL);
        } else {
                char *str;

                str = g_strdup_printf ("%s\n<i>%s</i>",
                                       name, mapping);
                g_object_set (cell, "markup", str, NULL);
                g_free (str);
        }

        g_free (name);
        g_free (mapping);
}

static GtkWidget *
create_stream_treeview (GvcMixerDialog *dialog,
                        GCallback       on_selection_changed)
{
        GtkWidget         *treeview;
        GtkListStore      *store;
        GtkCellRenderer   *renderer;
        GtkTreeViewColumn *column;
        GtkTreeSelection  *selection;

        treeview = gtk_tree_view_new ();
        gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (treeview), FALSE);

        store = gtk_list_store_new (NUM_COLUMNS,
                                    G_TYPE_STRING,
                                    G_TYPE_STRING,
                                    G_TYPE_BOOLEAN,
                                    G_TYPE_UINT,
                                    G_TYPE_STRING,
                                    G_TYPE_ICON);
        gtk_tree_view_set_model (GTK_TREE_VIEW (treeview),
                                 GTK_TREE_MODEL (store));

        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
        gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);

        column = gtk_tree_view_column_new ();
        gtk_tree_view_column_set_title (column, _("Name"));
        renderer = gtk_cell_renderer_pixbuf_new ();
        gtk_tree_view_column_pack_start (column, renderer, FALSE);
        g_object_set (G_OBJECT (renderer), "stock-size", GTK_ICON_SIZE_LARGE_TOOLBAR, NULL);
        gtk_tree_view_column_set_attributes (column, renderer,
                                             "gicon", ICON_COLUMN,
                                             NULL);

        renderer = gtk_cell_renderer_text_new ();
        gtk_tree_view_column_pack_start (column, renderer, TRUE);
        gtk_tree_view_column_set_cell_data_func (column, renderer,
                                                 name_to_text, NULL, NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

        g_signal_connect ( selection, "changed",
                          on_selection_changed, dialog);
#if 0
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Device"),
                                                           renderer,
                                                           "text", DEVICE_COLUMN,
                                                           NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
#endif
        return treeview;
}

static void
on_profile_changed (GvcComboBox *widget,
                    const char  *profile,
                    gpointer     user_data)
{
        GvcMixerCard        *card;

        card = g_object_get_data (G_OBJECT (widget), "card");
        if (card == NULL) {
                g_warning ("Could not find card for combobox");
                return;
        }

        g_debug ("Profile changed to %s for card %s", profile,
                 gvc_mixer_card_get_name (card));

        gvc_mixer_card_change_profile (card, profile);
}

static void
on_test_speakers_clicked (GtkButton *widget,
                          gpointer  user_data)
{
        GvcMixerDialog      *dialog = GVC_MIXER_DIALOG (user_data);
        GtkTreeModel        *model;
        GtkTreeIter         iter;
        gint                active_output = GVC_MIXER_UI_DEVICE_INVALID;

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (dialog->priv->output_treeview));
        
        if (gtk_tree_model_get_iter_first (model, &iter) == FALSE){
                g_warning ("The tree is empty => we have no device to test speakers with return");
                return;        
        }
                
        do {
                gboolean         is_selected = FALSE ;
                gint             id;
                        
                gtk_tree_model_get (model, &iter,
                                    ID_COLUMN, &id,
                                    ACTIVE_COLUMN, &is_selected,
                                    -1);
                
                if (is_selected) {
                        active_output = id;
                        break;
                }
                
        }while (gtk_tree_model_iter_next (model, &iter));
        
        if (active_output == GVC_MIXER_UI_DEVICE_INVALID) {
                g_warning ("Cant find the active output from the UI");
                return;
        }        

        GvcMixerUIDevice *output;
        output = gvc_mixer_control_lookup_output_id (dialog->priv->mixer_control, (guint)active_output);
        gint stream_id = gvc_mixer_ui_device_get_stream_id(output);

        if (stream_id == GVC_MIXER_UI_DEVICE_INVALID)
                return;

        g_debug ("Test the speakers on the %s", gvc_mixer_ui_device_get_description (output));
        
        GvcMixerStream        *stream;
        GvcMixerCardProfile *profile;
        GtkWidget           *d, *speaker_test, *container;
        char                *title;

        stream = gvc_mixer_control_lookup_stream_id (dialog->priv->mixer_control, stream_id);
        if (stream == NULL) {
                g_debug ("Stream/sink not found");
                return;
        }
        title = g_strdup_printf (_("Speaker Testing for %s"), gvc_mixer_ui_device_get_description (output));
        d = gtk_dialog_new_with_buttons (title,
                                         GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (widget))),
                                         GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                         GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
                                         NULL);
        gtk_window_set_has_resize_grip (GTK_WINDOW (d), FALSE);

        g_free (title);
        speaker_test = gvc_speaker_test_new (dialog->priv->mixer_control,
                                             stream);
        gtk_widget_show (speaker_test);
        container = gtk_dialog_get_content_area (GTK_DIALOG (d));
        gtk_container_add (GTK_CONTAINER (container), speaker_test);

        gtk_dialog_run (GTK_DIALOG (d));
        gtk_widget_destroy (d);
}

static void
change_sound_file (GtkFileChooserButton *widget,
                    gpointer              user_data)
{
    GSettings * settings = user_data;
    char * filename = gtk_file_chooser_get_filename(widget);
    g_settings_set_string(settings, g_object_get_data (G_OBJECT (widget), "gsetting_key"), filename);
    g_free(filename);
}


static void
toggle_sound_file (GtkToggleButton *widget,
                    gpointer              user_data)
{
    GSettings *settings = user_data;
    gboolean active = gtk_toggle_button_get_active (widget);
    g_settings_set_boolean(settings, g_object_get_data (G_OBJECT (widget), "gsetting_key"), active);
    gtk_widget_set_sensitive(g_object_get_data (G_OBJECT (widget), "widget"), active);
}

static void
test_sound_file (GtkButton *widget,
                          gpointer  user_data)
{
    GvcMixerDialog      *dialog = GVC_MIXER_DIALOG (user_data);
    char * filename = gtk_file_chooser_get_filename(g_object_get_data (G_OBJECT (widget), "widget"));    
    ca_context_play (dialog->priv->sound_context, 0, CA_PROP_MEDIA_FILENAME, filename, NULL);
}

static void
add_sound_effect_selector (GtkWidget * grid, int row, char * label, GSettings * settings, char * file_key, char * enabled_key, GvcMixerDialog  * dialog)
{
        GtkWidget * file_chooser = gtk_file_chooser_button_new(_("Choose an audio file"), GTK_FILE_CHOOSER_ACTION_OPEN);
        g_object_set_data (G_OBJECT (file_chooser), "gsetting_key", file_key);

        gtk_file_chooser_set_filename(file_chooser, g_settings_get_string(settings, file_key));
        gtk_widget_set_sensitive(file_chooser, g_settings_get_boolean(settings, enabled_key));
        g_signal_connect (file_chooser, "file-set", G_CALLBACK (change_sound_file), settings);
        
        GtkWidget * checkbox = gtk_check_button_new_with_label(label);
        g_object_set_data (G_OBJECT (checkbox), "widget", file_chooser);
        g_object_set_data (G_OBJECT (checkbox), "gsetting_key", enabled_key);        
        gtk_toggle_button_set_active(checkbox, g_settings_get_boolean(settings, enabled_key));
        g_signal_connect (checkbox, "toggled", G_CALLBACK (toggle_sound_file), settings);

        GtkWidget * button = gtk_button_new();        
        GtkWidget * image = gtk_image_new_from_stock(GTK_STOCK_MEDIA_PLAY, GTK_ICON_SIZE_MENU);
        gtk_button_set_image(button, image);
        g_object_set_data (G_OBJECT (button), "widget", file_chooser);
        g_signal_connect (button, "released", G_CALLBACK (test_sound_file), dialog);

        gtk_grid_attach(grid, checkbox, 0, row, 1, 1);        
        gtk_grid_attach(grid, file_chooser, 1, row, 1, 1);       
        gtk_grid_attach(grid, button, 2, row, 1, 1);
}



static GObject *
gvc_mixer_dialog_constructor (GType                  type,
                              guint                  n_construct_properties,
                              GObjectConstructParam *construct_params)
{
        GObject          *object;
        GvcMixerDialog   *self;
        GtkWidget        *main_vbox;
        GtkWidget        *label;
        GtkWidget        *alignment;
        GtkWidget        *alignment_settings_box;
        GtkWidget        *settings_box;
        GtkWidget        *box;
        GtkWidget        *sbox;
        GtkWidget        *ebox;
        GtkWidget        *test_sound_box;
        GSList           *streams;
        GSList           *cards;
        GSList           *l;
        GvcMixerStream   *stream;
        GvcMixerCard     *card;
        GtkTreeSelection *selection;        

        object = G_OBJECT_CLASS (gvc_mixer_dialog_parent_class)->constructor (type, n_construct_properties, construct_params);

        self = GVC_MIXER_DIALOG (object);

        main_vbox = GTK_WIDGET (self);
        gtk_box_set_spacing (GTK_BOX (main_vbox), 2);

        gtk_container_set_border_width (GTK_CONTAINER (self), 3);        

        self->priv->output_stream_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
        alignment = gtk_alignment_new (0, 0, 1, 1);
        gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 0, 0, 0, 0);
        gtk_container_add (GTK_CONTAINER (alignment), self->priv->output_stream_box);
        gtk_box_pack_start (GTK_BOX (main_vbox),
                            alignment,
                            FALSE, FALSE, 0);

        self->priv->notebook = gtk_notebook_new ();
        gtk_box_pack_start (GTK_BOX (main_vbox),
                            self->priv->notebook,
                            TRUE, TRUE, 0);
        gtk_container_set_border_width (GTK_CONTAINER (self->priv->notebook), 5);

        /* Output page */
        self->priv->output_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);        
        gtk_container_set_border_width (GTK_CONTAINER (self->priv->output_box), 12);
        label = gtk_label_new (_("Output"));
        gtk_notebook_append_page (GTK_NOTEBOOK (self->priv->notebook),
                                  self->priv->output_box,
                                  label);

        box = gtk_frame_new (_("Play sound through"));
        gtk_widget_set_size_request (GTK_WIDGET (box), 310, -1);        
        label = gtk_frame_get_label_widget (GTK_FRAME (box));
        _gtk_label_make_bold (GTK_LABEL (label));
        gtk_label_set_use_underline (GTK_LABEL (label), TRUE);
        gtk_frame_set_shadow_type (GTK_FRAME (box), GTK_SHADOW_NONE);
        gtk_box_pack_start (GTK_BOX (self->priv->output_box), box, FALSE, TRUE, 0);

        alignment = gtk_alignment_new (0, 0, 1, 1);
        gtk_container_add (GTK_CONTAINER (box), alignment);
        gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 10, 5, 0, 15);

        self->priv->output_treeview = create_stream_treeview (self,
                                                              G_CALLBACK (on_output_selection_changed));
        gtk_label_set_mnemonic_widget (GTK_LABEL (label), self->priv->output_treeview);

        box = gtk_scrolled_window_new (NULL, NULL);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (box),
                                        GTK_POLICY_NEVER,
                                        GTK_POLICY_AUTOMATIC);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (box),
                                             GTK_SHADOW_IN);
        gtk_container_add (GTK_CONTAINER (box), self->priv->output_treeview);
        gtk_container_add (GTK_CONTAINER (alignment), box);
        
        self->priv->selected_output_label = gtk_label_new (_("Settings for the selected device"));
        gtk_widget_set_halign (self->priv->selected_output_label, GTK_ALIGN_START);
        gtk_widget_set_valign (self->priv->selected_output_label, GTK_ALIGN_START);       
        gtk_misc_set_padding (GTK_MISC (self->priv->selected_output_label), 0, 0);
        _gtk_label_make_bold (GTK_LABEL (self->priv->selected_output_label));
        settings_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
        alignment_settings_box = gtk_alignment_new (0, 0, 1, 1);
        gtk_alignment_set_padding (GTK_ALIGNMENT (alignment_settings_box), 7, 0, 0, 0);
        self->priv->output_settings_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
        gtk_container_add (GTK_CONTAINER (alignment_settings_box), self->priv->output_settings_box);

        gtk_box_pack_start (GTK_BOX (self->priv->output_box),
                            settings_box,
                            FALSE,
                            FALSE,
                            0);
        gtk_box_pack_start (GTK_BOX (settings_box),
                            self->priv->selected_output_label,
                            FALSE,
                            FALSE,
                            0);
        gtk_box_pack_start (GTK_BOX (settings_box),
                            alignment_settings_box,
                            FALSE,
                            FALSE,
                            0);

        self->priv->output_balance_bar = gvc_balance_bar_new (BALANCE_TYPE_RL);
        if (self->priv->size_group != NULL) {
                gvc_balance_bar_set_size_group (GVC_BALANCE_BAR (self->priv->output_balance_bar),
                                                self->priv->size_group,
                                                FALSE);
        }
        gtk_box_pack_start (GTK_BOX (self->priv->output_settings_box),
                            self->priv->output_balance_bar,
                            FALSE, FALSE, 3);
        gtk_widget_show (self->priv->output_balance_bar);

        self->priv->output_fade_bar = gvc_balance_bar_new (BALANCE_TYPE_FR);
        if (self->priv->size_group != NULL) {
                gvc_balance_bar_set_size_group (GVC_BALANCE_BAR (self->priv->output_fade_bar),
                                                self->priv->size_group,
                                                FALSE);
        }
        gtk_box_pack_start (GTK_BOX (self->priv->output_settings_box),
                            self->priv->output_fade_bar,
                            FALSE, FALSE, 3);
        gtk_widget_show (self->priv->output_fade_bar);

        self->priv->output_lfe_bar = gvc_balance_bar_new (BALANCE_TYPE_LFE);
        if (self->priv->size_group != NULL) {
                gvc_balance_bar_set_size_group (GVC_BALANCE_BAR (self->priv->output_lfe_bar),
                                                self->priv->size_group,
                                                FALSE);
        }
        gtk_box_pack_start (GTK_BOX (self->priv->output_settings_box),
                            self->priv->output_lfe_bar,
                            FALSE, FALSE, 3);
        gtk_widget_show (self->priv->output_lfe_bar);
                
        /* Creating a box and try to deal using the same size group. */
        test_sound_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_box_pack_end (GTK_BOX (self->priv->output_settings_box),
                          test_sound_box,
                          FALSE, 
                          FALSE,
                          5);

        sbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_box_pack_start (GTK_BOX (test_sound_box),
                            sbox,
                            FALSE, FALSE, 0);

        label = gtk_label_new (_("Test:"));
        gtk_box_pack_start (GTK_BOX (sbox),
                            label,
                            FALSE, FALSE, 0);
        if (self->priv->size_group != NULL)
                gtk_size_group_add_widget (self->priv->size_group, sbox);

        self->priv->test_output_button = gtk_button_new_with_label (_("Test Sound"));
        
        /* FIXME: I am getting mental with all these hardcoded padding values,
         * Here 8 works fine, not sure why. */
        gtk_box_pack_start (GTK_BOX (test_sound_box),
                            self->priv->test_output_button,
                            TRUE, TRUE, 8);

        /* Is this needed */
        if (self->priv->size_group != NULL)
                gtk_size_group_add_widget (self->priv->size_group, self->priv->test_output_button);

        gtk_widget_show (test_sound_box);

        //gtk_container_add (GTK_CONTAINER (box), self->priv->output_settings_box);
	    g_signal_connect (self->priv->test_output_button,
                          "released",
                           G_CALLBACK (on_test_speakers_clicked),
                           self);

        /* Input page */
        self->priv->input_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
        gtk_container_set_border_width (GTK_CONTAINER (self->priv->input_box), 12);
        label = gtk_label_new (_("Input"));
        gtk_notebook_append_page (GTK_NOTEBOOK (self->priv->notebook),
                                  self->priv->input_box,
                                  label);
        box = gtk_frame_new (_("Record sound from"));
        gtk_widget_set_size_request (GTK_WIDGET (box), 310, -1);        
        label = gtk_frame_get_label_widget (GTK_FRAME (box));
        _gtk_label_make_bold (GTK_LABEL (label));
        gtk_label_set_use_underline (GTK_LABEL (label), TRUE);
        gtk_frame_set_shadow_type (GTK_FRAME (box), GTK_SHADOW_NONE);
        gtk_box_pack_start (GTK_BOX (self->priv->input_box), box, FALSE, TRUE, 0);

        alignment = gtk_alignment_new (0, 0, 1, 1);
        gtk_container_add (GTK_CONTAINER (box), alignment);
        gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 10, 5, 0, 15);

        self->priv->input_treeview = create_stream_treeview (self,
                                                             G_CALLBACK (on_input_selection_changed));
        gtk_label_set_mnemonic_widget (GTK_LABEL (label), self->priv->input_treeview);

        box = gtk_scrolled_window_new (NULL, NULL);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (box),
                                        GTK_POLICY_NEVER,
                                        GTK_POLICY_AUTOMATIC);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (box),
                                             GTK_SHADOW_IN);
        gtk_container_add (GTK_CONTAINER (box), self->priv->input_treeview);
        gtk_container_add (GTK_CONTAINER (alignment), box);

        self->priv->selected_input_label = gtk_label_new (_("Settings for the selected device"));
        gtk_widget_set_halign (self->priv->selected_input_label, GTK_ALIGN_START);
        _gtk_label_make_bold (GTK_LABEL (self->priv->selected_input_label));
        
        self->priv->input_settings_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
        gtk_box_pack_start (GTK_BOX (self->priv->input_box),
                            self->priv->input_settings_box,
                            FALSE,
                            FALSE,
                            0);
        gtk_box_pack_start (GTK_BOX (self->priv->input_settings_box),
                            self->priv->selected_input_label,
                            FALSE,
                            FALSE,
                            0);     

        self->priv->input_bar = create_bar (self, FALSE, TRUE);
        gvc_channel_bar_set_name (GVC_CHANNEL_BAR (self->priv->input_bar),
                                  _("_Input volume:"));
        gvc_channel_bar_set_low_icon_name (GVC_CHANNEL_BAR (self->priv->input_bar),
                                           "cin-audio-input-microphone-low-symbolic");
        gvc_channel_bar_set_high_icon_name (GVC_CHANNEL_BAR (self->priv->input_bar),
                                            "cin-audio-input-microphone-high-symbolic");
        gtk_widget_set_sensitive (self->priv->input_bar, FALSE);

        if (self->priv->size_group != NULL) {
                gvc_channel_bar_set_size_group (GVC_CHANNEL_BAR (self->priv->input_bar),
                                                self->priv->size_group,
                                                FALSE);                
        }

        gtk_box_pack_start (GTK_BOX (self->priv->input_settings_box),
                            self->priv->input_bar,
                            FALSE, FALSE, 15);
        gtk_widget_show (self->priv->input_bar);
        

        /* Creating a box and try to deal using the same size group. */
        GtkWidget *input_level_box;
        input_level_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_box_pack_start (GTK_BOX (self->priv->input_settings_box),
                            input_level_box,
                            FALSE,
                            FALSE,
                            5);

        sbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_box_pack_start (GTK_BOX (input_level_box),
                            sbox,
                            FALSE, FALSE, 0);
        
        label = gtk_label_new (_("Input level:"));
        gtk_box_pack_start (GTK_BOX (sbox),
                            label,
                            FALSE, FALSE, 0);
        if (self->priv->size_group != NULL)
                gtk_size_group_add_widget (self->priv->size_group, sbox);


        self->priv->input_level_bar = gvc_level_bar_new ();
        gvc_level_bar_set_orientation (GVC_LEVEL_BAR (self->priv->input_level_bar),
                                       GTK_ORIENTATION_HORIZONTAL);
        gvc_level_bar_set_scale (GVC_LEVEL_BAR (self->priv->input_level_bar),
                                 GVC_LEVEL_SCALE_LINEAR);

        gtk_box_pack_start (GTK_BOX (input_level_box),
                            self->priv->input_level_bar,
                            TRUE, TRUE, 0);        
        
        /* Sound context */
        ca_context_create (&self->priv->sound_context);
        ca_context_change_props (self->priv->sound_context, CA_PROP_APPLICATION_NAME, "cinnamon", CA_PROP_APPLICATION_ID, "org.Cinnamon", NULL);
        ca_context_open (self->priv->sound_context);

        /* Effects page */
        self->priv->sound_effects_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
        gtk_container_set_border_width (GTK_CONTAINER (self->priv->sound_effects_box), 12);
        label = gtk_label_new (_("Sound Effects"));
        gtk_notebook_append_page (GTK_NOTEBOOK (self->priv->notebook),
                                  self->priv->sound_effects_box,
                                  label);

        self->priv->effects_bar = create_bar (self, FALSE, TRUE);
        gvc_channel_bar_set_name (GVC_CHANNEL_BAR (self->priv->effects_bar),
                                  _("_Alert volume:"));
        gtk_widget_set_sensitive (self->priv->effects_bar, FALSE);
        gtk_box_pack_start (GTK_BOX (self->priv->sound_effects_box),
                            self->priv->effects_bar, FALSE, FALSE, 0);

        GSettings * sound_settings = g_settings_new("org.cinnamon.sounds");
        GSettings * desktop_sound_settings = g_settings_new("org.cinnamon.desktop.sound");

        GtkWidget * sounds_grid = gtk_grid_new ();
        gtk_grid_set_column_spacing (sounds_grid, 6);
        gtk_grid_set_row_spacing (sounds_grid, 3);

        add_sound_effect_selector (sounds_grid, 0, _("Starting Cinnamon:"), sound_settings, "login-file", "login-enabled", self);
        add_sound_effect_selector (sounds_grid, 1, _("Switching workspace:"), sound_settings, "switch-file", "switch-enabled", self);       
        add_sound_effect_selector (sounds_grid, 2, _("Opening new windows:"), sound_settings, "map-file", "map-enabled", self);       
        add_sound_effect_selector (sounds_grid, 3, _("Closing windows:"), sound_settings, "close-file", "close-enabled", self);       
        add_sound_effect_selector (sounds_grid, 4, _("Minimizing windows:"), sound_settings, "minimize-file", "minimize-enabled", self);       
        add_sound_effect_selector (sounds_grid, 5, _("Maximizing windows:"), sound_settings, "maximize-file", "maximize-enabled", self);       
        add_sound_effect_selector (sounds_grid, 6, _("Unmaximizing windows:"), sound_settings, "unmaximize-file", "unmaximize-enabled", self);       
        add_sound_effect_selector (sounds_grid, 7, _("Tiling and snapping windows:"), sound_settings, "tile-file", "tile-enabled", self);       
        add_sound_effect_selector (sounds_grid, 8, _("Inserting a device:"), sound_settings, "plug-file", "plug-enabled", self);       
        add_sound_effect_selector (sounds_grid, 9, _("Removing a device:"), sound_settings, "unplug-file", "unplug-enabled", self);       
        add_sound_effect_selector (sounds_grid, 10, _("Changing the sound volume:"), desktop_sound_settings, "volume-sound-file", "volume-sound-enabled", self);
        add_sound_effect_selector (sounds_grid, 11, _("Leaving Cinnamon:"), sound_settings, "logout-file", "logout-enabled", self);

        gtk_box_pack_start (GTK_BOX (self->priv->sound_effects_box), sounds_grid, FALSE, FALSE, 0);
      
        /* Applications */
        self->priv->applications_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
        gtk_container_set_border_width (GTK_CONTAINER (self->priv->applications_box), 12);
        label = gtk_label_new (_("Applications"));
        gtk_notebook_append_page (GTK_NOTEBOOK (self->priv->notebook),
                                  self->priv->applications_box,
                                  label);
        self->priv->no_apps_label = gtk_label_new (_("No application is currently playing or recording audio."));
        gtk_box_pack_start (GTK_BOX (self->priv->applications_box),
                            self->priv->no_apps_label,
                            TRUE, TRUE, 0);

        self->priv->output_stream_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
        alignment = gtk_alignment_new (0, 0, 1, 1);
        gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 12, 0, 0, 0);
        gtk_container_add (GTK_CONTAINER (alignment), self->priv->output_stream_box);
        gtk_box_pack_start (GTK_BOX (main_vbox),
                            alignment,
                            FALSE, FALSE, 0);
        // Output volume
        self->priv->output_bar = create_bar (self, FALSE, TRUE);
        gvc_channel_bar_set_name (GVC_CHANNEL_BAR (self->priv->output_bar),
                                  _("_Output volume:"));
        gtk_widget_set_sensitive (self->priv->output_bar, FALSE);
        gtk_widget_set_size_request (self->priv->output_bar, 460, -1);        

        gtk_box_pack_start (GTK_BOX (self->priv->output_stream_box),
                            self->priv->output_bar, TRUE, FALSE, 12);

        gtk_widget_show_all (main_vbox);

        g_signal_connect (self->priv->mixer_control,
                          "stream-added",
                          G_CALLBACK (on_control_stream_added),
                          self);
        g_signal_connect (self->priv->mixer_control,
                          "stream-removed",
                          G_CALLBACK (on_control_stream_removed),
                          self);
        g_signal_connect (self->priv->mixer_control,
                          "output-added",
                          G_CALLBACK (on_control_output_added),
                          self);
        g_signal_connect (self->priv->mixer_control,
                          "output-removed",
                          G_CALLBACK (on_control_output_removed),
                          self);
        g_signal_connect (self->priv->mixer_control,
                          "input-added",
                          G_CALLBACK (on_control_input_added),
                          self);
        g_signal_connect (self->priv->mixer_control,
                          "input-removed",
                          G_CALLBACK (on_control_input_removed),
                          self);        
        return object;
}



static void
gvc_mixer_dialog_dispose (GObject *object)
{
        GvcMixerDialog *dialog = GVC_MIXER_DIALOG (object);
        
        g_clear_object (&dialog->priv->sound_settings);

        if (dialog->priv->mixer_control != NULL) {
                g_signal_handlers_disconnect_by_func (dialog->priv->mixer_control,
                                                      on_control_output_added,
                                                      dialog);
                g_signal_handlers_disconnect_by_func (dialog->priv->mixer_control,
                                                      on_control_output_removed,
                                                      dialog);
                g_signal_handlers_disconnect_by_func (dialog->priv->mixer_control,
                                                      on_control_active_input_update,
                                                      dialog);
                g_signal_handlers_disconnect_by_func (dialog->priv->mixer_control,
                                                      on_control_active_output_update,
                                                      dialog);                                            
                g_signal_handlers_disconnect_by_func (dialog->priv->mixer_control,
                                                      on_control_input_added,
                                                      dialog);
                g_signal_handlers_disconnect_by_func (dialog->priv->mixer_control,
                                                      on_control_input_removed,
                                                      dialog);
                g_signal_handlers_disconnect_by_func (dialog->priv->mixer_control,
                                                      on_control_stream_added,
                                                      dialog);
                g_signal_handlers_disconnect_by_func (dialog->priv->mixer_control,
                                                      on_control_stream_removed,
                                                      dialog);
                g_object_unref (dialog->priv->mixer_control);
                dialog->priv->mixer_control = NULL;
        }

        if (dialog->priv->bars != NULL) {
                g_hash_table_destroy (dialog->priv->bars);
                dialog->priv->bars = NULL;
        }

        G_OBJECT_CLASS (gvc_mixer_dialog_parent_class)->dispose (object);
}

static void
gvc_mixer_dialog_class_init (GvcMixerDialogClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->constructor = gvc_mixer_dialog_constructor;
        object_class->dispose = gvc_mixer_dialog_dispose;
        object_class->finalize = gvc_mixer_dialog_finalize;
        object_class->set_property = gvc_mixer_dialog_set_property;
        object_class->get_property = gvc_mixer_dialog_get_property;

        g_object_class_install_property (object_class,
                                         PROP_MIXER_CONTROL,
                                         g_param_spec_object ("mixer-control",
                                                              "mixer control",
                                                              "mixer control",
                                                              GVC_TYPE_MIXER_CONTROL,
                                                              G_PARAM_READWRITE|G_PARAM_CONSTRUCT));

        g_type_class_add_private (klass, sizeof (GvcMixerDialogPrivate));
}


static void
gvc_mixer_dialog_init (GvcMixerDialog *dialog)
{
        dialog->priv = GVC_MIXER_DIALOG_GET_PRIVATE (dialog);
        dialog->priv->bars = g_hash_table_new (NULL, NULL);
        dialog->priv->size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
}

static void
gvc_mixer_dialog_finalize (GObject *object)
{
        GvcMixerDialog *mixer_dialog;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GVC_IS_MIXER_DIALOG (object));

        mixer_dialog = GVC_MIXER_DIALOG (object);

        g_return_if_fail (mixer_dialog->priv != NULL);
        G_OBJECT_CLASS (gvc_mixer_dialog_parent_class)->finalize (object);
}

GvcMixerDialog *
gvc_mixer_dialog_new (GvcMixerControl *control)
{
        GObject *dialog;
        dialog = g_object_new (GVC_TYPE_MIXER_DIALOG,
                               "mixer-control", control,
                               NULL);
        return GVC_MIXER_DIALOG (dialog);
}

enum {
        PAGE_OUTPUT,
        PAGE_INPUT,
        PAGE_EVENTS,
        PAGE_APPLICATIONS
};

gboolean
gvc_mixer_dialog_set_page (GvcMixerDialog *self,
                           const char     *page)
{
        guint num;

        g_return_val_if_fail (self != NULL, FALSE);

        num = PAGE_OUTPUT;

        if (g_str_equal (page, "effects"))
                num = PAGE_EVENTS;
        else if (g_str_equal (page, "input"))
                num = PAGE_INPUT;
        else if (g_str_equal (page, "output"))
                num = PAGE_OUTPUT;
        else if (g_str_equal (page, "applications"))
                num = PAGE_APPLICATIONS;

        gtk_notebook_set_current_page (GTK_NOTEBOOK (self->priv->notebook), num);

        return TRUE;
}
