/*
 * Copyright 2012 Canonical
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
 * Author: Aurélien Gâteau <aurelien.gateau@canonical.com>
 */

#ifndef _CC_SHELL_NAV_BAR_H
#define _CC_SHELL_NAV_BAR_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define CC_TYPE_SHELL_NAV_BAR cc_shell_nav_bar_get_type()

#define CC_SHELL_NAV_BAR(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  CC_TYPE_SHELL_NAV_BAR, CcShellNavBar))

#define CC_SHELL_NAV_BAR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  CC_TYPE_SHELL_NAV_BAR, CcShellNavBarClass))

#define CC_IS_SHELL_NAV_BAR(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  CC_TYPE_SHELL_NAV_BAR))

#define CC_IS_SHELL_NAV_BAR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  CC_TYPE_SHELL_NAV_BAR))

#define CC_SHELL_NAV_BAR_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  CC_TYPE_SHELL_NAV_BAR, CcShellNavBarClass))

typedef struct _CcShellNavBar CcShellNavBar;
typedef struct _CcShellNavBarClass CcShellNavBarClass;
typedef struct _CcShellNavBarPrivate CcShellNavBarPrivate;

struct _CcShellNavBar
{
  GtkBox parent;

  CcShellNavBarPrivate *priv;
};

struct _CcShellNavBarClass
{
  GtkBoxClass parent_class;
};

GType cc_shell_nav_bar_get_type (void) G_GNUC_CONST;

GtkWidget *cc_shell_nav_bar_new (void);

void cc_shell_nav_bar_show_detail_button (CcShellNavBar *bar, const gchar *label);

void cc_shell_nav_bar_hide_detail_button (CcShellNavBar *bar);

G_END_DECLS

#endif /* _CC_SHELL_NAV_BAR_H */
