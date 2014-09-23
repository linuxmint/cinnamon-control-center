/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2011-2012 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU General Public License Version 2
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
 */

#include "config.h"

#include <glib-object.h>
#include <glib/gi18n.h>

#include <netinet/ether.h>

#include <nm-client.h>
#include <nm-utils.h>
#include <nm-device.h>
#include <nm-device-wifi.h>
#include <nm-device-ethernet.h>
#include <nm-setting-wireless-security.h>
#include <nm-remote-connection.h>
#include <nm-setting-wireless.h>

#include "../list-box-helper.h"
#include "network-dialogs.h"
#include "panel-common.h"

#include "connection-editor/net-connection-editor.h"
#include "net-device-wifi.h"

#define NET_DEVICE_WIFI_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), NET_TYPE_DEVICE_WIFI, NetDeviceWifiPrivate))

typedef enum {
  NM_AP_SEC_UNKNOWN,
  NM_AP_SEC_NONE,
  NM_AP_SEC_WEP,
  NM_AP_SEC_WPA,
  NM_AP_SEC_WPA2
} NMAccessPointSecurity;

static void nm_device_wifi_refresh_ui (NetDeviceWifi *device_wifi);
static void show_wifi_list (NetDeviceWifi *device_wifi);
static void populate_ap_list (NetDeviceWifi *device_wifi);

struct _NetDeviceWifiPrivate
{
        GtkBuilder              *builder;
        GtkWidget               *details_dialog;
        GtkWidget               *hotspot_dialog;
        gboolean                 updating_device;
        gchar                   *selected_ssid_title;
        gchar                   *selected_connection_id;
        gchar                   *selected_ap_id;
};

G_DEFINE_TYPE (NetDeviceWifi, net_device_wifi, NET_TYPE_DEVICE)

enum {
        COLUMN_CONNECTION_ID,
        COLUMN_ACCESS_POINT_ID,
        COLUMN_TITLE,
        COLUMN_SORT,
        COLUMN_STRENGTH,
        COLUMN_MODE,
        COLUMN_SECURITY,
        COLUMN_ACTIVE,
        COLUMN_AP_IN_RANGE,
        COLUMN_AP_OUT_OF_RANGE,
        COLUMN_AP_IS_SAVED,
        COLUMN_LAST
};

static GtkWidget *
device_wifi_proxy_add_to_notebook (NetObject *object,
                                    GtkNotebook *notebook,
                                    GtkSizeGroup *heading_size_group)
{
        GtkWidget *widget;
        NetDeviceWifi *device_wifi = NET_DEVICE_WIFI (object);

        /* add widgets to size group */
        widget = GTK_WIDGET (gtk_builder_get_object (device_wifi->priv->builder,
                                                     "heading_ipv4"));
        gtk_size_group_add_widget (heading_size_group, widget);

        widget = GTK_WIDGET (gtk_builder_get_object (device_wifi->priv->builder,
                                                     "notebook_view"));
        gtk_notebook_append_page (notebook, widget, NULL);

        return widget;
}

static guint
get_access_point_security (NMAccessPoint *ap)
{
        NM80211ApFlags flags;
        NM80211ApSecurityFlags wpa_flags;
        NM80211ApSecurityFlags rsn_flags;
        guint type;

        flags = nm_access_point_get_flags (ap);
        wpa_flags = nm_access_point_get_wpa_flags (ap);
        rsn_flags = nm_access_point_get_rsn_flags (ap);

        if (!(flags & NM_802_11_AP_FLAGS_PRIVACY) &&
            wpa_flags == NM_802_11_AP_SEC_NONE &&
            rsn_flags == NM_802_11_AP_SEC_NONE)
                type = NM_AP_SEC_NONE;
        else if ((flags & NM_802_11_AP_FLAGS_PRIVACY) &&
                 wpa_flags == NM_802_11_AP_SEC_NONE &&
                 rsn_flags == NM_802_11_AP_SEC_NONE)
                type = NM_AP_SEC_WEP;
        else if (!(flags & NM_802_11_AP_FLAGS_PRIVACY) &&
                 wpa_flags != NM_802_11_AP_SEC_NONE &&
                 rsn_flags != NM_802_11_AP_SEC_NONE)
                type = NM_AP_SEC_WPA;
        else
                type = NM_AP_SEC_WPA2;

        return type;
}

static GPtrArray *
panel_get_strongest_unique_aps (const GPtrArray *aps)
{
        const GByteArray *ssid;
        const GByteArray *ssid_tmp;
        GPtrArray *aps_unique = NULL;
        gboolean add_ap;
        guint i;
        guint j;
        NMAccessPoint *ap;
        NMAccessPoint *ap_tmp;

        /* we will have multiple entries for typical hotspots, just
         * filter to the one with the strongest signal */
        aps_unique = g_ptr_array_new_with_free_func ((GDestroyNotify) g_object_unref);
        if (aps != NULL)
                for (i = 0; i < aps->len; i++) {
                        ap = NM_ACCESS_POINT (g_ptr_array_index (aps, i));

                        /* Hidden SSIDs don't get shown in the list */
                        ssid = nm_access_point_get_ssid (ap);
                        if (!ssid)
                                continue;

                        add_ap = TRUE;

                        /* get already added list */
                        for (j=0; j<aps_unique->len; j++) {
                                ap_tmp = NM_ACCESS_POINT (g_ptr_array_index (aps_unique, j));
                                ssid_tmp = nm_access_point_get_ssid (ap_tmp);
                                g_assert (ssid_tmp);

                                /* is this the same type and data? */
                                if (nm_utils_same_ssid (ssid, ssid_tmp, TRUE)) {

                                        g_debug ("found duplicate: %s",
                                                 nm_utils_escape_ssid (ssid_tmp->data,
                                                                       ssid_tmp->len));

                                        /* the new access point is stronger */
                                        if (nm_access_point_get_strength (ap) >
                                            nm_access_point_get_strength (ap_tmp)) {
                                                g_debug ("removing %s",
                                                         nm_utils_escape_ssid (ssid_tmp->data,
                                                                               ssid_tmp->len));
                                                g_ptr_array_remove (aps_unique, ap_tmp);
                                                add_ap = TRUE;
                                        } else {
                                                add_ap = FALSE;
                                        }

                                        break;
                                }
                        }
                        if (add_ap) {
                                g_debug ("adding %s",
                                         nm_utils_escape_ssid (ssid->data,
                                                               ssid->len));
                                g_ptr_array_add (aps_unique, g_object_ref (ap));
                        }
                }
        return aps_unique;
}

static gchar *
get_ap_security_string (NMAccessPoint *ap)
{
        NM80211ApSecurityFlags wpa_flags, rsn_flags;
        NM80211ApFlags flags;
        GString *str;

        flags = nm_access_point_get_flags (ap);
        wpa_flags = nm_access_point_get_wpa_flags (ap);
        rsn_flags = nm_access_point_get_rsn_flags (ap);

        str = g_string_new ("");
        if ((flags & NM_802_11_AP_FLAGS_PRIVACY) &&
            (wpa_flags == NM_802_11_AP_SEC_NONE) &&
            (rsn_flags == NM_802_11_AP_SEC_NONE)) {
                /* TRANSLATORS: this WEP WiFi security */
                g_string_append_printf (str, "%s, ", _("WEP"));
        }
        if (wpa_flags != NM_802_11_AP_SEC_NONE) {
                /* TRANSLATORS: this WPA WiFi security */
                g_string_append_printf (str, "%s, ", _("WPA"));
        }
        if (rsn_flags != NM_802_11_AP_SEC_NONE) {
                /* TRANSLATORS: this WPA WiFi security */
                g_string_append_printf (str, "%s, ", _("WPA2"));
        }
        if ((wpa_flags & NM_802_11_AP_SEC_KEY_MGMT_802_1X) ||
            (rsn_flags & NM_802_11_AP_SEC_KEY_MGMT_802_1X)) {
                /* TRANSLATORS: this Enterprise WiFi security */
                g_string_append_printf (str, "%s, ", _("Enterprise"));
        }
        if (str->len > 0)
                g_string_set_size (str, str->len - 2);
        else {
                g_string_append (str, C_("Wifi security", "None"));
        }
        return g_string_free (str, FALSE);
}

static void
wireless_enabled_toggled (NMClient       *client,
                          GParamSpec     *pspec,
                          NetDeviceWifi *device_wifi)
{
        gboolean enabled;
        GtkSwitch *sw;
        NMDevice *device;

        device = net_device_get_nm_device (NET_DEVICE (device_wifi));
        if (nm_device_get_device_type (device) != NM_DEVICE_TYPE_WIFI)
                return;

        enabled = nm_client_wireless_get_enabled (client);
        sw = GTK_SWITCH (gtk_builder_get_object (device_wifi->priv->builder,
                                                 "device_off_switch"));

        device_wifi->priv->updating_device = TRUE;
        gtk_switch_set_active (sw, enabled);
        device_wifi->priv->updating_device = FALSE;
}

static NMConnection *
find_connection_for_device (NetDeviceWifi *device_wifi,
                            NMDevice       *device)
{
        NetDevice *tmp;
        NMConnection *connection;
        NMRemoteSettings *remote_settings;
        NMClient *client;

        client = net_object_get_client (NET_OBJECT (device_wifi));
        remote_settings = net_object_get_remote_settings (NET_OBJECT (device_wifi));
        tmp = g_object_new (NET_TYPE_DEVICE,
                            "client", client,
                            "remote-settings", remote_settings,
                            "nm-device", device,
                            NULL);
        connection = net_device_get_find_connection (tmp);
        g_object_unref (tmp);
        return connection;
}

