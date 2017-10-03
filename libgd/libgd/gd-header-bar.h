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

#ifndef __GD_HEADER_BAR_H__
#define __GD_HEADER_BAR_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GD_TYPE_HEADER_BAR            (gd_header_bar_get_type ())
#define GD_HEADER_BAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GD_TYPE_HEADER_BAR, GdHeaderBar))
#define GD_HEADER_BAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GD_TYPE_HEADER_BAR, GdHeaderBarClass))
#define GD_IS_HEADER_BAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GD_TYPE_HEADER_BAR))
#define GD_IS_HEADER_BAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GD_TYPE_HEADER_BAR))
#define GD_HEADER_BAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GD_TYPE_HEADER_BAR, GdHeaderBarClass))

typedef struct _GdHeaderBar              GdHeaderBar;
typedef struct _GdHeaderBarPrivate       GdHeaderBarPrivate;
typedef struct _GdHeaderBarClass         GdHeaderBarClass;

struct _GdHeaderBar
{
  GtkContainer container;

  /*< private >*/
  GdHeaderBarPrivate *priv;
};

struct _GdHeaderBarClass
{
  GtkContainerClass parent_class;

  /* Padding for future expansion */
  void (*_gd_reserved1) (void);
  void (*_gd_reserved2) (void);
  void (*_gd_reserved3) (void);
  void (*_gd_reserved4) (void);
};

GType        gd_header_bar_get_type          (void) G_GNUC_CONST;
GtkWidget   *gd_header_bar_new               (void);
void         gd_header_bar_set_title         (GdHeaderBar *bar,
                                              const char  *title);
const char * gd_header_bar_get_title         (GdHeaderBar *bar);
void         gd_header_bar_set_subtitle      (GdHeaderBar *bar,
                                              const gchar *subtitle);
const gchar *gd_header_bar_get_subtitle      (GdHeaderBar *bar);
void         gd_header_bar_set_custom_title  (GdHeaderBar *bar,
                                              GtkWidget   *title_widget);
GtkWidget *  gd_header_bar_get_custom_title  (GdHeaderBar *bar);
void         gd_header_bar_pack_start        (GdHeaderBar *bar,
                                              GtkWidget   *child);
void         gd_header_bar_pack_end          (GdHeaderBar *bar,
                                              GtkWidget   *child);

G_END_DECLS

#endif /* __GD_HEADER_BAR_H__ */
