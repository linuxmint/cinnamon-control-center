/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */
/*
 * Copyright (C) 2011 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: David Zeuthen <davidz@redhat.com>
 */

#ifndef __GOA_PANEL_H__
#define __GOA_PANEL_H__

#include <shell/cc-panel.h>

G_BEGIN_DECLS

#define CC_TYPE_GOA_PANEL (cc_goa_panel_get_type ())

#define CC_GOA_PANEL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  CC_TYPE_GOA_PANEL, CcGoaPanel))

#define CC_GOA_PANEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  CC_TYPE_GOA_PANEL, CcGoaPanelClass))

#define CC_IS_GOA_PANEL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  CC_TYPE_GOA_PANEL))

#define CC_IS_GOA_PANEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  CC_TYPE_GOA_PANEL))

#define CC_GOA_PANEL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  CC_TYPE_GOA_PANEL, CcGoaPanelClass))

typedef struct _CcGoaPanel CcGoaPanel;
typedef struct _CcGoaPanelClass CcGoaPanelClass;
typedef struct _CcGoaPanelPrivate CcGoaPanelPrivate;

struct _CcGoaPanelClass
{
  CcPanelClass parent_class;
};

GType cc_goa_panel_get_type (void) G_GNUC_CONST;

void cc_goa_panel_register (GIOModule *module);

G_END_DECLS

#endif /* __GOA_PANEL_H__ */