static gboolean
connection_is_shared (NMConnection *c)
{
        NMSettingIP4Config *s_ip4;

        s_ip4 = nm_connection_get_setting_ip4_config (c);
        if (g_strcmp0 (nm_setting_ip4_config_get_method (s_ip4),
                       NM_SETTING_IP4_CONFIG_METHOD_SHARED) != 0) {
                return FALSE;
        }

        return TRUE;
}

static gboolean
device_is_hotspot (NetDeviceWifi *device_wifi)
{
        NMConnection *c;
        NMDevice *device;

        device = net_device_get_nm_device (NET_DEVICE (device_wifi));
        c = find_connection_for_device (device_wifi, device);
        if (c == NULL)
                return FALSE;

        return connection_is_shared (c);
}

static const GByteArray *
device_get_hotspot_ssid (NetDeviceWifi *device_wifi,
                         NMDevice *device)
{
        NMConnection *c;
        NMSettingWireless *sw;

        c = find_connection_for_device (device_wifi, device);
        if (c == NULL) {
                return FALSE;
        }

        sw = nm_connection_get_setting_wireless (c);
        return nm_setting_wireless_get_ssid (sw);
}

static void
get_secrets_cb (NMRemoteConnection *c,
                GHashTable         *secrets,
                GError             *error,
                gpointer            data)
{
        NetDeviceWifi *device_wifi = data;
        NMSettingWireless *sw;

        sw = nm_connection_get_setting_wireless (NM_CONNECTION (c));

        nm_connection_update_secrets (NM_CONNECTION (c),
                                      nm_setting_wireless_get_security (sw),
                                      secrets, NULL);

        nm_device_wifi_refresh_ui (device_wifi);
}

static void
device_get_hotspot_security_details (NetDeviceWifi *device_wifi,
                                     NMDevice *device,
                                     gchar **secret,
                                     gchar **security)
{
        NMConnection *c;
        NMSettingWireless *sw;
        NMSettingWirelessSecurity *sws;
        const gchar *key_mgmt;
        const gchar *tmp_secret;
        const gchar *tmp_security;

        c = find_connection_for_device (device_wifi, device);
        if (c == NULL)
                return;

        sw = nm_connection_get_setting_wireless (c);
        sws = nm_connection_get_setting_wireless_security (c);
        if (sw == NULL || sws == NULL)
                return;

        tmp_secret = NULL;
        tmp_security = C_("Wifi security", "None");

        /* Key management values:
         * "none" = WEP
         * "wpa-none" = WPAv1 Ad-Hoc mode (not supported in NM >= 0.9.4)
         * "wpa-psk" = WPAv2 Ad-Hoc mode (eg IBSS RSN) and AP-mode WPA v1 and v2
         */
        key_mgmt = nm_setting_wireless_security_get_key_mgmt (sws);
        if (strcmp (key_mgmt, "none") == 0) {
                tmp_secret = nm_setting_wireless_security_get_wep_key (sws, 0);
                tmp_security = _("WEP");
        }
        else if (strcmp (key_mgmt, "wpa-none") == 0 ||
                 strcmp (key_mgmt, "wpa-psk") == 0) {
                tmp_secret = nm_setting_wireless_security_get_psk (sws);
                tmp_security = _("WPA");
        } else {
                g_warning ("unhandled security key-mgmt: %s", key_mgmt);
        }

        /* If we don't have secrets, request them from NM and bail.
         * We'll refresh the UI when secrets arrive.
         */
        if (tmp_secret == NULL) {
                nm_remote_connection_get_secrets ((NMRemoteConnection*)c,
                                                  nm_setting_wireless_get_security (sw),
                                                  get_secrets_cb,
                                                  device_wifi);
                return;
        }

        if (secret)
                *secret = g_strdup (tmp_secret);
        if (security)
                *security = g_strdup (tmp_security);
}

static void
nm_device_wifi_refresh_hotspot (NetDeviceWifi *device_wifi)
{
        const GByteArray *ssid;
        gchar *hotspot_secret = NULL;
        gchar *hotspot_security = NULL;
        gchar *hotspot_ssid = NULL;
        NMDevice *nm_device;

        /* refresh hotspot ui */
        nm_device = net_device_get_nm_device (NET_DEVICE (device_wifi));
        ssid = device_get_hotspot_ssid (device_wifi, nm_device);
        if (ssid)
                hotspot_ssid = nm_utils_ssid_to_utf8 (ssid);
        device_get_hotspot_security_details (device_wifi,
                                             nm_device,
                                             &hotspot_secret,
                                             &hotspot_security);

        panel_set_device_widget_details (device_wifi->priv->builder,
                                         "hotspot_network_name",
                                         hotspot_ssid);
        panel_set_device_widget_details (device_wifi->priv->builder,
                                         "hotspot_security_key",
                                         hotspot_secret);
        panel_set_device_widget_details (device_wifi->priv->builder,
                                         "hotspot_security",
                                         hotspot_security);
        panel_set_device_widget_details (device_wifi->priv->builder,
                                         "hotspot_connected",
                                         NULL);

        g_free (hotspot_secret);
        g_free (hotspot_security);
        g_free (hotspot_ssid);
}

static void
update_last_used (NetDeviceWifi *device_wifi, NMConnection *connection)
{
        gchar *last_used = NULL;
        GDateTime *now = NULL;
        GDateTime *then = NULL;
        gint days;
        GTimeSpan diff;
        guint64 timestamp;
        NMSettingConnection *s_con;

        s_con = nm_connection_get_setting_connection (connection);
        if (s_con == NULL)
                goto out;
        timestamp = nm_setting_connection_get_timestamp (s_con);
        if (timestamp == 0) {
                last_used = g_strdup (_("never"));
                goto out;
        }

        /* calculate the amount of time that has elapsed */
        now = g_date_time_new_now_utc ();
        then = g_date_time_new_from_unix_utc (timestamp);
        diff = g_date_time_difference  (now, then);
        days = diff / G_TIME_SPAN_DAY;
        if (days == 0)
                last_used = g_strdup (_("today"));
        else if (days == 1)
                last_used = g_strdup (_("yesterday"));
        else
                last_used = g_strdup_printf (ngettext ("%i day ago", "%i days ago", days), days);
out:
        panel_set_device_widget_details (device_wifi->priv->builder,
                                         "last_used",
                                         last_used);
        if (now != NULL)
                g_date_time_unref (now);
        if (then != NULL)
                g_date_time_unref (then);
        g_free (last_used);
}

static void
nm_device_wifi_refresh_ui (NetDeviceWifi *device_wifi)
{
        const gchar *str;
        gboolean is_hotspot;
        gchar *str_tmp = NULL;
        gint strength = 0;
        guint speed = 0;
        NMAccessPoint *active_ap;
        NMDevice *nm_device;
        NMDeviceState state;
        NMClient *client;
        NMAccessPoint *ap;
        NMConnection *connection;
        NetDeviceWifiPrivate *priv = device_wifi->priv;
        GtkWidget *dialog;

        is_hotspot = device_is_hotspot (device_wifi);
        if (is_hotspot) {
                nm_device_wifi_refresh_hotspot (device_wifi);
                return;
        }

        nm_device = net_device_get_nm_device (NET_DEVICE (device_wifi));

        dialog = device_wifi->priv->details_dialog;

        ap = g_object_get_data (G_OBJECT (dialog), "ap");
        connection = g_object_get_data (G_OBJECT (dialog), "connection");

        active_ap = nm_device_wifi_get_active_access_point (NM_DEVICE_WIFI (nm_device));

        state = nm_device_get_state (nm_device);

        /* keep this in sync with the signal handler setup in cc_network_panel_init */
        client = net_object_get_client (NET_OBJECT (device_wifi));
        wireless_enabled_toggled (client, NULL, device_wifi);

        if (ap != active_ap)
                speed = 0;
        else if (state != NM_DEVICE_STATE_UNAVAILABLE)
                speed = nm_device_wifi_get_bitrate (NM_DEVICE_WIFI (nm_device));
        speed /= 1000;
        if (speed > 0) {
                /* Translators: network device speed */
                str_tmp = g_strdup_printf (_("%d Mb/s"), speed);
        }
        panel_set_device_widget_details (device_wifi->priv->builder,
                                         "speed",
                                         str_tmp);

        /* device MAC */
        str = nm_device_wifi_get_hw_address (NM_DEVICE_WIFI (nm_device));
        panel_set_device_widget_details (device_wifi->priv->builder,
                                         "mac",
                                         str);
        /* security */
        if (ap != active_ap)
                str_tmp = NULL;
        else if (active_ap != NULL)
                str_tmp = get_ap_security_string (active_ap);
        panel_set_device_widget_details (device_wifi->priv->builder,
                                         "security",
                                         str_tmp);
        g_free (str_tmp);

        /* signal strength */
        if (ap != NULL)
                strength = nm_access_point_get_strength (ap);
        else
                strength = 0;
        if (strength <= 0)
                str = NULL;
        else if (strength < 20)
                str = C_("Signal strength", "Terrible");
        else if (strength < 40)
                str = C_("Signal strength", "Weak");
        else if (strength < 50)
                str = C_("Signal strength", "Ok");
        else if (strength < 80)
                str = C_("Signal strength", "Good");
        else
                str = C_("Signal strength", "Excellent");
        panel_set_device_widget_details (device_wifi->priv->builder,
                                         "strength",
                                         str);

        /* device MAC */
        if (ap != active_ap)
                str = NULL;
        else
                str = nm_device_wifi_get_hw_address (NM_DEVICE_WIFI (nm_device));
        panel_set_device_widget_details (priv->builder, "mac", str);

        /* set IP entries */
        if (ap != active_ap)
                panel_unset_device_widgets (priv->builder);
        else
                panel_set_device_widgets (priv->builder, nm_device);

        if (ap != active_ap && connection)
                update_last_used (device_wifi, connection);
        else
                panel_set_device_widget_details (priv->builder, "last_used", NULL);

        panel_set_device_status (priv->builder, "heading_status", nm_device, NULL);

        /* update list of APs */
        populate_ap_list (device_wifi);
}

