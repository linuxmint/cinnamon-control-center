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

#include "gd-header-bar.h"

#include <string.h>

/* TODO
 * - wm communication
 */

#define DEFAULT_SPACING 8
#define DEFAULT_HPADDING 8
#define DEFAULT_VPADDING 6

struct _GdHeaderBarPrivate
{
  gchar *title;
  gchar *subtitle;

  GtkWidget *title_label;
  GtkWidget *subtitle_label;
  GtkWidget *labels_box;
  GtkWidget *labels_sizing_box;

  GtkWidget *custom_title;
  GdkWindow *event_window;
  gint spacing;
  gint hpadding;
  gint vpadding;

  GList *children;
};

typedef struct _Child Child;
struct _Child
{
  GtkWidget *widget;
  GtkPackType pack_type;
};

enum {
  PROP_0,
  PROP_TITLE,
  PROP_SUBTITLE,
  PROP_CUSTOM_TITLE,
  PROP_SPACING,
  PROP_HPADDING,
  PROP_VPADDING
};

enum {
  CHILD_PROP_0,
  CHILD_PROP_PACK_TYPE,
  CHILD_PROP_POSITION
};

static void gd_header_buildable_init (GtkBuildableIface *iface);

G_DEFINE_TYPE_WITH_CODE (GdHeaderBar, gd_header_bar, GTK_TYPE_CONTAINER,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_BUILDABLE,
                                                gd_header_buildable_init));

static void
boldify_label (GtkWidget *label)
{
  PangoAttrList *attrs;
  attrs = pango_attr_list_new ();
  pango_attr_list_insert (attrs, pango_attr_weight_new (PANGO_WEIGHT_BOLD));
  gtk_label_set_attributes (GTK_LABEL (label), attrs);
  pango_attr_list_unref (attrs);
}

static void
smallify_label (GtkWidget *label)
{
  PangoAttrList *attrs;
  attrs = pango_attr_list_new ();
  pango_attr_list_insert (attrs, pango_attr_scale_new (PANGO_SCALE_SMALL));
  gtk_label_set_attributes (GTK_LABEL (label), attrs);
  pango_attr_list_unref (attrs);

  gtk_style_context_add_class (gtk_widget_get_style_context (label), "dim-label");
}

static void
get_css_padding_and_border (GtkWidget *widget,
                            GtkBorder *border)
{
  GtkStyleContext *context;
  GtkStateFlags state;
  GtkBorder tmp;

  context = gtk_widget_get_style_context (widget);
  state = gtk_widget_get_state_flags (widget);

  gtk_style_context_get_padding (context, state, border);
  gtk_style_context_get_border (context, state, &tmp);
  border->top += tmp.top;
  border->right += tmp.right;
  border->bottom += tmp.bottom;
  border->left += tmp.left;
}

static void
init_sizing_box (GdHeaderBar *bar)
{
  GdHeaderBarPrivate *priv = bar->priv;
  GtkWidget *w;

  /* We use this box to always request size for the two labels (title and subtitle)
   * as if they were always visible, but then allocate the real label box
   * with its actual size, to keep it center-aligned in case we have only the title.
   */
  priv->labels_sizing_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

  w = gtk_label_new (NULL);
  boldify_label (w);
  gtk_box_pack_start (GTK_BOX (priv->labels_sizing_box), w, FALSE, FALSE, 0);
  gtk_label_set_line_wrap (GTK_LABEL (w), FALSE);
  gtk_label_set_single_line_mode (GTK_LABEL (w), TRUE);
  gtk_label_set_ellipsize (GTK_LABEL (w), PANGO_ELLIPSIZE_END);

  w = gtk_label_new (NULL);
  smallify_label (w);
  gtk_box_pack_start (GTK_BOX (priv->labels_sizing_box), w, FALSE, FALSE, 0);
  gtk_label_set_line_wrap (GTK_LABEL (w), FALSE);
  gtk_label_set_single_line_mode (GTK_LABEL (w), TRUE);
  gtk_label_set_ellipsize (GTK_LABEL (w), PANGO_ELLIPSIZE_END);

  gtk_widget_show_all (priv->labels_sizing_box);
}

