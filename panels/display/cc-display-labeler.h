/* gnome-rr-labeler.h - Utility to label monitors to identify them
 * while they are being configured.
 *
 * Copyright 2008, Novell, Inc.
 *
 * This file is part of the Gnome Library.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Author: Federico Mena-Quintero <federico@novell.com>
 */
#pragma once

// #define GNOME_DESKTOP_USE_UNSTABLE_API
#include "cc-display-config.h"

G_BEGIN_DECLS

#define CC_TYPE_DISPLAY_LABELER (cc_display_labeler_get_type ())
G_DECLARE_FINAL_TYPE (CcDisplayLabeler, cc_display_labeler, CC, DISPLAY_LABELER, GObject)

CcDisplayLabeler *cc_display_labeler_new (CcDisplayConfig *config);
void cc_display_labeler_show (CcDisplayLabeler *labeler);
void cc_display_labeler_hide (CcDisplayLabeler *labeler);

G_END_DECLS