static void
device_wifi_refresh (NetObject *object)
{
        NetDeviceWifi *device_wifi = NET_DEVICE_WIFI (object);
        nm_device_wifi_refresh_ui (device_wifi);
}

static void
device_off_toggled (GtkSwitch *sw,
                    GParamSpec *pspec,
                    NetDeviceWifi *device_wifi)
{
        NMClient *client;
        gboolean active;

        if (device_wifi->priv->updating_device)
                return;

        client = net_object_get_client (NET_OBJECT (device_wifi));
        active = gtk_switch_get_active (sw);
        nm_client_wireless_set_enabled (client, active);
}

static void
connect_to_hidden_network (NetDeviceWifi *device_wifi)
{
        NMRemoteSettings *remote_settings;
        NMClient *client;
        CcNetworkPanel *panel;

        remote_settings = net_object_get_remote_settings (NET_OBJECT (device_wifi));
        client = net_object_get_client (NET_OBJECT (device_wifi));
        panel = net_object_get_panel (NET_OBJECT (device_wifi));
        cc_network_panel_connect_to_hidden_network (panel, client, remote_settings);
}

static void
connection_add_or_activate_cb (NMActiveConnection *connection,
                               GError *error,
                               NetDeviceWifi *device_wifi)
{
        if (connection == NULL) {
                /* failed to activate */
                g_debug ("Failed to add and/or activate connection '%d': %s",
                         error->code,
                         error->message);
                nm_device_wifi_refresh_ui (device_wifi);
        }
}

static void
connection_add_activate_cb (NMClient *client,
                            NMActiveConnection *connection,
                            const char *path,
                            GError *error,
                            gpointer user_data)
{
        connection_add_or_activate_cb (connection, error, user_data);
}

static void
connection_activate_cb (NMClient *client,
                        NMActiveConnection *connection,
                        GError *error,
                        gpointer user_data)
{
        connection_add_or_activate_cb (connection, error, user_data);
}

static gboolean
is_8021x (NMDevice   *device,
          const char *ap_object_path)
{
        NM80211ApSecurityFlags wpa_flags, rsn_flags;
        NMAccessPoint *ap;

        ap = nm_device_wifi_get_access_point_by_path (NM_DEVICE_WIFI (device),
                                                      ap_object_path);
        if (!ap)
                return FALSE;

        rsn_flags = nm_access_point_get_rsn_flags (ap);
        if (rsn_flags & NM_802_11_AP_SEC_KEY_MGMT_802_1X)
                return TRUE;

        wpa_flags = nm_access_point_get_wpa_flags (ap);
        if (wpa_flags & NM_802_11_AP_SEC_KEY_MGMT_802_1X)
                return TRUE;
        return FALSE;
}

static void
wireless_try_to_connect (NetDeviceWifi *device_wifi,
                         const gchar *ssid_target,
                         const gchar *ap_object_path)
{
        const GByteArray *ssid;
        const gchar *ssid_tmp;
        GSList *list, *l;
        NMConnection *connection_activate = NULL;
        NMDevice *device;
        NMSettingWireless *setting_wireless;
        NMClient *client;

        if (device_wifi->priv->updating_device)
                goto out;

        if (ap_object_path == NULL || ap_object_path[0] == 0)
                goto out;

        device = net_device_get_nm_device (NET_DEVICE (device_wifi));
        if (device == NULL)
                goto out;

        g_debug ("try to connect to WIFI network %s [%s]",
                 ssid_target, ap_object_path);

        /* look for an existing connection we can use */
        list = net_device_get_valid_connections (NET_DEVICE (device_wifi));
        g_debug ("%i suitable remote connections to check", g_slist_length (list));
        for (l = list; l; l = g_slist_next (l)) {
                NMConnection *connection;

                connection = NM_CONNECTION (l->data);
                setting_wireless = nm_connection_get_setting_wireless (connection);
                if (!NM_IS_SETTING_WIRELESS (setting_wireless))
                        continue;
                ssid = nm_setting_wireless_get_ssid (setting_wireless);
                if (ssid == NULL)
                        continue;
                ssid_tmp = nm_utils_escape_ssid (ssid->data, ssid->len);
                if (g_strcmp0 (ssid_target, ssid_tmp) == 0) {
                        g_debug ("we found an existing connection %s to activate!",
                                 nm_connection_get_id (connection));
                        connection_activate = connection;
                        break;
                }
        }

        g_slist_free (list);

        /* activate the connection */
        client = net_object_get_client (NET_OBJECT (device_wifi));
        if (connection_activate != NULL) {
                nm_client_activate_connection (client,
                                               connection_activate,
                                               device, NULL,
                                               connection_activate_cb, device_wifi);
                goto out;
        }

        /* create one, as it's missing */
        g_debug ("no existing connection found for %s, creating", ssid_target);

        if (!is_8021x (device, ap_object_path)) {
                g_debug ("no existing connection found for %s, creating and activating one", ssid_target);
                nm_client_add_and_activate_connection (client,
                                                       NULL,
                                                       device, ap_object_path,
                                                       connection_add_activate_cb, device_wifi);
        } else {
                CcNetworkPanel *panel;
                GVariantBuilder *builder;
                GVariant *parameters;

                g_debug ("no existing connection found for %s, creating", ssid_target);
                builder = g_variant_builder_new (G_VARIANT_TYPE ("av"));
                g_variant_builder_add (builder, "v", g_variant_new_string ("connect-8021x-wifi"));
                g_variant_builder_add (builder, "v", g_variant_new_string (nm_object_get_path (NM_OBJECT (device))));
                g_variant_builder_add (builder, "v", g_variant_new_string (ap_object_path));
                parameters = g_variant_new ("av", builder);

                panel = net_object_get_panel (NET_OBJECT (device_wifi));
                g_object_set (G_OBJECT (panel), "parameters", parameters, NULL);

                g_variant_builder_unref (builder);
        }
out:
        return;
}

static GByteArray *
ssid_to_byte_array (const gchar *ssid)
{
        guint32 len;
        GByteArray *ba;

        len = strlen (ssid);
        ba = g_byte_array_sized_new (len);
        g_byte_array_append (ba, (guchar *)ssid, len);

        return ba;
}

static gchar *
get_hostname (void)
{
        GDBusConnection *bus;
        GVariant *res;
        GVariant *inner;
        gchar *str;
        GError *error;

        error = NULL;
        bus = g_bus_get_sync (G_BUS_TYPE_SYSTEM, NULL, &error);
        if (bus == NULL) {
                g_warning ("Failed to get system bus connection: %s", error->message);
                g_error_free (error);

                return NULL;
        }
        res = g_dbus_connection_call_sync (bus,
                                           "org.freedesktop.hostname1",
                                           "/org/freedesktop/hostname1",
                                           "org.freedesktop.DBus.Properties",
                                           "Get",
                                           g_variant_new ("(ss)",
                                                          "org.freedesktop.hostname1",
                                                          "PrettyHostname"),
                                           (GVariantType*)"(v)",
                                           G_DBUS_CALL_FLAGS_NONE,
                                           -1,
                                           NULL,
                                           &error);
        g_object_unref (bus);

        if (res == NULL) {
                g_warning ("Getting pretty hostname failed: %s", error->message);
                g_error_free (error);
        }

        str = NULL;

        if (res != NULL) {
                g_variant_get (res, "(v)", &inner);
                str = g_variant_dup_string (inner, NULL);
                g_variant_unref (res);
        }

        if (str == NULL || *str == '\0') {
                str = g_strdup (g_get_host_name ());
        }

        if (str == NULL || *str == '\0') {
                str = g_strdup ("GNOME");
	}

        return str;
}

static GByteArray *
generate_ssid_for_hotspot (NetDeviceWifi *device_wifi)
{
        GByteArray *ssid_array;
        gchar *ssid;

        ssid = get_hostname ();
        ssid_array = ssid_to_byte_array (ssid);
        g_free (ssid);

        return ssid_array;
}

#define WPA_PASSKEY_SIZE 8
static void
set_wpa_key (NMSettingWirelessSecurity *sws)
{
        /* generate a 8-chars ASCII WPA key */
        char key[WPA_PASSKEY_SIZE + 1];
        guint i;

        for (i = 0; i < WPA_PASSKEY_SIZE; i++) {
                gint c;
                c = g_random_int_range (33, 126);
                /* too many non alphanumeric characters are hard to remember for humans */
                while (!g_ascii_isalnum (c))
                        c = g_random_int_range (33, 126);

                key[i] = (gchar) c;
        }
        key[WPA_PASSKEY_SIZE] = '\0';

        g_object_set (sws,
                      "key-mgmt", "wpa-psk",
                      "psk", key,
                      NULL);
}

static void
set_wep_key (NMSettingWirelessSecurity *sws)
{
        gchar key[11];
        gint i;
        const gchar *hexdigits = "0123456789abcdef";

        /* generate a 10-digit hex WEP key */
        for (i = 0; i < 10; i++) {
                gint digit;
                digit = g_random_int_range (0, 16);
                key[i] = hexdigits[digit];
        }
        key[10] = 0;

        g_object_set (sws,
                      "key-mgmt", "none",
                      "wep-key0", key,
                      "wep-key-type", NM_WEP_KEY_TYPE_KEY,
                      NULL);
}

