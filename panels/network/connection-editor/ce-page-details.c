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

#include <NetworkManager.h>

#include "../panel-common.h"
#include "ce-page-details.h"

G_DEFINE_TYPE (CEPageDetails, ce_page_details, CE_TYPE_PAGE)

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
update_last_used (CEPageDetails *page, NMConnection *connection)
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
                last_used = g_strdup (_("Never"));
                goto out;
        }

        /* calculate the amount of time that has elapsed */
        now = g_date_time_new_now_utc ();
        then = g_date_time_new_from_unix_utc (timestamp);

        diff = g_date_time_difference  (now, then);
        days = diff / G_TIME_SPAN_DAY;
        if (days == 0)
                last_used = g_strdup (_("Today"));
        else if (days == 1)
                last_used = g_strdup (_("Yesterday"));
        else
                last_used = g_strdup_printf (ngettext ("%i day ago", "%i days ago", days), days);
out:
        panel_set_device_widget_details (CE_PAGE (page)->builder, "last_used", last_used);
        if (now != NULL)
                g_date_time_unref (now);
        if (then != NULL)
                g_date_time_unref (then);
        g_free (last_used);
}

static void
connect_details_page (CEPageDetails *page)
{
        guint speed;
        guint strength;
        NMDeviceState state;
        NMAccessPoint *active_ap;
        const gchar *str;
        gboolean device_is_active;

        if (NM_IS_DEVICE_WIFI (page->device))
                active_ap = nm_device_wifi_get_active_access_point (NM_DEVICE_WIFI (page->device));
        else
                active_ap = NULL;

        state = page->device ? nm_device_get_state (page->device) : NM_DEVICE_STATE_DISCONNECTED;

        device_is_active = FALSE;
        speed = 0;
        if (active_ap && page->ap == active_ap && state != NM_DEVICE_STATE_UNAVAILABLE) {
                device_is_active = TRUE;
                if (NM_IS_DEVICE_WIFI (page->device))
                        speed = nm_device_wifi_get_bitrate (NM_DEVICE_WIFI (page->device)) / 1000;
        } else if (page->device) {
                NMActiveConnection *ac;
                const gchar *p1, *p2;

                ac = nm_device_get_active_connection (page->device);
                p1 = ac ? nm_active_connection_get_uuid (ac) : NULL;
                p2 = nm_connection_get_uuid (CE_PAGE (page)->connection);
                if (g_strcmp0 (p1, p2) == 0) {
                        device_is_active = TRUE;
                        if (NM_IS_DEVICE_WIFI (page->device))
                                speed = nm_device_wifi_get_bitrate (NM_DEVICE_WIFI (page->device)) / 1000;
                        else if (NM_IS_DEVICE_ETHERNET (page->device))
                                speed = nm_device_ethernet_get_speed (NM_DEVICE_ETHERNET (page->device));
                }
        }
        if (speed > 0)
                str = g_strdup_printf (_("%d Mb/s"), speed);
        else
                str = NULL;
        panel_set_device_widget_details (CE_PAGE (page)->builder, "speed", str);
        g_clear_pointer (&str, g_free);

        if (NM_IS_DEVICE_WIFI (page->device))
                str = nm_device_wifi_get_hw_address (NM_DEVICE_WIFI (page->device));
        else if (NM_IS_DEVICE_ETHERNET (page->device))
                str = nm_device_ethernet_get_hw_address (NM_DEVICE_ETHERNET (page->device));

        panel_set_device_widget_details (CE_PAGE (page)->builder, "mac", str);

        str = NULL;
        if (device_is_active && active_ap)
                str = get_ap_security_string (active_ap);
        panel_set_device_widget_details (CE_PAGE (page)->builder, "security", str);
        g_clear_pointer (&str, g_free);

        strength = 0;
        if (page->ap != NULL)
                strength = nm_access_point_get_strength (page->ap);

        if (strength <= 0)
                str = NULL;
        else if (strength < 20)
                str = C_("Signal strength", "None");
        else if (strength < 40)
                str = C_("Signal strength", "Weak");
        else if (strength < 50)
                str = C_("Signal strength", "Ok");
        else if (strength < 80)
                str = C_("Signal strength", "Good");
        else
                str = C_("Signal strength", "Excellent");
        panel_set_device_widget_details (CE_PAGE (page)->builder, "strength", str);

        /* set IP entries */
        if (device_is_active)
                panel_set_device_widgets (CE_PAGE (page)->builder, page->device);
        else
                panel_unset_device_widgets (CE_PAGE (page)->builder);

        if (!device_is_active && CE_PAGE (page)->connection)
                update_last_used (page, CE_PAGE (page)->connection);
        else
                panel_set_device_widget_details (CE_PAGE (page)->builder, "last_used", NULL);

}

static void
ce_page_details_init (CEPageDetails *page)
{
}

static void
ce_page_details_class_init (CEPageDetailsClass *class)
{
}

CEPage *
ce_page_details_new (NMConnection     *connection,
                     NMClient         *client,
                     NMDevice         *device,
                     NMAccessPoint    *ap)
{
        CEPageDetails *page;

        page = CE_PAGE_DETAILS (ce_page_new (CE_TYPE_PAGE_DETAILS,
                                             connection,
                                             client,
                                             "/org/cinnamon/control-center/network/details-page.ui",
                                             _("Details")));

        page->device = device;
        page->ap = ap;

        connect_details_page (page);

        return CE_PAGE (page);
}

