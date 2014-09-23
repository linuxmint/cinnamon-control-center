/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2012 Red Hat, Inc
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

#include <nm-setting-wireless.h>
#include <nm-utils.h>
#include <nm-device-wifi.h>

#include <net/if_arp.h>

#include "firewall-helpers.h"
#include "ce-page-wifi.h"
#include "ui-helpers.h"

G_DEFINE_TYPE (CEPageWifi, ce_page_wifi, CE_TYPE_PAGE)

static void
all_user_changed (GtkToggleButton *b, CEPageWifi *page)
{
        gboolean all_users;
        NMSettingConnection *sc;

        sc = nm_connection_get_setting_connection (CE_PAGE (page)->connection);
        all_users = gtk_toggle_button_get_active (b);

        g_object_set (sc, "permissions", NULL, NULL);
        if (!all_users)
                nm_setting_connection_add_permission (sc, "user", g_get_user_name (), NULL);
}

static void
connect_wifi_page (CEPageWifi *page)
{
        NMSettingConnection *sc;
        GtkWidget *widget;
        const GByteArray *ssid;
        gchar *utf8_ssid;
        GPtrArray *bssid_array;
        gchar **bssid_list;
        const GByteArray *s_bssid;
        gchar *s_bssid_str;
        gchar **mac_list;
        const GByteArray *s_mac;
        gchar *s_mac_str;
        gint i;

        widget = GTK_WIDGET (gtk_builder_get_object (CE_PAGE (page)->builder,
                                                     "entry_ssid"));

        ssid = nm_setting_wireless_get_ssid (page->setting);
        if (ssid)
                utf8_ssid = nm_utils_ssid_to_utf8 (ssid);
        else
                utf8_ssid = g_strdup ("");
        gtk_entry_set_text (GTK_ENTRY (widget), utf8_ssid);
        g_free (utf8_ssid);

        g_signal_connect_swapped (widget, "changed", G_CALLBACK (ce_page_changed), page);

        widget = GTK_WIDGET (gtk_builder_get_object (CE_PAGE (page)->builder,
                                                     "combo_bssid"));

        bssid_array = g_ptr_array_new ();
        for (i = 0; i < nm_setting_wireless_get_num_seen_bssids (page->setting); i++) {
                g_ptr_array_add (bssid_array, g_strdup (nm_setting_wireless_get_seen_bssid (page->setting, i)));
        }
        g_ptr_array_add (bssid_array, NULL);
        bssid_list = (gchar **) g_ptr_array_free (bssid_array, FALSE);
        s_bssid = nm_setting_wireless_get_bssid (page->setting);
        s_bssid_str = s_bssid ? nm_utils_hwaddr_ntoa (s_bssid->data, ARPHRD_ETHER) : NULL;
        ce_page_setup_mac_combo (GTK_COMBO_BOX_TEXT (widget), s_bssid_str, bssid_list);
        g_free (s_bssid_str);
        g_strfreev (bssid_list);
        g_signal_connect_swapped (widget, "changed", G_CALLBACK (ce_page_changed), page);

        widget = GTK_WIDGET (gtk_builder_get_object (CE_PAGE (page)->builder,
                                                     "combo_mac"));
        mac_list = ce_page_get_mac_list (CE_PAGE (page)->client, NM_TYPE_DEVICE_WIFI,
                                         NM_DEVICE_WIFI_PERMANENT_HW_ADDRESS);
        s_mac = nm_setting_wireless_get_mac_address (page->setting);
        s_mac_str = s_mac ? nm_utils_hwaddr_ntoa (s_mac->data, ARPHRD_ETHER) : NULL;
        ce_page_setup_mac_combo (GTK_COMBO_BOX_TEXT (widget), s_mac_str, mac_list);
        g_free (s_mac_str);
        g_strfreev (mac_list);
        g_signal_connect_swapped (widget, "changed", G_CALLBACK (ce_page_changed), page);


        widget = GTK_WIDGET (gtk_builder_get_object (CE_PAGE (page)->builder,
                                                     "entry_cloned_mac"));
        ce_page_mac_to_entry (nm_setting_wireless_get_cloned_mac_address (page->setting),
                              ARPHRD_ETHER, GTK_ENTRY (widget));
        g_signal_connect_swapped (widget, "changed", G_CALLBACK (ce_page_changed), page);

        widget = GTK_WIDGET (gtk_builder_get_object (CE_PAGE (page)->builder,
                                                     "auto_connect_check"));
        sc = nm_connection_get_setting_connection (CE_PAGE (page)->connection);
        g_object_bind_property (sc, "autoconnect",
                                widget, "active",
                                G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
        g_signal_connect_swapped (widget, "toggled", G_CALLBACK (ce_page_changed), page);

        widget = GTK_WIDGET (gtk_builder_get_object (CE_PAGE (page)->builder,
                                                     "all_user_check"));
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
                                      nm_setting_connection_get_num_permissions (sc) == 0);
        g_signal_connect (widget, "toggled",
                          G_CALLBACK (all_user_changed), page);
        g_signal_connect_swapped (widget, "toggled", G_CALLBACK (ce_page_changed), page);

        widget = GTK_WIDGET (gtk_builder_get_object (CE_PAGE (page)->builder, "combo_zone"));
        firewall_ui_setup (sc, widget, CE_PAGE (page)->cancellable);
        g_signal_connect_swapped (widget, "changed", G_CALLBACK (ce_page_changed), page);
}