static gboolean
is_hotspot_connection (NMConnection *connection)
{
        NMSettingConnection *sc;
        NMSettingWireless *sw;
        NMSettingIP4Config *sip;

        sc = nm_connection_get_setting_connection (connection);
        if (g_strcmp0 (nm_setting_connection_get_connection_type (sc), "802-11-wireless") != 0) {
                return FALSE;
        }
        sw = nm_connection_get_setting_wireless (connection);
        if (g_strcmp0 (nm_setting_wireless_get_mode (sw), "adhoc") != 0 &&
            g_strcmp0 (nm_setting_wireless_get_mode (sw), "ap") != 0) {
                return FALSE;
        }
        if (g_strcmp0 (nm_setting_wireless_get_security (sw), "802-11-wireless-security") != 0) {
                return FALSE;
        }
        sip = nm_connection_get_setting_ip4_config (connection);
        if (g_strcmp0 (nm_setting_ip4_config_get_method (sip), "shared") != 0) {
                return FALSE;
        }

        return TRUE;
}

static void
show_hotspot_ui (NetDeviceWifi *device_wifi)
{
        GtkWidget *widget;
        GtkSwitch *sw;

        /* show hotspot tab */
        widget = GTK_WIDGET (gtk_builder_get_object (device_wifi->priv->builder, "notebook_view"));
        gtk_notebook_set_current_page (GTK_NOTEBOOK (widget), 1);

        /* force switch to on as this succeeded */
        sw = GTK_SWITCH (gtk_builder_get_object (device_wifi->priv->builder,
                                                 "switch_hotspot_off"));
        device_wifi->priv->updating_device = TRUE;
        gtk_switch_set_active (sw, TRUE);
        device_wifi->priv->updating_device = FALSE;
}

static void
activate_cb (NMClient           *client,
             NMActiveConnection *connection,
             GError             *error,
             NetDeviceWifi     *device_wifi)
{
        if (error != NULL) {
                g_warning ("Failed to add new connection: (%d) %s",
                           error->code,
                           error->message);
                return;
        }

        /* show hotspot tab */
        nm_device_wifi_refresh_ui (device_wifi);
        show_hotspot_ui (device_wifi);
}

static void
activate_new_cb (NMClient           *client,
                 NMActiveConnection *connection,
                 const gchar        *path,
                 GError             *error,
                 NetDeviceWifi     *device_wifi)
{
        activate_cb (client, connection, error, device_wifi);
}

static void
start_shared_connection (NetDeviceWifi *device_wifi)
{
        NMConnection *c;
        NMConnection *tmp;
        NMSettingConnection *sc;
        NMSettingWireless *sw;
        NMSettingIP4Config *sip;
        NMSettingWirelessSecurity *sws;
        NMDevice *device;
        GByteArray *ssid_array;
        const gchar *str_mac;
        struct ether_addr *bin_mac;
        GSList *connections;
        GSList *l;
        NMClient *client;
        const char *mode;
        NMDeviceWifiCapabilities caps;

        device = net_device_get_nm_device (NET_DEVICE (device_wifi));
        g_assert (nm_device_get_device_type (device) == NM_DEVICE_TYPE_WIFI);

        connections = net_device_get_valid_connections (NET_DEVICE (device_wifi));
        c = NULL;
        for (l = connections; l; l = l->next) {
                tmp = l->data;
                if (is_hotspot_connection (tmp)) {
                        c = tmp;
                        break;
                }
        }
        g_slist_free (connections);

        client = net_object_get_client (NET_OBJECT (device_wifi));
        if (c != NULL) {
                g_debug ("activate existing hotspot connection\n");
                nm_client_activate_connection (client,
                                               c,
                                               device,
                                               NULL,
                                               (NMClientActivateFn)activate_cb,
                                               device_wifi);
                return;
        }

        g_debug ("create new hotspot connection\n");
        c = nm_connection_new ();

        sc = (NMSettingConnection *)nm_setting_connection_new ();
        g_object_set (sc,
                      "type", "802-11-wireless",
                      "id", "Hotspot",
                      "autoconnect", FALSE,
                      NULL);
        nm_connection_add_setting (c, (NMSetting *)sc);

        sw = (NMSettingWireless *)nm_setting_wireless_new ();

	/* Use real AP mode if the device supports it */
        caps = nm_device_wifi_get_capabilities (NM_DEVICE_WIFI (device));
        if (caps & NM_WIFI_DEVICE_CAP_AP)
		mode = NM_SETTING_WIRELESS_MODE_AP;
        else
                mode = NM_SETTING_WIRELESS_MODE_ADHOC;

        g_object_set (sw,
                      "mode", mode,
                      "security", "802-11-wireless-security",
                      NULL);

        str_mac = nm_device_wifi_get_permanent_hw_address (NM_DEVICE_WIFI (device));
        bin_mac = ether_aton (str_mac);
        if (bin_mac) {
                GByteArray *hw_address;

                hw_address = g_byte_array_sized_new (ETH_ALEN);
                g_byte_array_append (hw_address, bin_mac->ether_addr_octet, ETH_ALEN);
                g_object_set (sw,
                              "mac-address", hw_address,
                              NULL);
                g_byte_array_unref (hw_address);
        }
        nm_connection_add_setting (c, (NMSetting *)sw);

        sip = (NMSettingIP4Config*) nm_setting_ip4_config_new ();
        g_object_set (sip, "method", "shared", NULL);
        nm_connection_add_setting (c, (NMSetting *)sip);

        ssid_array = generate_ssid_for_hotspot (device_wifi);
        g_object_set (sw,
                      "ssid", ssid_array,
                      NULL);
        g_byte_array_unref (ssid_array);

        sws = (NMSettingWirelessSecurity*) nm_setting_wireless_security_new ();

        if (g_strcmp0 (mode, NM_SETTING_WIRELESS_MODE_AP) == 0) {
                if (caps & NM_WIFI_DEVICE_CAP_RSN) {
                        set_wpa_key (sws);
                        nm_setting_wireless_security_add_proto (sws, "rsn");
                        nm_setting_wireless_security_add_pairwise (sws, "ccmp");
                        nm_setting_wireless_security_add_group (sws, "ccmp");
                } else if (caps & NM_WIFI_DEVICE_CAP_WPA) {
                        set_wpa_key (sws);
                        nm_setting_wireless_security_add_proto (sws, "wpa");
                        nm_setting_wireless_security_add_pairwise (sws, "tkip");
                        nm_setting_wireless_security_add_group (sws, "tkip");
                } else {
                        set_wep_key (sws);
                }
        } else {
                set_wep_key (sws);
        }

        nm_connection_add_setting (c, (NMSetting *)sws);

        nm_client_add_and_activate_connection (client,
                                               c,
                                               device,
                                               NULL,
                                               (NMClientAddActivateFn)activate_new_cb,
                                               device_wifi);

        g_object_unref (c);
}

static void
start_hotspot_response_cb (GtkWidget *dialog, gint response, NetDeviceWifi *device_wifi)
{
        if (response == GTK_RESPONSE_OK) {
                start_shared_connection (device_wifi);
        }
        gtk_widget_hide (dialog);
}

static void
start_hotspot (GtkButton *button, NetDeviceWifi *device_wifi)
{
        NMDevice *device;
        const GPtrArray *connections;
        gchar *active_ssid;
        NMClient *client;
        GtkWidget *dialog;
        GtkWidget *window;
        GtkWidget *widget;
        GString *str;

        active_ssid = NULL;

        client = net_object_get_client (NET_OBJECT (device_wifi));
        device = net_device_get_nm_device (NET_DEVICE (device_wifi));
        connections = nm_client_get_active_connections (client);
        if (connections) {
                gint i;
                for (i = 0; i < connections->len; i++) {
                        NMActiveConnection *c;
                        const GPtrArray *devices;
                        c = (NMActiveConnection *)connections->pdata[i];
                        devices = nm_active_connection_get_devices (c);
                        if (devices && devices->pdata[0] == device) {
                                NMAccessPoint *ap;
                                ap = nm_device_wifi_get_active_access_point (NM_DEVICE_WIFI (device));
                                active_ssid = nm_utils_ssid_to_utf8 (nm_access_point_get_ssid (ap));
                                break;
                        }
                }
        }

        window = gtk_widget_get_toplevel (GTK_WIDGET (button));

        dialog = device_wifi->priv->hotspot_dialog;
        gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (window));

        str = g_string_new (_("If you have a connection to the Internet other than wireless, you can set up a wireless hotspot to share the connection with others."));
        g_string_append (str, "\n\n");

        if (active_ssid) {
                g_string_append_printf (str, _("Switching on the wireless hotspot will disconnect you from <b>%s</b>."), active_ssid);
                g_string_append (str, " ");
        }

        g_string_append (str, _("It is not possible to access the Internet through your wireless while the hotspot is active."));

        widget = GTK_WIDGET (gtk_builder_get_object (device_wifi->priv->builder, "hotspot-dialog-content"));
        gtk_label_set_markup (GTK_LABEL (widget), str->str);
        g_string_free (str, TRUE);

        g_signal_connect (dialog, "response",
                          G_CALLBACK (start_hotspot_response_cb), device_wifi);
        g_signal_connect (dialog, "delete-event",
                        G_CALLBACK (gtk_widget_hide_on_delete), NULL);

        gtk_window_present (GTK_WINDOW (dialog));
        g_free (active_ssid);
}