static void
gd_header_bar_init (GdHeaderBar *bar)
{
  GtkStyleContext *context;
  GdHeaderBarPrivate *priv;

  priv = G_TYPE_INSTANCE_GET_PRIVATE (bar, GD_TYPE_HEADER_BAR, GdHeaderBarPrivate);
  bar->priv = priv;

  gtk_widget_set_has_window (GTK_WIDGET (bar), FALSE);
  gtk_widget_set_redraw_on_allocate (GTK_WIDGET (bar), FALSE);

  priv->labels_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_parent (priv->labels_box, GTK_WIDGET (bar));
  gtk_widget_set_valign (priv->labels_box, GTK_ALIGN_CENTER);
  gtk_widget_show (priv->labels_box);

  init_sizing_box (bar);

  priv->title_label = gtk_label_new ("");
  boldify_label (priv->title_label);
  gtk_box_pack_start (GTK_BOX (priv->labels_box), priv->title_label, FALSE, FALSE, 0);
  gtk_label_set_line_wrap (GTK_LABEL (priv->title_label), FALSE);
  gtk_label_set_single_line_mode (GTK_LABEL (priv->title_label), TRUE);
  gtk_label_set_ellipsize (GTK_LABEL (priv->title_label), PANGO_ELLIPSIZE_END);
  gtk_widget_show (priv->title_label);

  priv->subtitle_label = gtk_label_new ("");
  smallify_label (priv->subtitle_label);
  gtk_box_pack_start (GTK_BOX (priv->labels_box), priv->subtitle_label, FALSE, FALSE, 0);
  gtk_label_set_line_wrap (GTK_LABEL (priv->subtitle_label), FALSE);
  gtk_label_set_single_line_mode (GTK_LABEL (priv->subtitle_label), TRUE);
  gtk_label_set_ellipsize (GTK_LABEL (priv->subtitle_label), PANGO_ELLIPSIZE_END);

  priv->title = NULL;
  priv->custom_title = NULL;
  priv->children = NULL;
  priv->spacing = DEFAULT_SPACING;
  priv->hpadding = DEFAULT_HPADDING;
  priv->vpadding = DEFAULT_VPADDING;

  context = gtk_widget_get_style_context (GTK_WIDGET (bar));
  gtk_style_context_add_class (context, "header-bar");
  gtk_style_context_add_class (context, GTK_STYLE_CLASS_HORIZONTAL);
}

static gint
count_visible_children (GdHeaderBar *bar)
{
  GList *l;
  Child *child;
  gint n;

  n = 0;
  for (l = bar->priv->children; l; l = l->next)
    {
      child = l->data;
      if (gtk_widget_get_visible (child->widget))
        n++;
    }

  return n;
}

static void
gd_header_bar_get_size (GtkWidget      *widget,
                        GtkOrientation  orientation,
                        gint           *minimum_size,
                        gint           *natural_size)
{
  GdHeaderBar *bar = GD_HEADER_BAR (widget);
  GdHeaderBarPrivate *priv = bar->priv;
  GList *l;
  gint nvis_children;
  gint minimum, natural;
  GtkBorder css_borders;

  minimum = natural = 0;
  nvis_children = 0;

  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    gtk_widget_get_preferred_width (priv->labels_sizing_box,
                                    &minimum, &natural);
  else
    gtk_widget_get_preferred_height (priv->labels_sizing_box,
                                     &minimum, &natural);

  for (l = priv->children; l; l = l->next)
    {
      Child *child = l->data;

      if (gtk_widget_get_visible (child->widget))
        {
          gint child_minimum, child_natural;

          if (orientation == GTK_ORIENTATION_HORIZONTAL)
            gtk_widget_get_preferred_width (child->widget,
                                            &child_minimum, &child_natural);
          else
            gtk_widget_get_preferred_height (child->widget,
                                             &child_minimum, &child_natural);

          if (GTK_ORIENTATION_HORIZONTAL == orientation)
            {
              minimum += child_minimum;
              natural += child_natural;
            }
          else
            {
              minimum = MAX (minimum, child_minimum);
              natural = MAX (natural, child_natural);
            }
          nvis_children += 1;
        }
    }

  if (nvis_children > 0 && orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      minimum += nvis_children * priv->spacing;
      natural += nvis_children * priv->spacing;
    }

  get_css_padding_and_border (widget, &css_borders);

  if (GTK_ORIENTATION_HORIZONTAL == orientation)
    {
      minimum += 2 * priv->hpadding + css_borders.left + css_borders.right;
      natural += 2 * priv->hpadding + css_borders.left + css_borders.right;
    }
  else
    {
      minimum += 2 * priv->vpadding + css_borders.top + css_borders.bottom;
      natural += 2 * priv->vpadding + css_borders.top + css_borders.bottom;
    }

  if (minimum_size)
    *minimum_size = minimum;

  if (natural_size)
    *natural_size = natural;
}

static void
gd_header_bar_compute_size_for_orientation (GtkWidget *widget,
                                            gint       avail_size,
                                            gint      *minimum_size,
                                            gint      *natural_size)
{
  GdHeaderBar *bar = GD_HEADER_BAR (widget);
  GdHeaderBarPrivate *priv = bar->priv;
  GList *children;
  gint required_size = 0;
  gint required_natural = 0;
  gint child_size;
  gint child_natural;
  gint nvis_children;
  GtkBorder css_borders;

  avail_size -= 2 * priv->vpadding;

  nvis_children = 0;

  for (children = priv->children; children != NULL; children = children->next)
    {
      Child *child = children->data;

      if (gtk_widget_get_visible (child->widget))
        {
          gtk_widget_get_preferred_width_for_height (child->widget,
                                                     avail_size, &child_size, &child_natural);

          required_size += child_size;
          required_natural += child_natural;

          nvis_children += 1;
        }
    }

  gtk_widget_get_preferred_width (priv->labels_sizing_box,
                                  &child_size, &child_natural);
  required_size += child_size;
  required_natural += child_natural;

  if (nvis_children > 0)
    {
      required_size += nvis_children * priv->spacing;
      required_natural += nvis_children * priv->spacing;
    }

  get_css_padding_and_border (widget, &css_borders);

  required_size += 2 * priv->hpadding + css_borders.left + css_borders.right;
  required_natural += 2 * priv->hpadding + css_borders.left + css_borders.right;

  if (minimum_size)
    *minimum_size = required_size;

  if (natural_size)
    *natural_size = required_natural;
}

