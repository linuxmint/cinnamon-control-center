/*
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
 * (C) Copyright 2013 Red Hat, Inc.
 */

#ifndef _FIREWALL_HELPERS_H_
#define _FIREWALL_HELPERS_H_

#include <nm-setting-connection.h>
#include <gtk/gtk.h>

void firewall_ui_setup      (NMSettingConnection *setting,
                             GtkWidget           *combo,
                             GCancellable        *cancellable);
void firewall_ui_to_setting (NMSettingConnection *setting,
                             GtkWidget           *combo);


#endif  /* _FIREWALL_HELPERS_H_ */
