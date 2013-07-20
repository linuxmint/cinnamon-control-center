/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright 2009-2010  Red Hat, Inc,
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
 * Written by: Matthias Clasen <mclasen@redhat.com>
 */

#ifndef __CC_LANGUAGE_CHOOSER_H__
#define __CC_LANGUAGE_CHOOSER_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

void              cc_add_user_languages            (GtkTreeModel *model);

GtkWidget        *cc_language_chooser_new          (GtkWidget *parent,
                                                    gboolean   regions);
void              cc_language_chooser_clear_filter (GtkWidget *chooser);
gchar            *cc_language_chooser_get_language (GtkWidget *chooser);

G_END_DECLS

#endif