static void
gd_header_bar_compute_size_for_opposing_orientation (GtkWidget *widget,
                                                     gint       avail_size,
                                                     gint      *minimum_size,
                                                     gint      *natural_size)
{
  GdHeaderBar *bar = GD_HEADER_BAR (widget);
  GdHeaderBarPrivate *priv = bar->priv;
  Child *child;
  GList *children;
  gint nvis_children;
  gint computed_minimum = 0;
  gint computed_natural = 0;
  GtkRequestedSize *sizes;
  GtkPackType packing;
  gint size;
  gint i;
  gint child_size;
  gint child_minimum;
  gint child_natural;
  GtkBorder css_borders;

  nvis_children = count_visible_children (bar);

  if (nvis_children <= 0)
    return;

  sizes = g_newa (GtkRequestedSize, nvis_children);
  size = avail_size - 2 * priv->hpadding;

  /* Retrieve desired size for visible children */
  for (i = 0, children = priv->children; children; children = children->next)
    {
      child = children->data;

      if (gtk_widget_get_visible (child->widget))
        {
          gtk_widget_get_preferred_width (child->widget,
                                          &sizes[i].minimum_size,
                                          &sizes[i].natural_size);

          size -= sizes[i].minimum_size;
          sizes[i].data = child;
          i += 1;
        }
    }

  /* Bring children up to size first */
  size = gtk_distribute_natural_allocation (MAX (0, size), nvis_children, sizes);

  /* Allocate child positions. */
  for (packing = GTK_PACK_START; packing <= GTK_PACK_END; ++packing)
    {
      for (i = 0, children = priv->children; children; children = children->next)
        {
          child = children->data;

          /* If widget is not visible, skip it. */
          if (!gtk_widget_get_visible (child->widget))
            continue;

          /* If widget is packed differently skip it, but still increment i,
           * since widget is visible and will be handled in next loop
           * iteration.
           */
          if (child->pack_type != packing)
            {
              i++;
              continue;
            }

          child_size = sizes[i].minimum_size;

          gtk_widget_get_preferred_height_for_width (child->widget,
                                                     child_size, &child_minimum, &child_natural);

          computed_minimum = MAX (computed_minimum, child_minimum);
          computed_natural = MAX (computed_natural, child_natural);
        }
      i += 1;
    }

  gtk_widget_get_preferred_height (priv->labels_sizing_box,
                                   &child_minimum, &child_natural);
  computed_minimum = MAX (computed_minimum, child_minimum);
  computed_natural = MAX (computed_natural, child_natural);

  get_css_padding_and_border (widget, &css_borders);

  computed_minimum += 2 * priv->vpadding + css_borders.top + css_borders.bottom;
  computed_natural += 2 * priv->vpadding + css_borders.top + css_borders.bottom;

  if (minimum_size)
    *minimum_size = computed_minimum;

  if (natural_size)
    *natural_size = computed_natural;
}

static void
gd_header_bar_get_preferred_width (GtkWidget *widget,
                                   gint      *minimum_size,
                                   gint      *natural_size)
{
  gd_header_bar_get_size (widget, GTK_ORIENTATION_HORIZONTAL, minimum_size, natural_size);
}

static void
gd_header_bar_get_preferred_height (GtkWidget *widget,
                                    gint      *minimum_size,
                                    gint      *natural_size)
{
  gd_header_bar_get_size (widget, GTK_ORIENTATION_VERTICAL, minimum_size, natural_size);
}

static void
gd_header_bar_get_preferred_width_for_height (GtkWidget *widget,
                                              gint       height,
                                              gint      *minimum_width,
                                              gint      *natural_width)
{
  gd_header_bar_compute_size_for_orientation (widget, height, minimum_width, natural_width);
}

static void
gd_header_bar_get_preferred_height_for_width (GtkWidget *widget,
                                              gint       width,
                                              gint      *minimum_height,
                                              gint      *natural_height)
{
  gd_header_bar_compute_size_for_opposing_orientation (widget, width, minimum_height, natural_height);
}