static void
stop_shared_connection (NetDeviceWifi *device_wifi)
{
        const GPtrArray *connections;
        const GPtrArray *devices;
        NMDevice *device;
        gint i;
        NMActiveConnection *c;
        NMClient *client;

        device = net_device_get_nm_device (NET_DEVICE (device_wifi));
        client = net_object_get_client (NET_OBJECT (device_wifi));
        connections = nm_client_get_active_connections (client);
        for (i = 0; connections && i < connections->len; i++) {
                c = (NMActiveConnection *)connections->pdata[i];

                devices = nm_active_connection_get_devices (c);
                if (devices && devices->pdata[0] == device) {
                        nm_client_deactivate_connection (client, c);
                        break;
                }
        }

        nm_device_wifi_refresh_ui (device_wifi);
        show_wifi_list (device_wifi);
}

static void
stop_hotspot_response_cb (GtkWidget *dialog, gint response, NetDeviceWifi *device_wifi)
{
        if (response == GTK_RESPONSE_OK) {
                stop_shared_connection (device_wifi);
        } else {
                GtkWidget *sw;

                sw = GTK_WIDGET (gtk_builder_get_object (device_wifi->priv->builder,
                                                         "switch_hotspot_off"));
                device_wifi->priv->updating_device = TRUE;
                gtk_switch_set_active (GTK_SWITCH (sw), TRUE);
                device_wifi->priv->updating_device = FALSE;
        }
        gtk_widget_destroy (dialog);
}

static void
switch_hotspot_changed_cb (GtkSwitch *sw,
                           GParamSpec *pspec,
                           NetDeviceWifi *device_wifi)
{
        GtkWidget *dialog;
        GtkWidget *window;
        CcNetworkPanel *panel;

        if (device_wifi->priv->updating_device)
                return;

        panel = net_object_get_panel (NET_OBJECT (device_wifi));
        window = gtk_widget_get_toplevel (GTK_WIDGET (panel));
        dialog = gtk_message_dialog_new (GTK_WINDOW (window),
                                         GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                         GTK_MESSAGE_OTHER,
                                         GTK_BUTTONS_NONE,
                                         _("Stop hotspot and disconnect any users?"));
        gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                                _("_Cancel"), GTK_RESPONSE_CANCEL,
                                _("_Stop Hotspot"), GTK_RESPONSE_OK,
                                NULL);
        g_signal_connect (dialog, "response",
                          G_CALLBACK (stop_hotspot_response_cb), device_wifi);
        gtk_window_present (GTK_WINDOW (dialog));
}

static void
show_wifi_list (NetDeviceWifi *device_wifi)
{
        GtkWidget *widget;
        widget = GTK_WIDGET (gtk_builder_get_object (device_wifi->priv->builder, "notebook_view"));
        gtk_notebook_set_current_page (GTK_NOTEBOOK (widget), 0);
}

static void
remote_settings_read_cb (NMRemoteSettings *remote_settings,
                         NetDeviceWifi *device_wifi)
{
        gboolean is_hotspot;

        populate_ap_list (device_wifi);

        /* go straight to the hotspot UI */
        is_hotspot = device_is_hotspot (device_wifi);
        if (is_hotspot) {
                nm_device_wifi_refresh_hotspot (device_wifi);
                show_hotspot_ui (device_wifi);
        }
}

static void
switch_page_cb (GtkNotebook   *notebook,
                GtkWidget     *page,
                guint          page_num,
                NetDeviceWifi *device_wifi)
{
        GtkWidget *widget;

        if (page_num == 1) {
                widget = GTK_WIDGET (gtk_builder_get_object (device_wifi->priv->builder,
                                                             "button_back1"));
                gtk_widget_grab_focus (widget);
        }
}

static void
net_device_wifi_constructed (GObject *object)
{
        NetDeviceWifi *device_wifi = NET_DEVICE_WIFI (object);
        NMClient *client;
        NMRemoteSettings *remote_settings;
        NMClientPermissionResult perm;
        NMDevice *nm_device;
        NMDeviceWifiCapabilities caps;
        GtkWidget *widget;

        G_OBJECT_CLASS (net_device_wifi_parent_class)->constructed (object);

        client = net_object_get_client (NET_OBJECT (device_wifi));
        g_signal_connect (client, "notify::wireless-enabled",
                          G_CALLBACK (wireless_enabled_toggled), device_wifi);

        nm_device = net_device_get_nm_device (NET_DEVICE (device_wifi));

        /* only enable the button if the user can create a hotspot */
        widget = GTK_WIDGET (gtk_builder_get_object (device_wifi->priv->builder,
                                                     "start_hotspot_button"));
        perm = nm_client_get_permission_result (client, NM_CLIENT_PERMISSION_WIFI_SHARE_OPEN);
        caps = nm_device_wifi_get_capabilities (NM_DEVICE_WIFI (nm_device));
        if (perm != NM_CLIENT_PERMISSION_RESULT_YES &&
            perm != NM_CLIENT_PERMISSION_RESULT_AUTH) {
                gtk_widget_set_tooltip_text (widget, _("System policy prohibits use as a Hotspot"));
                gtk_widget_set_sensitive (widget, FALSE);
        } else if (!(caps & (NM_WIFI_DEVICE_CAP_AP | NM_WIFI_DEVICE_CAP_ADHOC))) {
                gtk_widget_set_tooltip_text (widget, _("Wireless device does not support Hotspot mode"));
                gtk_widget_set_sensitive (widget, FALSE);
        } else
                gtk_widget_set_sensitive (widget, TRUE);

        remote_settings = net_object_get_remote_settings (NET_OBJECT (device_wifi));
        g_signal_connect (remote_settings, "connections-read",
                          G_CALLBACK (remote_settings_read_cb), device_wifi);

        widget = GTK_WIDGET (gtk_builder_get_object (device_wifi->priv->builder, "heading_list"));
        g_object_bind_property (device_wifi, "title", widget, "label", 0);

        nm_device_wifi_refresh_ui (device_wifi);
}

static void
net_device_wifi_finalize (GObject *object)
{
        NetDeviceWifi *device_wifi = NET_DEVICE_WIFI (object);
        NetDeviceWifiPrivate *priv = device_wifi->priv;

        g_clear_pointer (&priv->details_dialog, gtk_widget_destroy);
        g_clear_pointer (&priv->hotspot_dialog, gtk_widget_destroy);
        g_object_unref (priv->builder);
        g_free (priv->selected_ssid_title);
        g_free (priv->selected_connection_id);
        g_free (priv->selected_ap_id);

        G_OBJECT_CLASS (net_device_wifi_parent_class)->finalize (object);
}

static void
device_wifi_edit (NetObject *object)
{
        const gchar *uuid;
        gchar *cmdline;
        GError *error = NULL;
        NetDeviceWifi *device = NET_DEVICE_WIFI (object);
        NMRemoteSettings *settings;
        NMRemoteConnection *connection;

        settings = net_object_get_remote_settings (object);
        connection = nm_remote_settings_get_connection_by_path (settings, device->priv->selected_connection_id);
        if (connection == NULL) {
                g_warning ("failed to get remote connection");
                return;
        }
        uuid = nm_connection_get_uuid (NM_CONNECTION (connection));
        cmdline = g_strdup_printf ("nm-connection-editor --edit %s", uuid);
        g_debug ("Launching '%s'\n", cmdline);
        if (!g_spawn_command_line_async (cmdline, &error)) {
                g_warning ("Failed to launch nm-connection-editor: %s", error->message);
                g_error_free (error);
        }
        g_free (cmdline);
}

static void
net_device_wifi_class_init (NetDeviceWifiClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        NetObjectClass *parent_class = NET_OBJECT_CLASS (klass);

        object_class->finalize = net_device_wifi_finalize;
        object_class->constructed = net_device_wifi_constructed;
        parent_class->add_to_notebook = device_wifi_proxy_add_to_notebook;
        parent_class->refresh = device_wifi_refresh;
        parent_class->edit = device_wifi_edit;

        g_type_class_add_private (klass, sizeof (NetDeviceWifiPrivate));
}

static void
really_forgotten (NMRemoteConnection *connection,
                  GError             *error,
                  gpointer            user_data)
{
        NetDeviceWifi *device_wifi = NET_DEVICE_WIFI (user_data);

        if (error != NULL) {
                g_warning ("failed to delete connection %s: %s",
                           nm_object_get_path (NM_OBJECT (connection)),
                           error->message);
                return;
        }

        /* remove the entry from the list */
        populate_ap_list (device_wifi);
}

static void
really_forget (GtkDialog *dialog, gint response, gpointer data)
{
        GtkWidget *forget = data;
        GtkWidget *row;
        GList *rows;
        GList *r;
        NMRemoteConnection *connection;
        NetDeviceWifi *device_wifi;

        gtk_widget_destroy (GTK_WIDGET (dialog));

        if (response != GTK_RESPONSE_OK)
                return;

        device_wifi = NET_DEVICE_WIFI (g_object_get_data (G_OBJECT (forget), "net"));
        rows = g_object_steal_data (G_OBJECT (forget), "rows");
        for (r = rows; r; r = r->next) {
                row = r->data;
                connection = g_object_get_data (G_OBJECT (row), "connection");
                nm_remote_connection_delete (connection, really_forgotten, device_wifi);
                gtk_widget_destroy (row);
        }
        g_list_free (rows);
}

