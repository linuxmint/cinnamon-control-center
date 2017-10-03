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
 * Author: Alexander Larsson <alexl@redhat.com>
 *
 */

#ifndef __GD_REVEALER_H__
#define __GD_REVEALER_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS


#define GD_TYPE_REVEALER (gd_revealer_get_type ())
#define GD_REVEALER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GD_TYPE_REVEALER, GdRevealer))
#define GD_REVEALER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GD_TYPE_REVEALER, GdRevealerClass))
#define GD_IS_REVEALER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GD_TYPE_REVEALER))
#define GD_IS_REVEALER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GD_TYPE_REVEALER))
#define GD_REVEALER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GD_TYPE_REVEALER, GdRevealerClass))

typedef struct _GdRevealer GdRevealer;
typedef struct _GdRevealerClass GdRevealerClass;
typedef struct _GdRevealerPrivate GdRevealerPrivate;

struct _GdRevealer {
  GtkBin parent_instance;
  GdRevealerPrivate * priv;
};

struct _GdRevealerClass {
  GtkBinClass parent_class;
};


GType          gd_revealer_get_type                (void) G_GNUC_CONST;
GtkWidget*     gd_revealer_new                     (void);
gboolean       gd_revealer_get_reveal_child        (GdRevealer     *revealer);
void           gd_revealer_set_reveal_child        (GdRevealer     *revealer,
						    gboolean        setting);
GtkOrientation gd_revealer_get_orientation         (GdRevealer     *revealer);
void           gd_revealer_set_orientation         (GdRevealer     *revealer,
						    GtkOrientation  value);
gint           gd_revealer_get_transition_duration (GdRevealer     *revealer);
void           gd_revealer_set_transition_duration (GdRevealer     *revealer,
						    gint            duration_msec);

gboolean       gd_revealer_get_child_revealed      (GdRevealer     *revealer);


G_END_DECLS

#endif