static void
gd_header_bar_size_allocate (GtkWidget     *widget,
                             GtkAllocation *allocation)
{
  GdHeaderBar *bar = GD_HEADER_BAR (widget);
  GdHeaderBarPrivate *priv = bar->priv;
  GtkRequestedSize *sizes;
  gint width, height;
  gint nvis_children;
  gint title_minimum_size;
  gint title_natural_size;
  gint side[2];
  GList *l;
  gint i;
  Child *child;
  GtkPackType packing;
  GtkAllocation child_allocation;
  gint x;
  gint child_size;
  GtkTextDirection direction;
  GtkBorder css_borders;

  gtk_widget_set_allocation (widget, allocation);

  if (gtk_widget_get_realized (widget))
    gdk_window_move_resize (priv->event_window,
                            allocation->x,
                            allocation->y,
                            allocation->width,
                            allocation->height);

  direction = gtk_widget_get_direction (widget);
  nvis_children = count_visible_children (bar);
  sizes = g_newa (GtkRequestedSize, nvis_children);

  get_css_padding_and_border (widget, &css_borders);
  width = allocation->width - nvis_children * priv->spacing -
    2 * priv->hpadding - css_borders.left - css_borders.right;
  height = allocation->height - 2 * priv->vpadding - css_borders.top - css_borders.bottom;

  i = 0;
  for (l = priv->children; l; l = l->next)
    {
      child = l->data;
      if (!gtk_widget_get_visible (child->widget))
        continue;

      gtk_widget_get_preferred_width_for_height (child->widget,
                                                 height,
                                                 &sizes[i].minimum_size,
                                                 &sizes[i].natural_size);
      width -= sizes[i].minimum_size;
      i++;
    }

  if (priv->custom_title)
    {
      gtk_widget_get_preferred_width_for_height (priv->custom_title,
                                                 height,
                                                 &title_minimum_size,
                                                 &title_natural_size);
    }
  else
    {
      gtk_widget_get_preferred_width_for_height (priv->labels_box,
                                                 height,
                                                 &title_minimum_size,
                                                 &title_natural_size);
    }
  width -= title_natural_size;

  width = gtk_distribute_natural_allocation (MAX (0, width), nvis_children, sizes);

  side[0] = side[1] = 0;
  for (packing = GTK_PACK_START; packing <= GTK_PACK_END; packing++)
    {
      child_allocation.y = allocation->y + priv->vpadding + css_borders.top;
      child_allocation.height = height;
      if (packing == GTK_PACK_START)
        x = allocation->x + priv->hpadding + css_borders.left;
      else
        x = allocation->x + allocation->width - priv->hpadding - css_borders.right;

      if (packing == GTK_PACK_START)
	{
	  l = priv->children;
	  i = 0;
	}
      else
	{
	  l = g_list_last (priv->children);
	  i = nvis_children - 1;
	}

      for (; l != NULL; (packing == GTK_PACK_START) ? (l = l->next) : (l = l->prev))
        {
          child = l->data;
          if (!gtk_widget_get_visible (child->widget))
            continue;

          if (child->pack_type != packing)
            goto next;

          child_size = sizes[i].minimum_size;

          child_allocation.width = child_size;

          if (packing == GTK_PACK_START)
            {
              child_allocation.x = x;
              x += child_size;
              x += priv->spacing;
            }
          else
            {
              x -= child_size;
              child_allocation.x = x;
              x -= priv->spacing;
            }

          side[packing] += child_size + priv->spacing;

          if (direction == GTK_TEXT_DIR_RTL)
            child_allocation.x = allocation->x + allocation->width - (child_allocation.x - allocation->x) - child_allocation.width;

          gtk_widget_size_allocate (child->widget, &child_allocation);

        next:
          if (packing == GTK_PACK_START)
            i++;
          else
            i--;
        }
    }

  child_allocation.y = allocation->y + priv->vpadding + css_borders.top;
  child_allocation.height = height;

  width = MAX(side[0], side[1]);

  if (allocation->width - 2 * width >= title_natural_size)
    child_size = MIN (title_natural_size, allocation->width - 2 * width);
  else if (allocation->width - side[0] - side[1] >= title_natural_size)
    child_size = MIN (title_natural_size, allocation->width - side[0] - side[1]);
  else
    child_size = allocation->width - side[0] - side[1];

  child_allocation.x = allocation->x + (allocation->width - child_size) / 2;
  child_allocation.width = child_size;

  if (allocation->x + side[0] > child_allocation.x)
    child_allocation.x = allocation->x + side[0];
  else if (allocation->x + allocation->width - side[1] < child_allocation.x + child_allocation.width)
    child_allocation.x = allocation->x + allocation->width - side[1] - child_allocation.width;

  if (direction == GTK_TEXT_DIR_RTL)
    child_allocation.x = allocation->x + allocation->width - (child_allocation.x - allocation->x) - child_allocation.width;

  if (priv->custom_title)
    gtk_widget_size_allocate (priv->custom_title, &child_allocation);
  else
    gtk_widget_size_allocate (priv->labels_box, &child_allocation);
}

