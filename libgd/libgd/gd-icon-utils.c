/*
 * Copyright (c) 2011, 2012, 2015 Red Hat, Inc.
 *
 * Gnome Documents is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * Gnome Documents is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with Gnome Documents; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Cosimo Cecchi <cosimoc@redhat.com>
 *
 */

#include "gd-icon-utils.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <string.h>
#include <math.h>

#define _BG_MIN_SIZE 20
#define _EMBLEM_MIN_SIZE 8

/**
 * gd_create_symbolic_icon_for_scale:
 * @name:
 * @base_size:
 * @scale:
 *
 * Returns: (transfer full):
 */
GIcon *
gd_create_symbolic_icon_for_scale (const gchar *name,
                                   gint base_size,
                                   gint scale)
{
  gchar *symbolic_name;
  GIcon *icon, *retval = NULL;
  cairo_surface_t *icon_surface;
  cairo_surface_t *surface;
  cairo_t *cr;
  GtkStyleContext *style;
  GtkWidgetPath *path;
  GdkPixbuf *pixbuf;
  GtkIconTheme *theme;
  GtkIconInfo *info;
  gint bg_size;
  gint emblem_size;
  gint total_size;
  gint total_size_scaled;

  total_size = base_size / 2;
  total_size_scaled = total_size * scale;

  bg_size = MAX (total_size / 2, _BG_MIN_SIZE);
  emblem_size = MAX (bg_size - 8, _EMBLEM_MIN_SIZE);

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, total_size_scaled, total_size_scaled);
  cairo_surface_set_device_scale (surface, (gdouble) scale, (gdouble) scale);
  cr = cairo_create (surface);

  style = gtk_style_context_new ();

  path = gtk_widget_path_new ();
  gtk_widget_path_append_type (path, GTK_TYPE_ICON_VIEW);
  gtk_style_context_set_path (style, path);
  gtk_widget_path_unref (path);

  gtk_style_context_add_class (style, "documents-icon-bg");

  gtk_render_background (style, cr, (total_size - bg_size) / 2, (total_size - bg_size) / 2, bg_size, bg_size);

  symbolic_name = g_strconcat (name, "-symbolic", NULL);
  icon = g_themed_icon_new_with_default_fallbacks (symbolic_name);
  g_free (symbolic_name);

  theme = gtk_icon_theme_get_default();
  info = gtk_icon_theme_lookup_by_gicon_for_scale (theme, icon, emblem_size, scale,
                                                   GTK_ICON_LOOKUP_FORCE_SIZE);
  g_object_unref (icon);

  if (info == NULL)
    goto out;

  pixbuf = gtk_icon_info_load_symbolic_for_context (info, style, NULL, NULL);
  g_object_unref (info);

  if (pixbuf == NULL)
    goto out;

  icon_surface = gdk_cairo_surface_create_from_pixbuf (pixbuf, scale, NULL);
  g_object_unref (pixbuf);

  gtk_render_icon_surface (style, cr, icon_surface, (total_size - emblem_size) / 2,  (total_size - emblem_size) / 2);
  cairo_surface_destroy (icon_surface);

  retval = G_ICON (gdk_pixbuf_get_from_surface (surface, 0, 0, total_size_scaled, total_size_scaled));

 out:
  g_object_unref (style);
  cairo_surface_destroy (surface);
  cairo_destroy (cr);

  return retval;
}

/**
 * gd_create_symbolic_icon:
 * @name:
 * @base_size:
 *
 * Returns: (transfer full):
 */
GIcon *
gd_create_symbolic_icon (const gchar *name,
                         gint base_size)
{
  return gd_create_symbolic_icon_for_scale (name, base_size, 1);
}

/**
 * gd_embed_surface_in_frame:
 * @source_image:
 * @frame_image_url:
 * @slice_width:
 * @border_width:
 *
 * Returns: (transfer full):
 */
cairo_surface_t *
gd_embed_surface_in_frame (cairo_surface_t *source_image,
                           const gchar *frame_image_url,
                           GtkBorder *slice_width,
                           GtkBorder *border_width)
{
  cairo_surface_t *surface;
  cairo_t *cr;
  int source_width, source_height;
  gchar *css_str;
  GtkCssProvider *provider;
  GtkStyleContext *context;
  GError *error = NULL;
  GdkPixbuf *retval;
  GtkWidgetPath *path;
  gdouble scale_x, scale_y;

  cairo_surface_get_device_scale (source_image, &scale_x, &scale_y);

  source_width = cairo_image_surface_get_width (source_image) / (gint) floor (scale_x),
  source_height = cairo_image_surface_get_height (source_image) / (gint) floor (scale_y);

  css_str = g_strdup_printf (".embedded-image { border-image: url(\"%s\") %d %d %d %d / %dpx %dpx %dpx %dpx }",
                             frame_image_url,
                             slice_width->top, slice_width->right, slice_width->bottom, slice_width->left,
                             border_width->top, border_width->right, border_width->bottom, border_width->left);
  provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_data (provider, css_str, -1, &error);

  if (error != NULL)
    {
      g_warning ("Unable to create the thumbnail frame image: %s", error->message);
      g_error_free (error);
      g_free (css_str);

      return g_object_ref (source_image);
    }

  surface = cairo_surface_create_similar (source_image,
                                          CAIRO_CONTENT_COLOR_ALPHA,
                                          source_width, source_height);
  cr = cairo_create (surface);

  context = gtk_style_context_new ();
  path = gtk_widget_path_new ();
  gtk_widget_path_append_type (path, GTK_TYPE_ICON_VIEW);

  gtk_style_context_set_path (context, path);
  gtk_style_context_add_provider (context, GTK_STYLE_PROVIDER (provider), 600);

  cairo_save (cr);
  cairo_rectangle (cr,
		   border_width->left,
		   border_width->top,
		   source_width - border_width->left - border_width->right,
		   source_height - border_width->top - border_width->bottom);
  cairo_clip (cr);
  gtk_render_icon_surface (context, cr,
                           source_image,
                           0, 0);
  cairo_restore (cr);

  gtk_style_context_save (context);
  gtk_style_context_add_class (context, "embedded-image");

  gtk_render_frame (context, cr,
                    0, 0,
                    source_width, source_height);

  gtk_style_context_restore (context);
  cairo_destroy (cr);

  gtk_widget_path_unref (path);
  g_object_unref (provider);
  g_object_unref (context);
  g_free (css_str);

  return surface;
}

/**
 * gd_embed_image_in_frame:
 * @source_image:
 * @frame_image_url:
 * @slice_width:
 * @border_width:
 *
 * Returns: (transfer full):
 */
GdkPixbuf *
gd_embed_image_in_frame (GdkPixbuf *source_image,
                         const gchar *frame_image_url,
                         GtkBorder *slice_width,
                         GtkBorder *border_width)
{
  cairo_surface_t *surface, *embedded_surface;
  GdkPixbuf *retval;

  surface = gdk_cairo_surface_create_from_pixbuf (source_image,
                                                  0, NULL);

  /* Force the device scale to 1.0, since pixbufs are always in unscaled
   * dimensions.
   */
  cairo_surface_set_device_scale (surface, 1.0, 1.0);
  embedded_surface = gd_embed_surface_in_frame (surface, frame_image_url,
                                                slice_width, border_width);
  retval = gdk_pixbuf_get_from_surface (embedded_surface,
                                        0, 0,
                                        cairo_image_surface_get_width (embedded_surface),
                                        cairo_image_surface_get_height (embedded_surface));

  cairo_surface_destroy (embedded_surface);
  cairo_surface_destroy (surface);

  return retval;
}
