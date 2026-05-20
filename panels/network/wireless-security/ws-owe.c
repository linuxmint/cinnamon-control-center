/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* NetworkManager Applet -- allow user control over networking
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Copyright 2007 - 2021 Red Hat, Inc.
 */

#include "nm-default.h"

#include "wireless-security.h"

struct _WirelessSecurityOWE {
    WirelessSecurity parent;
};

static gboolean
validate (WirelessSecurity *parent, GError **error)
{
    return TRUE;
}

static void
add_to_size_group (WirelessSecurity *parent, GtkSizeGroup *group)
{
}

static void
fill_connection (WirelessSecurity *parent, NMConnection *connection)
{
    NMSetting *s_wireless_sec;

    /* Blow away the old security setting by adding a clear one */
    s_wireless_sec = nm_setting_wireless_security_new ();
    g_object_set (s_wireless_sec,
                  NM_SETTING_WIRELESS_SECURITY_KEY_MGMT, "owe",
                  NULL);

    nm_connection_add_setting (connection, s_wireless_sec);
}

WirelessSecurityOWE *
ws_owe_new (NMConnection *connection)
{
    WirelessSecurity *parent;

    parent = wireless_security_init (sizeof (WirelessSecurityOWE),
                                     validate,
                                     add_to_size_group,
                                     fill_connection,
                                     NULL,
                                     NULL,
                                     "/org/cinnamon/control-center/network/ws-owe.ui",
                                     "owe_box",
                                     NULL);
    if (!parent)
        return NULL;

    parent->adhoc_compatible = FALSE;
    parent->hotspot_compatible = TRUE;

    return (WirelessSecurityOWE *) parent;
}