/**
 * gd_header_bar_set_title:
 * @bar: a #GdHeaderBar
 * @title: (allow-none): a title
 *
 * Sets the title of the #GdHeaderBar. The title should help a user
 * identify the current view. A good title should not include the
 * application name.
 *
 **/
void
gd_header_bar_set_title (GdHeaderBar *bar,
                         const gchar *title)
{
  GdHeaderBarPrivate *priv;
  char *new_title;

  g_return_if_fail (GD_IS_HEADER_BAR (bar));

  priv = bar->priv;

  new_title = g_strdup (title);
  g_free (priv->title);
  priv->title = new_title;

  gtk_label_set_label (GTK_LABEL (priv->title_label), priv->title);
  gtk_widget_queue_resize (GTK_WIDGET (bar));

  g_object_notify (G_OBJECT (bar), "title");
}

/**
 * gd_header_bar_get_title:
 * @bar: a #GdHeaderBar
 *
 * Retrieves the title of the header. See gd_header_bar_set_title().
 *
 * Return value: the title of the header, or %NULL if none has
 *    been set explicitely. The returned string is owned by the widget
 *    and must not be modified or freed.
 **/
const gchar *
gd_header_bar_get_title (GdHeaderBar *bar)
{
  g_return_val_if_fail (GD_IS_HEADER_BAR (bar), NULL);

  return bar->priv->title;
}

/**
 * gd_header_bar_set_subtitle:
 * @bar: a #GdHeaderBar
 * @subtitle: (allow-none): a subtitle
 *
 * Sets the subtitle of the #GdHeaderBar. The subtitle should give a user
 * an additional detail to help him identify the current view.
 *
 **/
void
gd_header_bar_set_subtitle (GdHeaderBar *bar,
                            const gchar *subtitle)
{
  GdHeaderBarPrivate *priv;
  char *new_subtitle;

  g_return_if_fail (GD_IS_HEADER_BAR (bar));

  priv = bar->priv;

  new_subtitle = g_strdup (subtitle);
  g_free (priv->subtitle);
  priv->subtitle = new_subtitle;

  gtk_label_set_label (GTK_LABEL (priv->subtitle_label), priv->subtitle);
  gtk_widget_set_visible (priv->subtitle_label, (priv->subtitle != NULL));

  gtk_widget_queue_resize (GTK_WIDGET (bar));

  g_object_notify (G_OBJECT (bar), "subtitle");
}

/**
 * gd_header_bar_get_subtitle:
 * @bar: a #GdHeaderBar
 *
 * Retrieves the subtitle of the header. See gd_header_bar_set_subtitle().
 *
 * Return value: the subtitle of the header, or %NULL if none has
 *    been set explicitely. The returned string is owned by the widget
 *    and must not be modified or freed.
 **/
const gchar *
gd_header_bar_get_subtitle (GdHeaderBar *bar)
{
  g_return_val_if_fail (GD_IS_HEADER_BAR (bar), NULL);

  return bar->priv->subtitle;
}

/**
 * gd_header_bar_set_custom_title:
 * @bar: a #GdHeaderBar
 * @title_widget: (allow-none): a custom widget to use for a title
 *
 * Sets a custom title for the #GdHeaderBar. The title should help a
 * user identify the current view. This supercedes any title set by
 * gd_header_bar_set_title(). You should set the custom title to %NULL,
 * for the header title label to be visible again.
 *
 **/
void
gd_header_bar_set_custom_title (GdHeaderBar *bar,
                                GtkWidget   *title_widget)
{
  GdHeaderBarPrivate *priv;

  g_return_if_fail (GD_IS_HEADER_BAR (bar));
  if (title_widget)
    g_return_if_fail (GTK_IS_WIDGET (title_widget));

  priv = bar->priv;

  /* No need to do anything if the custom widget stays the same */
  if (priv->custom_title == title_widget)
    return;

  if (priv->custom_title)
    {
      GtkWidget *custom = priv->custom_title;

      priv->custom_title = NULL;
      gtk_widget_unparent (custom);
      g_object_unref (custom);
    }

  if (title_widget)
    {
      priv->custom_title = g_object_ref (title_widget);

      gtk_widget_hide (priv->labels_box);

      gtk_widget_set_parent (priv->custom_title, GTK_WIDGET (bar));
      gtk_widget_set_valign (priv->custom_title, GTK_ALIGN_CENTER);

      gtk_widget_show (title_widget);
    }
  else
    {
      gtk_widget_show (priv->labels_box);
    }

  gtk_widget_queue_resize (GTK_WIDGET (bar));

  g_object_notify (G_OBJECT (bar), "custom-title");
}

/**
 * gd_header_bar_get_custom_title:
 * @bar: a #GdHeaderBar
 *
 * Retrieves the custom title widget of the header. See
 * gd_header_bar_set_custom_title().
 *
 * Return value: (transfer none): the custom title widget of the header, or %NULL if
 *    none has been set explicitely.
 **/
