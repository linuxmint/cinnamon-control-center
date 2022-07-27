/*
 * Copyright © 2016 Red Hat, Inc.
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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Carlos Garnacho <carlosg@gnome.org>
 *
 */

#pragma once

#include "config.h"
#include <glib-object.h>

#define CC_TYPE_WACOM_OUTPUT_MANAGER (cc_wacom_output_manager_get_type ())
G_DECLARE_FINAL_TYPE (CcWacomOutputManager, cc_wacom_output_manager, CC, WACOM_OUTPUT_MANAGER, GObject)

typedef struct
{
    gchar   *connector_name;
    gchar   *display_name;
    gchar   *vendor;
    gchar   *product;
    gchar   *serial;
    gint     x;
    gint     y;
    gboolean primary;
    gboolean builtin;
}  MonitorInfo;

CcWacomOutputManager * cc_wacom_output_manager_get           (void);

MonitorInfo   * cc_wacom_output_manager_get_monitor      (CcWacomOutputManager *monitors);
void            cc_wacom_output_manager_set_monitor      (CcWacomOutputManager *monitors,
                                                          MonitorInfo          *monitor);

GList         * cc_wacom_output_manager_get_all_monitors (CcWacomOutputManager *monitors);
void            cc_wacom_output_manager_refresh_monitors (CcWacomOutputManager *monitors);

gboolean        monitor_info_cmp                         (MonitorInfo *a, MonitorInfo *b);
void            monitor_info_spew                        (MonitorInfo *info);