static void
forget_selected (GtkButton *forget, NetDeviceWifi *device_wifi)
{
        GtkWidget *window;
        GtkWidget *dialog;

        window = gtk_widget_get_toplevel (GTK_WIDGET (forget));
        dialog = gtk_message_dialog_new (GTK_WINDOW (window),
                                         GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                         GTK_MESSAGE_OTHER,
                                         GTK_BUTTONS_NONE,
                                         NULL);
        gtk_message_dialog_set_markup (GTK_MESSAGE_DIALOG (dialog),
                                       _("Network details for the selected networks, including passwords and any custom configuration will be lost."));

        gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                                _("_Cancel"), GTK_RESPONSE_CANCEL,
                                _("_Forget"), GTK_RESPONSE_OK,
                                NULL);
        g_signal_connect (dialog, "response",
                          G_CALLBACK (really_forget), forget);
        gtk_window_present (GTK_WINDOW (dialog));
}

static void
check_toggled (GtkToggleButton *check, GtkWidget *forget)
{
        gboolean active;
        GtkWidget *row;
        GList *rows;

        row = gtk_widget_get_ancestor (GTK_WIDGET (check), GTK_TYPE_LIST_BOX_ROW);
        active = gtk_toggle_button_get_active (check);
        rows = g_object_steal_data (G_OBJECT (forget), "rows");

        if (active) {
                rows = g_list_prepend (rows, row);
        } else {
                rows = g_list_remove (rows, row);
        }

        g_object_set_data_full (G_OBJECT (forget), "rows", rows, (GDestroyNotify)g_list_free);
        gtk_widget_set_sensitive (forget, rows != NULL);
}

static void
update_forget (GtkWidget *forget,
               gpointer   row)
{
        GList *rows;

        rows = g_object_steal_data (G_OBJECT (forget), "rows");
        rows = g_list_remove (rows, row);
        g_object_set_data_full (G_OBJECT (forget), "rows", rows, (GDestroyNotify)g_list_free);
        gtk_widget_set_sensitive (forget, rows != NULL);
}

static void
make_row (GtkSizeGroup   *rows,
          GtkSizeGroup   *icons,
          GtkWidget      *forget,
          NMDevice       *device,
          NMConnection   *connection,
          NMAccessPoint  *ap,
          NMAccessPoint  *active_ap,
          GtkWidget     **row_out,
          GtkWidget     **check_out,
          GtkWidget     **edit_out,
          GtkWidget     **connect_out,
          GtkWidget     **disconnect_out)
{
        GtkWidget *row, *row_box;
        GtkWidget *widget;
        GtkWidget *box;
        const gchar *title;
        gboolean active;
        gboolean in_range;
        gboolean connecting;
        guint security;
        guint strength;
        const GByteArray *ssid;
        const gchar *icon_name;
        guint64 timestamp;
        GtkSizeGroup *spinner_button_group;
        NMDeviceState state;

        g_assert (connection || ap);

        state = nm_device_get_state (device);

        if (connection != NULL) {
                NMSettingWireless *sw;
                NMSettingConnection *sc;
                sw = nm_connection_get_setting_wireless (connection);
                ssid = nm_setting_wireless_get_ssid (sw);
                sc = nm_connection_get_setting_connection (connection);
                timestamp = nm_setting_connection_get_timestamp (sc);
        } else {
                ssid = nm_access_point_get_ssid (ap);
                timestamp = 0;
        }

        title = nm_utils_escape_ssid (ssid->data, ssid->len);

        if (ap != NULL) {
                in_range = TRUE;
                active = (ap == active_ap) && (state == NM_DEVICE_STATE_ACTIVATED);
                connecting = (ap == active_ap) &&
                             (state == NM_DEVICE_STATE_PREPARE ||
                              state == NM_DEVICE_STATE_CONFIG ||
                              state == NM_DEVICE_STATE_IP_CONFIG ||
                              state == NM_DEVICE_STATE_IP_CHECK ||
                              state == NM_DEVICE_STATE_NEED_AUTH);
                security = get_access_point_security (ap);
                strength = nm_access_point_get_strength (ap);
        } else {
                in_range = FALSE;
                active = FALSE;
                connecting = FALSE;
                security = 0;
                strength = 0;
        }

        row = gtk_list_box_row_new ();
        gtk_size_group_add_widget (rows, row);

        row_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
        gtk_widget_set_margin_left (row_box, 12);
        gtk_widget_set_margin_right (row_box, 12);
        gtk_container_add (GTK_CONTAINER (row), row_box);

        widget = NULL;
        if (forget) {
                widget = gtk_check_button_new ();
                g_signal_connect (widget, "toggled",
                                  G_CALLBACK (check_toggled), forget);
                gtk_widget_set_halign (widget, GTK_ALIGN_CENTER);
                gtk_widget_set_valign (widget, GTK_ALIGN_CENTER);
                gtk_box_pack_start (GTK_BOX (row_box), widget, FALSE, FALSE, 0);
                g_signal_connect_object (row, "destroy",
                                         G_CALLBACK (update_forget), forget, G_CONNECT_SWAPPED);
        }
        if (check_out)
                *check_out = widget;

        widget = gtk_label_new (title);
        gtk_widget_set_margin_top (widget, 12);
        gtk_widget_set_margin_bottom (widget, 12);
        gtk_box_pack_start (GTK_BOX (row_box), widget, FALSE, FALSE, 0);

        if (active) {
                widget = gtk_image_new_from_icon_name ("emblem-default-symbolic", GTK_ICON_SIZE_MENU);
                gtk_widget_set_halign (widget, GTK_ALIGN_CENTER);
                gtk_widget_set_valign (widget, GTK_ALIGN_CENTER);
                gtk_box_pack_start (GTK_BOX (row_box), widget, FALSE, FALSE, 0);
        }

        gtk_box_pack_start (GTK_BOX (row_box), gtk_label_new (""), TRUE, FALSE, 0);

        spinner_button_group = gtk_size_group_new (GTK_SIZE_GROUP_BOTH);
        g_object_set_data_full (G_OBJECT (row), "spinner_button_group", spinner_button_group, g_object_unref);
             
        /*Connection prefrences button*/
        widget = NULL;
        if (connection) {
                GtkWidget *image;
                image = gtk_image_new_from_icon_name ("preferences-system-symbolic", GTK_ICON_SIZE_MENU);
                gtk_widget_show (image);
                widget = gtk_button_new ();
                gtk_style_context_add_class (gtk_widget_get_style_context (widget), "image-button");
                gtk_widget_set_no_show_all (widget, TRUE);
                gtk_widget_set_tooltip_text (widget, _("Preferences"));
                if (!connecting)
                        gtk_widget_show (widget);
                gtk_container_add (GTK_CONTAINER (widget), image);
                gtk_widget_set_halign (widget, GTK_ALIGN_CENTER);
                gtk_widget_set_valign (widget, GTK_ALIGN_CENTER);
                atk_object_set_name (gtk_widget_get_accessible (widget), _("Options"));
                gtk_box_pack_start (GTK_BOX (row_box), widget, FALSE, FALSE, 0);
                gtk_size_group_add_widget (spinner_button_group, widget);
                g_object_set_data (G_OBJECT (row), "edit", widget);
        }
        if (edit_out) {
            *edit_out = widget;
        }
        
        /*Connect Button*/        
        widget = NULL;
        if (!active && in_range) {
            GtkWidget *image;
            image = gtk_image_new_from_icon_name ("media-playback-start-symbolic", GTK_ICON_SIZE_MENU);
            gtk_widget_show (image);
            widget = gtk_button_new ();
            gtk_style_context_add_class (gtk_widget_get_style_context (widget), "image-button");
            gtk_widget_set_no_show_all (widget, TRUE);
            gtk_widget_set_tooltip_text (widget, _("Connect"));
            if (!connecting)
                    gtk_widget_show (widget);
            gtk_container_add (GTK_CONTAINER (widget), image);
            gtk_widget_set_halign (widget, GTK_ALIGN_CENTER);
            gtk_widget_set_valign (widget, GTK_ALIGN_CENTER);
            gtk_box_pack_start (GTK_BOX (row_box), widget, FALSE, FALSE, 0);
            gtk_size_group_add_widget (spinner_button_group, widget);
            g_object_set_data (G_OBJECT (row), "connect", widget);
        }
        if (connect_out) {
            *connect_out = widget;
        }
        
        /*Disconnect Button */
        widget = NULL;
        if (active && in_range) {
            GtkWidget *image;
            image = gtk_image_new_from_icon_name ("media-playback-stop-symbolic", GTK_ICON_SIZE_MENU);
            gtk_widget_show (image);
            widget = gtk_button_new ();
            gtk_style_context_add_class (gtk_widget_get_style_context (widget), "image-button");
            gtk_widget_set_no_show_all (widget, TRUE);
            gtk_widget_set_tooltip_text (widget, _("Disconnect"));
            if (!connecting)
                    gtk_widget_show (widget);
            gtk_container_add (GTK_CONTAINER (widget), image);
            gtk_widget_set_halign (widget, GTK_ALIGN_CENTER);
            gtk_widget_set_valign (widget, GTK_ALIGN_CENTER);
            gtk_box_pack_start (GTK_BOX (row_box), widget, FALSE, FALSE, 0);
            gtk_size_group_add_widget (spinner_button_group, widget);
            g_object_set_data (G_OBJECT (row), "disconnect", widget);
        }
        if (connect_out) {
            *disconnect_out = widget;
        }

        widget = gtk_spinner_new ();
        gtk_widget_set_no_show_all (widget, TRUE);
        if (connecting) {
                gtk_widget_show (widget);
                gtk_spinner_start (GTK_SPINNER (widget));
        }
        gtk_widget_set_halign (widget, GTK_ALIGN_CENTER);
        gtk_widget_set_valign (widget, GTK_ALIGN_CENTER);
        gtk_box_pack_start (GTK_BOX (row_box), widget, FALSE, FALSE, 0);
        gtk_size_group_add_widget (spinner_button_group, widget);
        g_object_set_data (G_OBJECT (row), "spinner", widget);

        box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
        gtk_box_set_homogeneous (GTK_BOX (box), TRUE);
        gtk_size_group_add_widget (icons, box);
        gtk_box_pack_start (GTK_BOX (row_box), box, FALSE, FALSE, 0);

        if (in_range) {
                if (security != NM_AP_SEC_UNKNOWN &&
                    security != NM_AP_SEC_NONE) {
                        widget = gtk_image_new_from_icon_name ("network-wireless-encrypted-symbolic", GTK_ICON_SIZE_MENU);
                } else {
                        widget = gtk_label_new ("");
                }
                gtk_box_pack_start (GTK_BOX (box), widget, FALSE, FALSE, 0);

                if (strength < 20)
                        icon_name = "network-wireless-signal-none-symbolic";
                else if (strength < 40)
                        icon_name = "network-wireless-signal-weak-symbolic";
                else if (strength < 50)
                        icon_name = "network-wireless-signal-ok-symbolic";
                else if (strength < 80)
                        icon_name = "network-wireless-signal-good-symbolic";
                else
                        icon_name = "network-wireless-signal-excellent-symbolic";
                widget = gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_MENU);
                gtk_box_pack_start (GTK_BOX (box), widget, FALSE, FALSE, 0);
        }

        gtk_widget_show_all (row);

        g_object_set_data (G_OBJECT (row), "ap", ap);
        if (connection) {
                g_object_set_data (G_OBJECT (row), "connection", connection);
                g_signal_connect_object (connection, "removed",
                                         G_CALLBACK (gtk_widget_destroy), row, G_CONNECT_SWAPPED);
        }
        g_object_set_data (G_OBJECT (row), "timestamp", GUINT_TO_POINTER (timestamp));
        g_object_set_data (G_OBJECT (row), "active", GUINT_TO_POINTER (active));
        g_object_set_data (G_OBJECT (row), "strength", GUINT_TO_POINTER (strength));

        *row_out = row;
}