GtkWidget *
gd_header_bar_get_custom_title (GdHeaderBar *bar)
{
  g_return_val_if_fail (GD_IS_HEADER_BAR (bar), NULL);

  return bar->priv->custom_title;
}

static void
gd_header_bar_finalize (GObject *object)
{
  GdHeaderBar *bar = GD_HEADER_BAR (object);
  GdHeaderBarPrivate *priv = bar->priv;

  g_free (priv->title);
  g_free (priv->subtitle);

  g_list_free (priv->children);

  G_OBJECT_CLASS (gd_header_bar_parent_class)->finalize (object);
}

static void
gd_header_bar_get_property (GObject      *object,
                            guint         prop_id,
                            GValue       *value,
                            GParamSpec   *pspec)
{
  GdHeaderBar *bar = GD_HEADER_BAR (object);
  GdHeaderBarPrivate *priv = bar->priv;

  switch (prop_id)
    {
    case PROP_TITLE:
      g_value_set_string (value, priv->title);
      break;

    case PROP_SUBTITLE:
      g_value_set_string (value, priv->subtitle);
      break;

    case PROP_CUSTOM_TITLE:
      g_value_set_object (value, priv->custom_title);
      break;

    case PROP_SPACING:
      g_value_set_int (value, priv->spacing);
      break;

    case PROP_HPADDING:
      g_value_set_int (value, priv->hpadding);
      break;

    case PROP_VPADDING:
      g_value_set_int (value, priv->vpadding);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gd_header_bar_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  GdHeaderBar *bar = GD_HEADER_BAR (object);
  GdHeaderBarPrivate *priv = bar->priv;

  switch (prop_id)
    {
    case PROP_TITLE:
      gd_header_bar_set_title (bar, g_value_get_string (value));
      break;

    case PROP_SUBTITLE:
      gd_header_bar_set_subtitle (bar, g_value_get_string (value));
      break;

    case PROP_CUSTOM_TITLE:
      gd_header_bar_set_custom_title (bar, g_value_get_object (value));
      break;

    case PROP_SPACING:
      priv->spacing = g_value_get_int (value);
      gtk_widget_queue_resize (GTK_WIDGET (bar));
      break;

    case PROP_HPADDING:
      priv->hpadding = g_value_get_int (value);
      gtk_widget_queue_resize (GTK_WIDGET (bar));
      break;

    case PROP_VPADDING:
      priv->vpadding = g_value_get_int (value);
      gtk_widget_queue_resize (GTK_WIDGET (bar));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gd_header_bar_pack (GdHeaderBar *bar,
                    GtkWidget   *widget,
                    GtkPackType  pack_type)
{
  Child *child;

  g_return_if_fail (gtk_widget_get_parent (widget) == NULL);

  child = g_new (Child, 1);
  child->widget = widget;
  child->pack_type = pack_type;

  bar->priv->children = g_list_append (bar->priv->children, child);

  gtk_widget_freeze_child_notify (widget);
  gtk_widget_set_parent (widget, GTK_WIDGET (bar));
  gtk_widget_child_notify (widget, "pack-type");
  gtk_widget_child_notify (widget, "position");
  gtk_widget_thaw_child_notify (widget);
}

static void
gd_header_bar_add (GtkContainer *container,
                   GtkWidget    *child)
{
  gd_header_bar_pack (GD_HEADER_BAR (container), child, GTK_PACK_START);
}

static GList *
find_child_link (GdHeaderBar *bar,
                 GtkWidget   *widget)
{
  GList *l;
  Child *child;

  for (l = bar->priv->children; l; l = l->next)
    {
      child = l->data;
      if (child->widget == widget)
        return l;
    }

  return NULL;
}

static void
gd_header_bar_remove (GtkContainer *container,
                      GtkWidget    *widget)
{
  GdHeaderBar *bar = GD_HEADER_BAR (container);
  GList *l;
  Child *child;

  l = find_child_link (bar, widget);
  if (l)
    {
      child = l->data;
      gtk_widget_unparent (child->widget);
      bar->priv->children = g_list_remove_link (bar->priv->children, l);
      g_free (child);
      gtk_widget_queue_resize (GTK_WIDGET (container));
    }
}

static void
gd_header_bar_forall (GtkContainer *container,
                      gboolean      include_internals,
                      GtkCallback   callback,
                      gpointer      callback_data)
{
  GdHeaderBar *bar = GD_HEADER_BAR (container);
  GdHeaderBarPrivate *priv = bar->priv;
  Child *child;
  GList *children;

  children = priv->children;
  while (children)
    {
      child = children->data;
      children = children->next;
      if (child->pack_type == GTK_PACK_START)
        (* callback) (child->widget, callback_data);
    }

  if (include_internals)
    {
      if (priv->custom_title)
        (* callback) (priv->custom_title, callback_data);
      else
        (* callback) (priv->labels_box, callback_data);
    }

  children = g_list_last (priv->children);
  while (children)
    {
      child = children->data;
      children = children->prev;
      if (child->pack_type == GTK_PACK_END)
        (* callback) (child->widget, callback_data);
    }
}

static GType
gd_header_bar_child_type (GtkContainer *container)
{
  return GTK_TYPE_WIDGET;
}

static void
gd_header_bar_get_child_property (GtkContainer *container,
                                  GtkWidget    *widget,
                                  guint         property_id,
                                  GValue       *value,
                                  GParamSpec   *pspec)
{
  GList *l;
  Child *child;

  l = find_child_link (GD_HEADER_BAR (container), widget);
  child = l->data;

  switch (property_id)
    {
    case CHILD_PROP_PACK_TYPE:
      g_value_set_enum (value, child->pack_type);
      break;

    case CHILD_PROP_POSITION:
      g_value_set_int (value, g_list_position (GD_HEADER_BAR (container)->priv->children, l));
      break;

    default:
      GTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }
}

static void
gd_header_bar_set_child_property (GtkContainer *container,
                                  GtkWidget    *widget,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  GList *l;
  Child *child;

  l = find_child_link (GD_HEADER_BAR (container), widget);
  child = l->data;

  switch (property_id)
    {
    case CHILD_PROP_PACK_TYPE:
      child->pack_type = g_value_get_enum (value);
      break;
    default:
      GTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }
}

static gboolean
gd_header_bar_button_press (GtkWidget      *toolbar,
                            GdkEventButton *event)
{
  GtkWidget *window;

  if (gdk_event_triggers_context_menu ((GdkEvent *) event))
    {

      /* FIXME menu ? */

      return FALSE;
    }

  if (event->type != GDK_BUTTON_PRESS)
    return FALSE;

  window = gtk_widget_get_toplevel (toolbar);

  if (window)
    {
      gtk_window_begin_move_drag (GTK_WINDOW (window),
                                  event->button,
                                  event->x_root,
                                  event->y_root,
                                  event->time);

      return TRUE;
    }

  return FALSE;
}

static void
gd_header_bar_realize (GtkWidget *widget)
{
  GdHeaderBar *bar = GD_HEADER_BAR (widget);
  GdHeaderBarPrivate *priv = bar->priv;
  GtkAllocation allocation;
  GdkWindow *window;
  GdkWindowAttr attributes;
  gint attributes_mask;

  gtk_widget_get_allocation (widget, &allocation);
  gtk_widget_set_realized (widget, TRUE);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.wclass = GDK_INPUT_ONLY;
  attributes.event_mask = gtk_widget_get_events (widget);
  attributes.event_mask |= (GDK_BUTTON_PRESS_MASK |
                            GDK_BUTTON_RELEASE_MASK |
                            GDK_TOUCH_MASK);
  attributes_mask = GDK_WA_X | GDK_WA_Y;

  window = gtk_widget_get_parent_window (widget);
  gtk_widget_set_window (widget, window);
  g_object_ref (window);

  priv->event_window = gdk_window_new (window,
                                       &attributes, attributes_mask);
  gdk_window_set_user_data (priv->event_window, widget);
}
static void
gd_header_bar_unrealize (GtkWidget *widget)
{
  GdHeaderBar *bar = GD_HEADER_BAR (widget);
  GdHeaderBarPrivate *priv = bar->priv;

  if (priv->event_window)
    {
      gdk_window_set_user_data (priv->event_window, NULL);
      gdk_window_destroy (priv->event_window);
      priv->event_window = NULL;
    }

  GTK_WIDGET_CLASS (gd_header_bar_parent_class)->unrealize (widget);
}

static void
gd_header_bar_map (GtkWidget *widget)
{
  GdHeaderBar *bar = GD_HEADER_BAR (widget);
  GdHeaderBarPrivate *priv = bar->priv;

  GTK_WIDGET_CLASS (gd_header_bar_parent_class)->map (widget);

  if (priv->event_window)
    gdk_window_show_unraised (priv->event_window);
}

static void
gd_header_bar_unmap (GtkWidget *widget)
{
  GdHeaderBar *bar = GD_HEADER_BAR (widget);
  GdHeaderBarPrivate *priv = bar->priv;

  if (priv->event_window)
    gdk_window_hide (priv->event_window);

  GTK_WIDGET_CLASS (gd_header_bar_parent_class)->unmap (widget);
}

static gint
gd_header_bar_draw (GtkWidget *widget,
                    cairo_t   *cr)
{
  GtkStyleContext *context;

  context = gtk_widget_get_style_context (widget);

  gtk_render_background (context, cr, 0, 0,
                         gtk_widget_get_allocated_width (widget),
                         gtk_widget_get_allocated_height (widget));
  gtk_render_frame (context, cr, 0, 0,
                    gtk_widget_get_allocated_width (widget),
                    gtk_widget_get_allocated_height (widget));


  GTK_WIDGET_CLASS (gd_header_bar_parent_class)->draw (widget, cr);

  return TRUE;
}

static void
gd_header_bar_class_init (GdHeaderBarClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (class);

  object_class->get_property = gd_header_bar_get_property;
  object_class->set_property = gd_header_bar_set_property;
  object_class->finalize = gd_header_bar_finalize;

  widget_class->size_allocate = gd_header_bar_size_allocate;
  widget_class->get_preferred_width = gd_header_bar_get_preferred_width;
  widget_class->get_preferred_height = gd_header_bar_get_preferred_height;
  widget_class->get_preferred_height_for_width = gd_header_bar_get_preferred_height_for_width;
  widget_class->get_preferred_width_for_height = gd_header_bar_get_preferred_width_for_height;
  widget_class->button_press_event = gd_header_bar_button_press;
  widget_class->realize = gd_header_bar_realize;
  widget_class->unrealize = gd_header_bar_unrealize;
  widget_class->map = gd_header_bar_map;
  widget_class->unmap = gd_header_bar_unmap;
  widget_class->draw = gd_header_bar_draw;

  container_class->add = gd_header_bar_add;
  container_class->remove = gd_header_bar_remove;
  container_class->forall = gd_header_bar_forall;
  container_class->child_type = gd_header_bar_child_type;
  container_class->set_child_property = gd_header_bar_set_child_property;
  container_class->get_child_property = gd_header_bar_get_child_property;
  gtk_container_class_handle_border_width (container_class);

  gtk_container_class_install_child_property (container_class,
                                              CHILD_PROP_PACK_TYPE,
                                              g_param_spec_enum ("pack-type",
                                                                 "Pack type",
                                                                 "A GtkPackType indicating whether the child is packed with reference to the start or end of the parent",
                                                                 GTK_TYPE_PACK_TYPE, GTK_PACK_START,
                                                                 G_PARAM_READWRITE));
  gtk_container_class_install_child_property (container_class,
                                              CHILD_PROP_POSITION,
                                              g_param_spec_int ("position",
                                                                "Position",
                                                                "The index of the child in the parent",
                                                                -1, G_MAXINT, 0,
                                                                G_PARAM_READABLE));

  g_object_class_install_property (object_class,
                                   PROP_TITLE,
                                   g_param_spec_string ("title",
                                                        "Title",
                                                        "The title to display",
                                                        NULL,
                                                        G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_SUBTITLE,
                                   g_param_spec_string ("subtitle",
                                                        "Subtitle",
                                                        "The subtitle to display",
                                                        NULL,
                                                        G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_CUSTOM_TITLE,
                                   g_param_spec_object ("custom-title",
                                                        "Custom Title",
                                                        "Custom title widget to display",
                                                        GTK_TYPE_WIDGET,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
                                   PROP_SPACING,
                                   g_param_spec_int ("spacing",
                                                     "Spacing",
                                                     "The amount of space between children",
                                                     0, G_MAXINT,
                                                     DEFAULT_SPACING,
                                                     G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_HPADDING,
                                   g_param_spec_int ("hpadding",
                                                     "Horizontal padding",
                                                     "The amount of space to the left and right of children",
                                                     0, G_MAXINT,
                                                     DEFAULT_HPADDING,
                                                     G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_VPADDING,
                                   g_param_spec_int ("vpadding",
                                                     "Vertical padding",
                                                     "The amount of space to the above and below children",
                                                     0, G_MAXINT,
                                                     DEFAULT_VPADDING,
                                                     G_PARAM_READWRITE));

  g_type_class_add_private (object_class, sizeof (GdHeaderBarPrivate));
}

static void
gd_header_buildable_add_child (GtkBuildable *buildable,
                               GtkBuilder   *builder,
                               GObject      *child,
                               const gchar  *type)
{
  if (type && strcmp (type, "title") == 0)
    gd_header_bar_set_custom_title (GD_HEADER_BAR (buildable), GTK_WIDGET (child));
  else if (!type)
    gtk_container_add (GTK_CONTAINER (buildable), GTK_WIDGET (child));
  else
    GTK_BUILDER_WARN_INVALID_CHILD_TYPE (GD_HEADER_BAR (buildable), type);
}

static void
gd_header_buildable_init (GtkBuildableIface *iface)
{
  iface->add_child = gd_header_buildable_add_child;
}

void
gd_header_bar_pack_start (GdHeaderBar *bar,
                          GtkWidget   *child)
{
  gd_header_bar_pack (bar, child, GTK_PACK_START);
}

void
gd_header_bar_pack_end (GdHeaderBar *bar,
                        GtkWidget   *child)
{
  gd_header_bar_pack (bar, child, GTK_PACK_END);
}

GtkWidget *
gd_header_bar_new (void)
{
  return GTK_WIDGET (g_object_new (GD_TYPE_HEADER_BAR, NULL));
}