static void
ui_to_setting (CEPageWifi *page)
{
        GByteArray *ssid;
        GByteArray *bssid = NULL;
        GByteArray *device_mac = NULL;
        GByteArray *cloned_mac = NULL;
        GtkWidget *entry;
        const gchar *utf8_ssid;
        NMSettingConnection *sc;

        entry = GTK_WIDGET (gtk_builder_get_object (CE_PAGE (page)->builder, "entry_ssid"));
        utf8_ssid = gtk_entry_get_text (GTK_ENTRY (entry));
        if (!utf8_ssid || !*utf8_ssid)
                ssid = NULL;
        else {
                ssid = g_byte_array_sized_new (strlen (utf8_ssid));
                g_byte_array_append (ssid, (const guint8*)utf8_ssid, strlen (utf8_ssid));
        }
        entry = gtk_bin_get_child (GTK_BIN (gtk_builder_get_object (CE_PAGE (page)->builder, "combo_bssid")));
        bssid = ce_page_entry_to_mac (GTK_ENTRY (entry), ARPHRD_ETHER, NULL);
        entry = gtk_bin_get_child (GTK_BIN (gtk_builder_get_object (CE_PAGE (page)->builder, "combo_mac")));
        device_mac = ce_page_entry_to_mac (GTK_ENTRY (entry), ARPHRD_ETHER, NULL);
        entry = GTK_WIDGET (gtk_builder_get_object (CE_PAGE (page)->builder, "entry_cloned_mac"));
        cloned_mac = ce_page_entry_to_mac (GTK_ENTRY (entry), ARPHRD_ETHER, NULL);

        sc = nm_connection_get_setting_connection (CE_PAGE (page)->connection);
        entry = GTK_WIDGET (gtk_builder_get_object (CE_PAGE (page)->builder, "combo_zone"));
        firewall_ui_to_setting (sc, entry);

        g_object_set (page->setting,
                      NM_SETTING_WIRELESS_SSID, ssid,
                      NM_SETTING_WIRELESS_BSSID, bssid,
                      NM_SETTING_WIRELESS_MAC_ADDRESS, device_mac,
                      NM_SETTING_WIRELESS_CLONED_MAC_ADDRESS, cloned_mac,
                      NULL);

        if (ssid)
                g_byte_array_free (ssid, TRUE);
        if (bssid)
                g_byte_array_free (bssid, TRUE);
        if (device_mac)
                g_byte_array_free (device_mac, TRUE);
        if (cloned_mac)
                g_byte_array_free (cloned_mac, TRUE);
}

static gboolean
validate (CEPage        *page,
          NMConnection  *connection,
          GError       **error)
{
        GtkWidget *entry;
        GByteArray *ignore;
        gboolean invalid;
        gchar *security;
        NMSettingWireless *setting;
        gboolean ret = TRUE;

        entry = gtk_bin_get_child (GTK_BIN (gtk_builder_get_object (page->builder, "combo_bssid")));
        ignore = ce_page_entry_to_mac (GTK_ENTRY (entry), ARPHRD_ETHER, &invalid);
        if (invalid) {
                widget_set_error (entry);
                ret = FALSE;
        } else {
                if (ignore)
                        g_byte_array_free (ignore, TRUE);
                widget_unset_error (entry);
        }

        entry = gtk_bin_get_child (GTK_BIN (gtk_builder_get_object (page->builder, "combo_mac")));
        ignore = ce_page_entry_to_mac (GTK_ENTRY (entry), ARPHRD_ETHER, &invalid);
        if (invalid) {
                widget_set_error (entry);
                ret = FALSE;
        } else {
                if (ignore)
                        g_byte_array_free (ignore, TRUE);
                widget_unset_error (entry);
        }

        entry = GTK_WIDGET (gtk_builder_get_object (page->builder, "entry_cloned_mac"));
        ignore = ce_page_entry_to_mac (GTK_ENTRY (entry), ARPHRD_ETHER, &invalid);
        if (invalid) {
                widget_set_error (entry);
                ret = FALSE;
        } else {
                if (ignore)
                        g_byte_array_free (ignore, TRUE);
                widget_unset_error (entry);
        }

        if (!ret)
                return ret;

        ui_to_setting (CE_PAGE_WIFI (page));

        /* A hack to not check the wifi security here */
        setting = CE_PAGE_WIFI (page)->setting;
        security = g_strdup (nm_setting_wireless_get_security (setting));
        g_object_set (setting, NM_SETTING_WIRELESS_SEC, NULL, NULL);
        ret = nm_setting_verify (NM_SETTING (setting), NULL, error);
        g_object_set (setting, NM_SETTING_WIRELESS_SEC, security, NULL);
        g_free (security);

        return ret;
}

static void
ce_page_wifi_init (CEPageWifi *page)
{
}

static void
ce_page_wifi_class_init (CEPageWifiClass *class)
{
        CEPageClass *page_class= CE_PAGE_CLASS (class);

        page_class->validate = validate;
}

CEPage *
ce_page_wifi_new (NMConnection     *connection,
                      NMClient         *client,
                      NMRemoteSettings *settings)
{
        CEPageWifi *page;

        page = CE_PAGE_WIFI (ce_page_new (CE_TYPE_PAGE_WIFI,
                                          connection,
                                          client,
                                          settings,
                                          "/org/cinnamon/control-center/network/wifi-page.ui",
                                          _("Identity")));

        page->setting = nm_connection_get_setting_wireless (connection);

        connect_wifi_page (page);

        return CE_PAGE (page);
}