static gint
history_sort (gconstpointer a, gconstpointer b, gpointer data)
{
        guint64 ta, tb;

        ta = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (a), "timestamp"));
        tb = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (b), "timestamp"));

        if (ta > tb) return -1;
        if (tb > ta) return 1;

        return 0;
}

static gint
ap_sort (gconstpointer a, gconstpointer b, gpointer data)
{
        guint sa, sb;

        sa = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (a), "strength"));
        sb = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (b), "strength"));
        if (sa > sb) return -1;
        if (sb > sa) return 1;

        return 0;
}

static void
editor_done (NetConnectionEditor *editor,
             gboolean             success,
             NetDeviceWifi       *device_wifi)
{
        g_object_unref (editor);
}

static void
show_details_for_row (GtkButton *button, NetDeviceWifi *device_wifi)
{
        GtkWidget *row;
        NMConnection *connection;
        NMAccessPoint *ap;
        GtkWidget *window;
        NetConnectionEditor *editor;
        NMClient *client;
        NMRemoteSettings *settings;
        NMDevice *device;

        window = gtk_widget_get_toplevel (GTK_WIDGET (button));

        row = GTK_WIDGET (g_object_get_data (G_OBJECT (button), "row"));
        connection = NM_CONNECTION (g_object_get_data (G_OBJECT (row), "connection"));
        ap = NM_ACCESS_POINT (g_object_get_data (G_OBJECT (row), "ap"));

        device = net_device_get_nm_device (NET_DEVICE (device_wifi));
        client = net_object_get_client (NET_OBJECT (device_wifi));
        settings = net_object_get_remote_settings (NET_OBJECT (device_wifi));
        editor = net_connection_editor_new (GTK_WINDOW (window), connection, device, ap, client, settings);
        g_signal_connect (editor, "done", G_CALLBACK (editor_done), device_wifi);
        net_connection_editor_run (editor);
}

static void
ap_disconnect (GtkButton *button, NetDeviceWifi *device_wifi)
{
        NMDevice *device;
        device = net_device_get_nm_device (NET_DEVICE (device_wifi));
        if (device == NULL) {
            return;
        }
        nm_device_disconnect (device, NULL, NULL);
}

static void
ap_activated (GtkButton *button, NetDeviceWifi *device_wifi)
{
        GtkWidget *row;
        NMConnection *connection;
        NMAccessPoint *ap;
        NMClient *client;
        NMDevice *nm_device;
        GtkWidget *spinner;
        GtkWidget *edit;
        GtkWidget *connect;

        row = GTK_WIDGET (g_object_get_data (G_OBJECT (button), "row_1"));
        connection = NM_CONNECTION (g_object_get_data (G_OBJECT (row), "connection"));
        ap = NM_ACCESS_POINT (g_object_get_data (G_OBJECT (row), "ap"));
        spinner = GTK_WIDGET (g_object_get_data (G_OBJECT (row), "spinner"));
        edit = GTK_WIDGET (g_object_get_data (G_OBJECT (row), "edit"));
        connect = GTK_WIDGET (g_object_get_data (G_OBJECT (row), "connect"));

        if (ap != NULL) {
                if (connection != NULL) {
                        gtk_widget_hide (edit);
                        client = net_object_get_client (NET_OBJECT (device_wifi));
                        nm_device = net_device_get_nm_device (NET_DEVICE (device_wifi));
                        nm_client_activate_connection (client,
                                                       connection,
                                                       nm_device, NULL,
                                                       connection_activate_cb, device_wifi);
                } else {
                        const GByteArray *ssid;
                        gchar *ssid_text;
                        const gchar *object_path;
                        gtk_widget_show (spinner);
                        gtk_spinner_start (GTK_SPINNER (spinner));
                        gtk_widget_hide (connect);

                        ssid = nm_access_point_get_ssid (ap);
                        ssid_text = g_markup_escape_text (nm_utils_escape_ssid (ssid->data, ssid->len), -1);
                        object_path = nm_object_get_path (NM_OBJECT (ap));
                        wireless_try_to_connect (device_wifi, ssid_text, object_path);
                        g_free (ssid_text);
                }
        }
}

static void
open_history (NetDeviceWifi *device_wifi)
{
        GtkWidget *dialog;
        GtkWidget *window;
        CcNetworkPanel *panel;
        GtkWidget *button;
        GtkWidget *connect_button;
        GtkWidget *disconnect_button;
        GtkWidget *forget;
        GtkWidget *hidden;
        GtkWidget *swin;
        GSList *connections;
        GSList *l;
        const GPtrArray *aps;
        GPtrArray *aps_unique = NULL;
        NMAccessPoint *active_ap;
        guint i;
        NMDevice *nm_device;
        GtkWidget *list;
        GtkWidget *row;
        GtkSizeGroup *rows;
        GtkSizeGroup *icons;

        dialog = gtk_dialog_new ();
        panel = net_object_get_panel (NET_OBJECT (device_wifi));
        window = gtk_widget_get_toplevel (GTK_WIDGET (panel));
        gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (window));
        gtk_window_set_title (GTK_WINDOW (dialog), _("Known Networks"));
        gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
        gtk_window_set_default_size (GTK_WINDOW (dialog), 600, 400);

        button = gtk_button_new_with_mnemonic (_("_Close"));
        gtk_widget_set_can_default (button, TRUE);
        gtk_widget_show (button);
        gtk_dialog_add_action_widget (GTK_DIALOG (dialog), button, 0);
        g_signal_connect_swapped (button, "clicked",
                                  G_CALLBACK (gtk_widget_destroy), dialog);

        /* translators: This is the label for the "Forget wireless network" functionality */
        forget = gtk_button_new_with_mnemonic (C_("Wi-Fi Network", "_Forget"));
        gtk_widget_show (forget);
        gtk_widget_set_sensitive (forget, FALSE);
        gtk_dialog_add_action_widget (GTK_DIALOG (dialog), forget, 0);
        g_signal_connect (forget, "clicked",
                          G_CALLBACK (forget_selected), device_wifi);
        gtk_container_child_set (GTK_CONTAINER (gtk_widget_get_parent (forget)), forget, "secondary", TRUE, NULL);
        g_object_set_data (G_OBJECT (forget), "net", device_wifi);

        swin = gtk_scrolled_window_new (NULL, NULL);
        gtk_widget_show (swin);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (swin), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (swin), GTK_SHADOW_IN);
        gtk_widget_set_margin_left (swin, 50);
        gtk_widget_set_margin_right (swin, 50);
        gtk_widget_set_margin_top (swin, 12);
        gtk_widget_set_margin_bottom (swin, 12);
        gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), swin, TRUE, TRUE, 0);

        list = GTK_WIDGET (gtk_list_box_new ());
        gtk_widget_show (list);
        gtk_list_box_set_selection_mode (GTK_LIST_BOX (list), GTK_SELECTION_NONE);
        gtk_list_box_set_header_func (GTK_LIST_BOX (list), cc_list_box_update_header_func, NULL, NULL);
        gtk_list_box_set_sort_func (GTK_LIST_BOX (list), (GtkListBoxSortFunc)history_sort, NULL, NULL);
        gtk_container_add (GTK_CONTAINER (swin), list);

        rows = gtk_size_group_new (GTK_SIZE_GROUP_VERTICAL);
        icons = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
        g_object_set_data_full (G_OBJECT (list), "rows", rows, g_object_unref);
        g_object_set_data_full (G_OBJECT (list), "icons", icons, g_object_unref);

        nm_device = net_device_get_nm_device (NET_DEVICE (device_wifi));

        connections = net_device_get_valid_connections (NET_DEVICE (device_wifi));

        aps = nm_device_wifi_get_access_points (NM_DEVICE_WIFI (nm_device));
        aps_unique = panel_get_strongest_unique_aps (aps);
        active_ap = nm_device_wifi_get_active_access_point (NM_DEVICE_WIFI (nm_device));

        for (l = connections; l; l = l->next) {
                NMConnection *connection = l->data;
                NMAccessPoint *ap = NULL;
                NMSetting *setting;
                const GByteArray *ssid;
                if (connection_is_shared (connection))
                        continue;

                setting = nm_connection_get_setting_by_name (connection, NM_SETTING_WIRELESS_SETTING_NAME);
                ssid = nm_setting_wireless_get_ssid (NM_SETTING_WIRELESS (setting));
                for (i = 0; i < aps_unique->len; i++) {
                        const GByteArray *ssid_ap;
                        ap = NM_ACCESS_POINT (g_ptr_array_index (aps_unique, i));
                        ssid_ap = nm_access_point_get_ssid (ap);
                        if (nm_utils_same_ssid (ssid, ssid_ap, TRUE))
                                break;
                        ap = NULL;
                }

                make_row (rows, icons, forget, nm_device, connection, ap, active_ap, &row, NULL, &button, &connect_button, &disconnect_button);
                gtk_container_add (GTK_CONTAINER (list), row);
                if (button) {
                        g_signal_connect (button, "clicked",
                                          G_CALLBACK (show_details_for_row), device_wifi);
                        g_object_set_data (G_OBJECT (button), "row", row);
                }
                if (connect_button) {
                        g_signal_connect (connect_button, "clicked",
                                          G_CALLBACK (ap_activated), device_wifi);
                        g_object_set_data (G_OBJECT (connect_button), "row_1", row);
                }
                if (disconnect_button) {
                        g_signal_connect (disconnect_button, "clicked",
                                          G_CALLBACK (ap_disconnect), device_wifi);
                        g_object_set_data (G_OBJECT (disconnect_button), "row_1", row);
                }
        }
        g_slist_free (connections);
        g_ptr_array_free (aps_unique, TRUE);

        gtk_window_present (GTK_WINDOW (dialog));
}

