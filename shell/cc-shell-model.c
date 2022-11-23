/*
 * Copyright (c) 2009, 2010 Intel, Inc.
 * Copyright (c) 2010 Red Hat, Inc.
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

#include "cc-shell-model.h"
#include <string.h>

#define GNOME_SETTINGS_PANEL_ID_KEY "X-Cinnamon-Settings-Panel"
#define GNOME_SETTINGS_PANEL_CATEGORY GNOME_SETTINGS_PANEL_ID_KEY
#define GNOME_SETTINGS_PANEL_ID_KEYWORDS "Keywords"


G_DEFINE_TYPE (CcShellModel, cc_shell_model, GTK_TYPE_LIST_STORE)

static GdkPixbuf *
load_pixbuf_for_gicon (GIcon *icon)
{
  GtkIconTheme *theme;
  GtkIconInfo *icon_info;
  GdkPixbuf *pixbuf = NULL;
  GError *err = NULL;

  if (icon == NULL)
    return NULL;

  theme = gtk_icon_theme_get_default ();

  icon_info = gtk_icon_theme_lookup_by_gicon (theme, icon,
                                              48, GTK_ICON_LOOKUP_FORCE_SIZE);
  if (icon_info)
    {
      pixbuf = gtk_icon_info_load_icon (icon_info, &err);
      if (err)
        {
          g_warning ("Could not load icon '%s': %s",
                     gtk_icon_info_get_filename (icon_info), err->message);
          g_error_free (err);
        }

      gtk_icon_info_free (icon_info);
    }
  else
    {
      g_warning ("Could not find icon");
    }

  return pixbuf;
}

static void
icon_theme_changed (GtkIconTheme *theme,
                    CcShellModel *self)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  gboolean cont;

  model = GTK_TREE_MODEL (self);
  cont = gtk_tree_model_get_iter_first (model, &iter);
  while (cont)
    {
      GdkPixbuf *pixbuf;
      GIcon *icon;

      gtk_tree_model_get (model, &iter,
                          COL_GICON, &icon,
                          -1);
      pixbuf = load_pixbuf_for_gicon (icon);
      g_object_unref (icon);
      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                          COL_PIXBUF, pixbuf,
                          -1);

      cont = gtk_tree_model_iter_next (model, &iter);
    }
}

static void
cc_shell_model_class_init (CcShellModelClass *klass)
{
}

static void
cc_shell_model_init (CcShellModel *self)
{
  GType types[] = {G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
      GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_ICON, G_TYPE_STRV};

  gtk_list_store_set_column_types (GTK_LIST_STORE (self),
                                   N_COLS, types);

  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (self), COL_NAME,
                                        GTK_SORT_ASCENDING);

  g_signal_connect (G_OBJECT (gtk_icon_theme_get_default ()), "changed",
                    G_CALLBACK (icon_theme_changed), self);
}

CcShellModel *
cc_shell_model_new (void)
{
  return g_object_new (CC_TYPE_SHELL_MODEL, NULL);
}

void
cc_shell_model_add_item (CcShellModel   *model,
                         const gchar    *path)
{

  g_autoptr(GAppInfo) appinfo = G_APP_INFO (g_desktop_app_info_new_from_filename (path));
  GIcon       *icon = g_app_info_get_icon (appinfo);
  const gchar *name = g_app_info_get_name (appinfo);
  const gchar *comment = g_app_info_get_description (appinfo);
  g_autofree gchar *id;
  g_autofree gchar *basename;
  g_autoptr(GdkPixbuf) pixbuf = NULL;
  GKeyFile *key_file;
  gchar **keywords;

  /* load the .desktop file since gnome-menus doesn't have a way to read
   * custom properties from desktop files */

  key_file = g_key_file_new ();
  g_key_file_load_from_file (key_file, path, 0, NULL);

  id = g_key_file_get_string (key_file, "Desktop Entry",
                              GNOME_SETTINGS_PANEL_ID_KEY, NULL);

  keywords = g_key_file_get_locale_string_list (key_file, "Desktop Entry",
                                                GNOME_SETTINGS_PANEL_ID_KEYWORDS,
                                                NULL, NULL, NULL);

  g_key_file_free (key_file);
  key_file = NULL;

  pixbuf = load_pixbuf_for_gicon (icon);
  basename = g_path_get_basename (path);

  gtk_list_store_insert_with_values (GTK_LIST_STORE (model), NULL, 0,
                                     COL_NAME, name,
                                     COL_DESKTOP_FILE, basename,
                                     COL_ID, id,
                                     COL_PIXBUF, pixbuf,
                                     COL_CATEGORY, "C Modules",
                                     COL_DESCRIPTION, comment,
                                     COL_GICON, icon,
                                     COL_KEYWORDS, keywords,
                                     -1);

  g_strfreev (keywords);
}
