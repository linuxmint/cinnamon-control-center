/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright 2012  Red Hat, Inc,
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Written by: Matthias Clasen <mclasen@redhat.com>
 */

#include "config.h"

#include "pw-utils.h"

#include <glib.h>
#include <glib/gi18n.h>

gdouble
pw_strength (const gchar  *password,
             const gchar  *old_password,
             const gchar  *username,
             const gchar **hint,
             const gchar **long_hint)
{
        gint rv = 0;
        gdouble level = 0.0;
        gdouble strength = 0.0;
        void *auxerror;

        if (g_strcmp0(password, old_password) == 0) {
            *hint = C_("Password strength", "Duplicate");
            *long_hint = _("Your new password is the same as the old one");
            level=0.0;
            goto out;
        } else if (strlen (password) < MIN_PW_LENGTH) {
            *hint = C_("Password strength", "Too short");
            *long_hint = g_strdup_printf(_("Your new password needs to be at least %d characters long"), MIN_PW_LENGTH);
            level=0.0;
            goto out;
        } else {
            level=1.0;
            *hint = C_("Password strength", "OK");
            *long_hint = NULL;
            goto out;
        }
out:
        return level;
}
