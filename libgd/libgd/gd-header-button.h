/*
 * Copyright (c) 2013 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef __GD_HEADER_BUTTON_H__
#define __GD_HEADER_BUTTON_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GD_TYPE_HEADER_BUTTON    (gd_header_button_get_type ())
#define GD_HEADER_BUTTON(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), GD_TYPE_HEADER_BUTTON, GdHeaderButton))
#define GD_IS_HEADER_BUTTON(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GD_TYPE_HEADER_BUTTON))

typedef struct _GdHeaderButton GdHeaderButton;

GType    gd_header_button_get_type               (void) G_GNUC_CONST;

void     gd_header_button_set_label              (GdHeaderButton *self,
                                                  const gchar    *label);
void     gd_header_button_set_symbolic_icon_name (GdHeaderButton *self,
                                                  const gchar    *symbolic_icon_name);
void     gd_header_button_set_use_markup         (GdHeaderButton *self,
                                                  gboolean        use_markup);
gchar *  gd_header_button_get_label              (GdHeaderButton *self);
gchar *  gd_header_button_get_symbolic_icon_name (GdHeaderButton *self);
gboolean gd_header_button_get_use_markup         (GdHeaderButton *self);

#define GD_TYPE_HEADER_SIMPLE_BUTTON (gd_header_simple_button_get_type ())
typedef GtkButton GdHeaderSimpleButton;
GType       gd_header_simple_button_get_type (void) G_GNUC_CONST;
GtkWidget * gd_header_simple_button_new      (void);

#define GD_TYPE_HEADER_TOGGLE_BUTTON (gd_header_toggle_button_get_type ())
typedef GtkToggleButton GdHeaderToggleButton;
GType       gd_header_toggle_button_get_type (void) G_GNUC_CONST;
GtkWidget * gd_header_toggle_button_new      (void);

#define GD_TYPE_HEADER_RADIO_BUTTON (gd_header_radio_button_get_type ())
typedef GtkRadioButton GdHeaderRadioButton;
GType       gd_header_radio_button_get_type  (void) G_GNUC_CONST;
GtkWidget * gd_header_radio_button_new       (void);

#define GD_TYPE_HEADER_MENU_BUTTON   (gd_header_menu_button_get_type ())
typedef GtkMenuButton GdHeaderMenuButton;
GType       gd_header_menu_button_get_type   (void) G_GNUC_CONST;
GtkWidget * gd_header_menu_button_new        (void);

G_END_DECLS

#endif /* __GD_HEADER_BUTTON_H__ */
