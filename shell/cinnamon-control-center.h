/*
 * Copyright (c) 2010 Intel, Inc.
 *
 * The Control Center is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * The Control Center is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with the Control Center; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Thomas Wood <thos@gnome.org>
 */

#ifndef _CINNAMON_CONTROL_CENTER_H
#define _CINNAMON_CONTROL_CENTER_H

#include <glib-object.h>
#include "cc-shell.h"

G_BEGIN_DECLS

#define CINNAMON_TYPE_CONTROL_CENTER cinnamon_control_center_get_type()

#define CINNAMON_CONTROL_CENTER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  CINNAMON_TYPE_CONTROL_CENTER, CinnamonControlCenter))

#define CINNAMON_CONTROL_CENTER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  CINNAMON_TYPE_CONTROL_CENTER, CinnamonControlCenterClass))

#define GNOME_IS_CONTROL_CENTER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  CINNAMON_TYPE_CONTROL_CENTER))

#define GNOME_IS_CONTROL_CENTER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  CINNAMON_TYPE_CONTROL_CENTER))

#define CINNAMON_CONTROL_CENTER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  CINNAMON_TYPE_CONTROL_CENTER, CinnamonControlCenterClass))

typedef struct _CinnamonControlCenter CinnamonControlCenter;
typedef struct _CinnamonControlCenterClass CinnamonControlCenterClass;
typedef struct _CinnamonControlCenterPrivate CinnamonControlCenterPrivate;

struct _CinnamonControlCenter
{
  CcShell parent;

  CinnamonControlCenterPrivate *priv;
};

struct _CinnamonControlCenterClass
{
  CcShellClass parent_class;
};

GType cinnamon_control_center_get_type (void) G_GNUC_CONST;

CinnamonControlCenter *cinnamon_control_center_new (void);

void cinnamon_control_center_present (CinnamonControlCenter *center);

void cinnamon_control_center_show (CinnamonControlCenter *center, GtkApplication *app);

void cinnamon_control_center_set_overview_page (CinnamonControlCenter *center);

G_END_DECLS

#endif /* _CINNAMON_CONTROL_CENTER_H */