static void
populate_ap_list (NetDeviceWifi *device_wifi)
{
        GtkWidget *swin;
        GtkWidget *list;
        GtkSizeGroup *rows;
        GtkSizeGroup *icons;
        NMDevice *nm_device;
        GSList *connections;
        GSList *l;
        const GPtrArray *aps;
        GPtrArray *aps_unique = NULL;
        NMAccessPoint *active_ap;
        guint i;
        GtkWidget *row;
        GtkWidget *button;
        GtkWidget *connect_button;
        GtkWidget *disconnect_button;
        GList *children, *child;

        swin = GTK_WIDGET (gtk_builder_get_object (device_wifi->priv->builder,
                                                   "scrolledwindow_list"));
        list = gtk_bin_get_child (GTK_BIN (gtk_bin_get_child (GTK_BIN (swin))));

        children = gtk_container_get_children (GTK_CONTAINER (list));
        for (child = children; child; child = child->next) {
                gtk_container_remove (GTK_CONTAINER (list), GTK_WIDGET (child->data));
        }
        g_list_free (children);

        rows = GTK_SIZE_GROUP (g_object_get_data (G_OBJECT (list), "rows"));
        icons = GTK_SIZE_GROUP (g_object_get_data (G_OBJECT (list), "icons"));

        nm_device = net_device_get_nm_device (NET_DEVICE (device_wifi));

        connections = net_device_get_valid_connections (NET_DEVICE (device_wifi));

        aps = nm_device_wifi_get_access_points (NM_DEVICE_WIFI (nm_device));
        aps_unique = panel_get_strongest_unique_aps (aps);
        active_ap = nm_device_wifi_get_active_access_point (NM_DEVICE_WIFI (nm_device));

        for (i = 0; i < aps_unique->len; i++) {
                const GByteArray *ssid_ap;
                NMAccessPoint *ap;
                NMConnection *connection = NULL;
                ap = NM_ACCESS_POINT (g_ptr_array_index (aps_unique, i));
                ssid_ap = nm_access_point_get_ssid (ap);
                for (l = connections; l; l = l->next) {
                        connection = l->data;
                        NMSetting *setting;
                        const GByteArray *ssid;

                        if (connection_is_shared (connection)) {
                                connection = NULL;
                                continue;
                        }

                        setting = nm_connection_get_setting_by_name (connection, NM_SETTING_WIRELESS_SETTING_NAME);
                        ssid = nm_setting_wireless_get_ssid (NM_SETTING_WIRELESS (setting));
                        if (nm_utils_same_ssid (ssid, ssid_ap, TRUE))
                                break;
                        connection = NULL;
                }

                make_row (rows, icons, NULL, nm_device, connection, ap, active_ap, &row, NULL, &button, &connect_button, &disconnect_button);
                gtk_container_add (GTK_CONTAINER (list), row);
                if (button) {
                        g_signal_connect (button, "clicked",
                                          G_CALLBACK (show_details_for_row), device_wifi);
                        g_object_set_data (G_OBJECT (button), "row", row);
                }
                if (connect_button) {
                        g_signal_connect (connect_button, "clicked",
                                          G_CALLBACK (ap_activated), device_wifi);
                        g_object_set_data (G_OBJECT (connect_button), "row_1", row);
                }
                if (disconnect_button) {
                        g_signal_connect (disconnect_button, "clicked",
                                          G_CALLBACK (ap_disconnect), device_wifi);
                        g_object_set_data (G_OBJECT (disconnect_button), "row_1", row);
                }
        }

        g_slist_free (connections);
        g_ptr_array_free (aps_unique, TRUE);
}

static void
net_device_wifi_init (NetDeviceWifi *device_wifi)
{
        GError *error = NULL;
        GtkWidget *widget;
        GtkWidget *swin;
        GtkWidget *list;
        GtkSizeGroup *rows;
        GtkSizeGroup *icons;

        device_wifi->priv = NET_DEVICE_WIFI_GET_PRIVATE (device_wifi);

        device_wifi->priv->builder = gtk_builder_new ();
        gtk_builder_set_translation_domain (device_wifi->priv->builder, GETTEXT_PACKAGE);
        gtk_builder_add_from_resource (device_wifi->priv->builder,
                                       "/org/cinnamon/control-center/network/network-wifi.ui",
                                       &error);
        if (error != NULL) {
                g_warning ("Could not load interface file: %s", error->message);
                g_error_free (error);
                return;
        }

        widget = GTK_WIDGET (gtk_builder_get_object (device_wifi->priv->builder,
                                                     "details_dialog"));
        device_wifi->priv->details_dialog = widget;

        widget = GTK_WIDGET (gtk_builder_get_object (device_wifi->priv->builder,
                                                     "hotspot-dialog"));
        device_wifi->priv->hotspot_dialog = widget;
        
        widget = GTK_WIDGET (gtk_builder_get_object (device_wifi->priv->builder,
                                                     "connect_phidden_button"));
        g_signal_connect_swapped (widget, "clicked",
                          G_CALLBACK (connect_to_hidden_network), device_wifi);

        /* setup wifi views */
        widget = GTK_WIDGET (gtk_builder_get_object (device_wifi->priv->builder,
                                                     "device_off_switch"));
        g_signal_connect (widget, "notify::active",
                          G_CALLBACK (device_off_toggled), device_wifi);

        swin = GTK_WIDGET (gtk_builder_get_object (device_wifi->priv->builder,
                                                   "scrolledwindow_list"));
        list = GTK_WIDGET (gtk_list_box_new ());
        gtk_widget_show (list);
        gtk_list_box_set_selection_mode (GTK_LIST_BOX (list), GTK_SELECTION_NONE);
        gtk_list_box_set_header_func (GTK_LIST_BOX (list), cc_list_box_update_header_func, NULL, NULL);
        gtk_list_box_set_sort_func (GTK_LIST_BOX (list), (GtkListBoxSortFunc)ap_sort, NULL, NULL);
        gtk_container_add (GTK_CONTAINER (swin), list);

        rows = gtk_size_group_new (GTK_SIZE_GROUP_VERTICAL);
        icons = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
        g_object_set_data_full (G_OBJECT (list), "rows", rows, g_object_unref);
        g_object_set_data_full (G_OBJECT (list), "icons", icons, g_object_unref);

        /* setup view */
        widget = GTK_WIDGET (gtk_builder_get_object (device_wifi->priv->builder,
                                                     "notebook_view"));
        gtk_notebook_set_show_tabs (GTK_NOTEBOOK (widget), FALSE);
        gtk_notebook_set_current_page (GTK_NOTEBOOK (widget), 0);
        g_signal_connect_after (widget, "switch-page",
                                G_CALLBACK (switch_page_cb), device_wifi);

        widget = GTK_WIDGET (gtk_builder_get_object (device_wifi->priv->builder,
                                                     "start_hotspot_button"));
        g_signal_connect (widget, "clicked",
                          G_CALLBACK (start_hotspot), device_wifi);

        widget = GTK_WIDGET (gtk_builder_get_object (device_wifi->priv->builder,
                                                     "history_button"));
        g_signal_connect_swapped (widget, "clicked",
                                  G_CALLBACK (open_history), device_wifi);

        widget = GTK_WIDGET (gtk_builder_get_object (device_wifi->priv->builder,
                                                     "switch_hotspot_off"));
        g_signal_connect (widget, "notify::active",
                          G_CALLBACK (switch_hotspot_changed_cb), device_wifi);
}
