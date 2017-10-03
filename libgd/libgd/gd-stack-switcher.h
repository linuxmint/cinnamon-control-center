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

#ifndef __GD_STACK_SWITCHER_H__
#define __GD_STACK_SWITCHER_H__

#include <gtk/gtk.h>
#include "gd-stack.h"

G_BEGIN_DECLS

#define GD_TYPE_STACK_SWITCHER            (gd_stack_switcher_get_type ())
#define GD_STACK_SWITCHER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GD_TYPE_STACK_SWITCHER, GdStackSwitcher))
#define GD_STACK_SWITCHER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GD_TYPE_STACK_SWITCHER, GdStackSwitcherClass))
#define GD_IS_STACK_SWITCHER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GD_TYPE_STACK_SWITCHER))
#define GD_IS_STACK_SWITCHER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GD_TYPE_STACK_SWITCHER))
#define GD_STACK_SWITCHER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GD_TYPE_STACK_SWITCHER, GdStackSwitcherClass))

typedef struct _GdStackSwitcher              GdStackSwitcher;
typedef struct _GdStackSwitcherPrivate       GdStackSwitcherPrivate;
typedef struct _GdStackSwitcherClass         GdStackSwitcherClass;

struct _GdStackSwitcher
{
  GtkBox widget;

  /*< private >*/
  GdStackSwitcherPrivate *priv;
};

struct _GdStackSwitcherClass
{
  GtkBoxClass parent_class;

  /* Padding for future expansion */
  void (*_gd_reserved1) (void);
  void (*_gd_reserved2) (void);
  void (*_gd_reserved3) (void);
  void (*_gd_reserved4) (void);
};

GType        gd_stack_switcher_get_type          (void) G_GNUC_CONST;
GtkWidget   *gd_stack_switcher_new               (void);
void         gd_stack_switcher_set_stack         (GdStackSwitcher *switcher,
                                                  GdStack         *stack);
GdStack     *gd_stack_switcher_get_stack         (GdStackSwitcher *switcher);

G_END_DECLS

#endif /* __GD_STACK_SWITCHER_H__ */
