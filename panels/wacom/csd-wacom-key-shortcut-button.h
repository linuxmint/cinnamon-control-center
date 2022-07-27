/*
 * csd-wacom-key-shortcut-button.h
 *
 * Copyright © 2013 Red Hat, Inc.
 *
 * Author: Joaquim Rocha <jrocha@redhat.com>
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
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define CSD_WACOM_TYPE_KEY_SHORTCUT_BUTTON (csd_wacom_key_shortcut_button_get_type ())
G_DECLARE_FINAL_TYPE (CsdWacomKeyShortcutButton, csd_wacom_key_shortcut_button, CSD, WACOM_KEY_SHORTCUT_BUTTON, GtkButton)

GType csd_wacom_key_shortcut_button_mode_type (void) G_GNUC_CONST;
#define CSD_WACOM_TYPE_KEY_SHORTCUT_BUTTON_MODE (csd_wacom_key_shortcut_button_mode_type ())

typedef enum
{
  CSD_WACOM_KEY_SHORTCUT_BUTTON_MODE_OTHER,
  CSD_WACOM_KEY_SHORTCUT_BUTTON_MODE_ALL
} CsdWacomKeyShortcutButtonMode;

GtkWidget    * csd_wacom_key_shortcut_button_new             (void);
