/* bg-wallpapers-source.h */
/*
 * Copyright (C) 2010 Intel, Inc
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
 * Author: Thomas Wood <thomas.wood@intel.com>
 *
 */


#ifndef _BG_WALLPAPERS_SOURCE_H
#define _BG_WALLPAPERS_SOURCE_H

#include <gtk/gtk.h>
#include "bg-source.h"

G_BEGIN_DECLS

#define BG_TYPE_WALLPAPERS_SOURCE bg_wallpapers_source_get_type()

#define BG_WALLPAPERS_SOURCE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  BG_TYPE_WALLPAPERS_SOURCE, BgWallpapersSource))

#define BG_WALLPAPERS_SOURCE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  BG_TYPE_WALLPAPERS_SOURCE, BgWallpapersSourceClass))

#define BG_IS_WALLPAPERS_SOURCE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  BG_TYPE_WALLPAPERS_SOURCE))

#define BG_IS_WALLPAPERS_SOURCE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  BG_TYPE_WALLPAPERS_SOURCE))

#define BG_WALLPAPERS_SOURCE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  BG_TYPE_WALLPAPERS_SOURCE, BgWallpapersSourceClass))

typedef struct _BgWallpapersSource BgWallpapersSource;
typedef struct _BgWallpapersSourceClass BgWallpapersSourceClass;
typedef struct _BgWallpapersSourcePrivate BgWallpapersSourcePrivate;

struct _BgWallpapersSource
{
  BgSource parent;

  BgWallpapersSourcePrivate *priv;
};

struct _BgWallpapersSourceClass
{
  BgSourceClass parent_class;
};

GType bg_wallpapers_source_get_type (void) G_GNUC_CONST;

BgWallpapersSource *bg_wallpapers_source_new (void);
GtkListStore       *bg_wallpapers_source_get_liststore (BgWallpapersSource *source);

G_END_DECLS

#endif /* _BG_WALLPAPERS_SOURCE_H */
