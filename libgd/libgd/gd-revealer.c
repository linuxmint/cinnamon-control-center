/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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

#include <gtk/gtk.h>
#include "gd-revealer.h"
#include <math.h>

enum  {
  PROP_0,
  PROP_ORIENTATION,
  PROP_TRANSITION_DURATION,
  PROP_REVEAL_CHILD,
  PROP_CHILD_REVEALED
};

#define FRAME_TIME_MSEC 17 /* 17 msec => 60 fps */

struct _GdRevealerPrivate {
  GtkOrientation orientation;
  gint transition_duration;

  GdkWindow* bin_window;
  GdkWindow* view_window;

  gdouble current_pos;
  gdouble source_pos;
  gdouble target_pos;

  guint tick_id;
  gint64 start_time;
  gint64 end_time;
};

#define GTK_PARAM_READWRITE G_PARAM_READWRITE|G_PARAM_STATIC_NAME|G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB
#define GD_REVEALER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GD_TYPE_REVEALER, GdRevealerPrivate))

static void     gd_revealer_real_realize                        (GtkWidget     *widget);
static void     gd_revealer_real_unrealize                      (GtkWidget     *widget);
static void     gd_revealer_real_add                            (GtkContainer  *widget,
                                                                 GtkWidget     *child);
static void     gd_revealer_real_style_updated                  (GtkWidget     *widget);
static void     gd_revealer_real_size_allocate                  (GtkWidget     *widget,
                                                                 GtkAllocation *allocation);
static void     gd_revealer_real_map                            (GtkWidget     *widget);
static void     gd_revealer_real_unmap                          (GtkWidget     *widget);
static gboolean gd_revealer_real_draw                           (GtkWidget     *widget,
                                                                 cairo_t       *cr);
static void     gd_revealer_real_get_preferred_height           (GtkWidget     *widget,
                                                                 gint          *minimum_height,
                                                                 gint          *natural_height);
static void     gd_revealer_real_get_preferred_height_for_width (GtkWidget     *widget,
                                                                 gint           width,
                                                                 gint          *minimum_height,
                                                                 gint          *natural_height);
static void     gd_revealer_real_get_preferred_width            (GtkWidget     *widget,
                                                                 gint          *minimum_width,
                                                                 gint          *natural_width);
static void     gd_revealer_real_get_preferred_width_for_height (GtkWidget     *widget,
                                                                 gint           height,
                                                                 gint          *minimum_width,
                                                                 gint          *natural_width);

G_DEFINE_TYPE(GdRevealer, gd_revealer, GTK_TYPE_BIN);

static void
gd_revealer_init (GdRevealer *revealer)
{
  GdRevealerPrivate *priv;

  priv = GD_REVEALER_GET_PRIVATE (revealer);
  revealer->priv = priv;

  priv->orientation = GTK_ORIENTATION_HORIZONTAL;
  priv->transition_duration = 250;
  priv->current_pos = 0.0;
  priv->target_pos = 0.0;

  gtk_widget_set_has_window ((GtkWidget*) revealer, TRUE);
  gtk_widget_set_redraw_on_allocate ((GtkWidget*) revealer, FALSE);
}

static void
gd_revealer_finalize (GObject* obj)
{
  GdRevealer *revealer = GD_REVEALER (obj);
  GdRevealerPrivate *priv = revealer->priv;

  if (priv->tick_id != 0)
    gtk_widget_remove_tick_callback (GTK_WIDGET (revealer), priv->tick_id);
  priv->tick_id = 0;

  G_OBJECT_CLASS (gd_revealer_parent_class)->finalize (obj);
}

static void
gd_revealer_get_property (GObject *object,
                          guint property_id,
                          GValue *value,
                          GParamSpec *pspec)
{
  GdRevealer *revealer = GD_REVEALER (object);

  switch (property_id) {
  case PROP_ORIENTATION:
    g_value_set_enum (value, gd_revealer_get_orientation (revealer));
    break;
  case PROP_TRANSITION_DURATION:
    g_value_set_int (value, gd_revealer_get_transition_duration (revealer));
    break;
  case PROP_REVEAL_CHILD:
    g_value_set_boolean (value, gd_revealer_get_reveal_child (revealer));
    break;
  case PROP_CHILD_REVEALED:
    g_value_set_boolean (value, gd_revealer_get_child_revealed (revealer));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
gd_revealer_set_property (GObject *object,
                          guint property_id,
                          const GValue *value,
                          GParamSpec *pspec)
{
  GdRevealer *revealer = GD_REVEALER (object);

  switch (property_id) {
  case PROP_ORIENTATION:
    gd_revealer_set_orientation (revealer, g_value_get_enum (value));
    break;
  case PROP_TRANSITION_DURATION:
    gd_revealer_set_transition_duration (revealer, g_value_get_int (value));
    break;
  case PROP_REVEAL_CHILD:
    gd_revealer_set_reveal_child (revealer, g_value_get_boolean (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
gd_revealer_class_init (GdRevealerClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

  object_class->get_property = gd_revealer_get_property;
  object_class->set_property = gd_revealer_set_property;
  object_class->finalize = gd_revealer_finalize;

  widget_class->realize = gd_revealer_real_realize;
  widget_class->unrealize = gd_revealer_real_unrealize;
  widget_class->style_updated = gd_revealer_real_style_updated;
  widget_class->size_allocate = gd_revealer_real_size_allocate;
  widget_class->map = gd_revealer_real_map;
  widget_class->unmap = gd_revealer_real_unmap;
  widget_class->draw = gd_revealer_real_draw;
  widget_class->get_preferred_height = gd_revealer_real_get_preferred_height;
  widget_class->get_preferred_height_for_width = gd_revealer_real_get_preferred_height_for_width;
  widget_class->get_preferred_width = gd_revealer_real_get_preferred_width;
  widget_class->get_preferred_width_for_height = gd_revealer_real_get_preferred_width_for_height;

  container_class->add = gd_revealer_real_add;

  g_object_class_install_property (object_class,
                                   PROP_ORIENTATION,
                                   g_param_spec_enum ("orientation", "orientation",
                                                      "The orientation of the widget",
                                                      GTK_TYPE_ORIENTATION,
                                                      GTK_ORIENTATION_HORIZONTAL,
                                                      GTK_PARAM_READWRITE | G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class,
                                   PROP_TRANSITION_DURATION,
                                   g_param_spec_int ("transition-duration", "Transition duration",
                                                     "The animation duration, in milliseconds",
                                                     G_MININT, G_MAXINT,
                                                     250,
                                                     GTK_PARAM_READWRITE | G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class,
                                   PROP_REVEAL_CHILD,
                                   g_param_spec_boolean ("reveal-child", "Reveal Child",
                                                         "Whether the container should reveal the child",
                                                         FALSE,
                                                         GTK_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class,
                                   PROP_CHILD_REVEALED,
                                   g_param_spec_boolean ("child-revealed", "Child Revealed",
                                                         "Whether the child is revealed and the animation target reached",
                                                         FALSE,
                                                         G_PARAM_READABLE));

  g_type_class_add_private (klass, sizeof (GdRevealerPrivate));
}


GtkWidget *
gd_revealer_new (void)
{
  return g_object_new (GD_TYPE_REVEALER, NULL);
}

static void
gd_revealer_get_child_allocation (GdRevealer *revealer,
                                  GtkAllocation* allocation,
                                  GtkAllocation* child_allocation)
{
  GtkWidget *child;
  GdRevealerPrivate *priv;

  g_return_if_fail (revealer != NULL);
  g_return_if_fail (allocation != NULL);

  priv = revealer->priv;

  child_allocation->x = 0;
  child_allocation->y = 0;
  child_allocation->width = allocation->width;
  child_allocation->height = allocation->height;

  child = gtk_bin_get_child (GTK_BIN (revealer));
  if (child != NULL && gtk_widget_get_visible (child))
    {
      if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
        gtk_widget_get_preferred_height_for_width (child, child_allocation->width, NULL,
                                                   &child_allocation->height);
      else
        gtk_widget_get_preferred_width_for_height (child, child_allocation->height, NULL,
                                                   &child_allocation->width);
    }
}

static void
gd_revealer_real_realize (GtkWidget *widget)
{
  GdRevealer *revealer = GD_REVEALER (widget);
  GdRevealerPrivate *priv = revealer->priv;
  GtkAllocation allocation;
  GdkWindowAttr attributes = { 0 };
  GdkWindowAttributesType attributes_mask;
  GtkAllocation child_allocation;
  GtkWidget *child;
  GtkStyleContext *context;

  gtk_widget_set_realized (widget, TRUE);

  gtk_widget_get_allocation (widget, &allocation);

  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.event_mask =
    gtk_widget_get_events (widget) | GDK_EXPOSURE_MASK;
  attributes_mask = (GDK_WA_X | GDK_WA_Y) | GDK_WA_VISUAL;

  priv->view_window =
    gdk_window_new (gtk_widget_get_parent_window ((GtkWidget*) revealer),
                    &attributes, attributes_mask);
  gtk_widget_set_window (widget, priv->view_window);
  gtk_widget_register_window (widget, priv->view_window);

  gd_revealer_get_child_allocation (revealer, &allocation, &child_allocation);

  attributes.x = 0;
  attributes.y = 0;
  attributes.width = child_allocation.width;
  attributes.height = child_allocation.height;

  if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
    attributes.y = allocation.height - child_allocation.height;
  else
    attributes.x = allocation.width - child_allocation.width;

  priv->bin_window =
    gdk_window_new (priv->view_window, &attributes, attributes_mask);
  gtk_widget_register_window (widget, priv->bin_window);

  child = gtk_bin_get_child (GTK_BIN (revealer));
  if (child != NULL)
    gtk_widget_set_parent_window (child, priv->bin_window);

  context = gtk_widget_get_style_context (widget);
  gtk_style_context_set_background (context, priv->view_window);
  gtk_style_context_set_background (context, priv->bin_window);
  gdk_window_show (priv->bin_window);
}


static void
gd_revealer_real_unrealize (GtkWidget* widget)
{
  GdRevealer *revealer = GD_REVEALER (widget);
  GdRevealerPrivate *priv = revealer->priv;

  gtk_widget_unregister_window (widget, priv->bin_window);
  gdk_window_destroy (priv->bin_window);
  priv->view_window = NULL;

  GTK_WIDGET_CLASS (gd_revealer_parent_class)->unrealize (widget);
}


static void
gd_revealer_real_add (GtkContainer* container,
                      GtkWidget* child)
{
  GdRevealer *revealer = GD_REVEALER (container);
  GdRevealerPrivate *priv = revealer->priv;

  g_return_if_fail (child != NULL);

  gtk_widget_set_parent_window (child, priv->bin_window);
  gtk_widget_set_child_visible (child, priv->current_pos != 0.0);

  GTK_CONTAINER_CLASS (gd_revealer_parent_class)->add (container, child);
}


static void
gd_revealer_real_style_updated (GtkWidget* widget)
{
  GdRevealer *revealer = GD_REVEALER (widget);
  GdRevealerPrivate *priv = revealer->priv;
  GtkStyleContext* context;

  GTK_WIDGET_CLASS (gd_revealer_parent_class)->style_updated (widget);

  if (gtk_widget_get_realized (widget))
    {
      context = gtk_widget_get_style_context (widget);
      gtk_style_context_set_background (context, priv->bin_window);
      gtk_style_context_set_background (context, priv->view_window);
    }
}


static void
gd_revealer_real_size_allocate (GtkWidget* widget,
                                GtkAllocation* allocation)
{
  GdRevealer *revealer = GD_REVEALER (widget);
  GdRevealerPrivate *priv = revealer->priv;
  GtkAllocation child_allocation;
  GtkWidget *child;
  gboolean window_visible;
  int bin_x, bin_y;

  g_return_if_fail (allocation != NULL);

  gtk_widget_set_allocation (widget, allocation);
  gd_revealer_get_child_allocation (revealer, allocation, &child_allocation);

  child = gtk_bin_get_child (GTK_BIN (revealer));
  if (child != NULL &&
      gtk_widget_get_visible (child))
    gtk_widget_size_allocate (child, &child_allocation);

  if (gtk_widget_get_realized (widget))
    {
      if (gtk_widget_get_mapped (widget))
        {
          window_visible =
            allocation->width > 0 && allocation->height > 0;

          if (!window_visible &&
              gdk_window_is_visible (priv->view_window))
            gdk_window_hide (priv->view_window);

          if (window_visible &&
              !gdk_window_is_visible (priv->view_window))
            gdk_window_show (priv->view_window);
        }

      gdk_window_move_resize (priv->view_window,
                              allocation->x, allocation->y,
                              allocation->width, allocation->height);

      bin_x = 0;
      bin_y = 0;
      if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
        bin_y = allocation->height - child_allocation.height;
      else
        bin_x = allocation->width - child_allocation.width;

      gdk_window_move_resize (priv->bin_window,
                              bin_x, bin_y,
                              child_allocation.width, child_allocation.height);
    }
}


static void
gd_revealer_set_position (GdRevealer *revealer,
                          gdouble pos)
{
  GdRevealerPrivate *priv = revealer->priv;
  gboolean new_visible;
  GtkWidget *child;

  priv->current_pos = pos;

  /* We check target_pos here too, because we want to ensure we set
   * child_visible immediately when starting a reveal operation
   * otherwise the child widgets will not be properly realized
   * after the reveal returns.
   */
  new_visible = priv->current_pos != 0.0 || priv->target_pos != 0.0;

  child = gtk_bin_get_child (GTK_BIN (revealer));
  if (child != NULL &&
      new_visible != gtk_widget_get_child_visible (child))
    gtk_widget_set_child_visible (child, new_visible);

  gtk_widget_queue_resize (GTK_WIDGET (revealer));

  if (priv->current_pos == priv->target_pos)
    g_object_notify (G_OBJECT (revealer), "child-revealed");
}

static gdouble
ease_out_quad (gdouble t, gdouble d)
{
  gdouble p = t / d;
  return  ((-1.0) * p) * (p - 2);
}

static void
gd_revealer_animate_step (GdRevealer *revealer,
                          gint64 now)
{
  GdRevealerPrivate *priv = revealer->priv;
  gdouble t;

  t = 1.0;
  if (now < priv->end_time)
      t = (now - priv->start_time) / (double) (priv->end_time - priv->start_time);
  t = ease_out_quad (t, 1.0);

  gd_revealer_set_position (revealer,
                            priv->source_pos + (t * (priv->target_pos - priv->source_pos)));
}

static gboolean
gd_revealer_animate_cb (GdRevealer *revealer,
                        GdkFrameClock *frame_clock,
                        gpointer user_data)
{
  GdRevealerPrivate *priv = revealer->priv;
  gint64 now;

  now = gdk_frame_clock_get_frame_time (frame_clock);
  gd_revealer_animate_step (revealer, now);
  if (priv->current_pos == priv->target_pos)
    {
      priv->tick_id = 0;
      return FALSE;
    }

  return TRUE;
}

static void
gd_revealer_start_animation (GdRevealer *revealer,
                             gdouble target)
{
  GdRevealerPrivate *priv = revealer->priv;
  GtkWidget *widget = GTK_WIDGET (revealer);

  if (priv->target_pos == target)
    return;

  priv->target_pos = target;
  g_object_notify (G_OBJECT (revealer), "reveal-child");

  if (gtk_widget_get_mapped (widget))
    {
      priv->source_pos = priv->current_pos;
      priv->start_time = gdk_frame_clock_get_frame_time (gtk_widget_get_frame_clock (widget));
      priv->end_time = priv->start_time + (priv->transition_duration * 1000);
      if (priv->tick_id == 0)
        priv->tick_id =
          gtk_widget_add_tick_callback (widget, (GtkTickCallback)gd_revealer_animate_cb, revealer, NULL);
      gd_revealer_animate_step (revealer, priv->start_time);
    }
  else
    {
      gd_revealer_set_position (revealer, target);
    }
}


static void
gd_revealer_stop_animation (GdRevealer *revealer)
{
  GdRevealerPrivate *priv = revealer->priv;

  priv->current_pos = priv->target_pos;
  if (priv->tick_id != 0)
    {
      gtk_widget_remove_tick_callback (GTK_WIDGET (revealer), priv->tick_id);
      priv->tick_id = 0;
    }
}


static void
gd_revealer_real_map (GtkWidget *widget)
{
  GdRevealer *revealer = GD_REVEALER (widget);
  GdRevealerPrivate *priv = revealer->priv;
  GtkAllocation allocation;

  if (!gtk_widget_get_mapped (widget))
    {
      gtk_widget_get_allocation (widget, &allocation);

      if (allocation.width > 0 && allocation.height > 0)
        gdk_window_show (priv->view_window);

      gd_revealer_start_animation (revealer, priv->target_pos);
    }

  GTK_WIDGET_CLASS (gd_revealer_parent_class)->map (widget);
}

static void
gd_revealer_real_unmap (GtkWidget *widget)
{
  GdRevealer *revealer = GD_REVEALER (widget);

  GTK_WIDGET_CLASS (gd_revealer_parent_class)->unmap (widget);

  gd_revealer_stop_animation (revealer);
}


static gboolean
gd_revealer_real_draw (GtkWidget *widget,
                       cairo_t *cr)
{
  GdRevealer *revealer = GD_REVEALER (widget);
  GdRevealerPrivate *priv = revealer->priv;

  if (gtk_cairo_should_draw_window (cr, priv->bin_window))
    GTK_WIDGET_CLASS (gd_revealer_parent_class)->draw (widget, cr);

  return TRUE;
}

void
gd_revealer_set_reveal_child (GdRevealer *revealer,
                              gboolean    setting)
{
  g_return_if_fail (GD_IS_REVEALER (revealer));

  if (setting)
    gd_revealer_start_animation (revealer, 1.0);
  else
    gd_revealer_start_animation (revealer, 0.0);
}

gboolean
gd_revealer_get_reveal_child (GdRevealer *revealer)
{
  g_return_val_if_fail (GD_IS_REVEALER (revealer), FALSE);

  return revealer->priv->target_pos != 0.0;
}

gboolean
gd_revealer_get_child_revealed (GdRevealer *revealer)
{
  gboolean animation_finished = (revealer->priv->target_pos == revealer->priv->current_pos);
  gboolean reveal_child = gd_revealer_get_reveal_child (revealer);

  if (animation_finished)
    return reveal_child;
  else
    return !reveal_child;
}

/* These all report only the natural size, ignoring the minimal size,
 * because its not really possible to allocate the right size during
 * animation if the child size can change (without the child
 * re-arranging itself during the animation).
 */

static void
gd_revealer_real_get_preferred_height (GtkWidget* widget,
                                       gint* minimum_height_out,
                                       gint* natural_height_out)
{
  GdRevealer *revealer = GD_REVEALER (widget);
  GdRevealerPrivate *priv = revealer->priv;
  gint minimum_height;
  gint natural_height;

  GTK_WIDGET_CLASS (gd_revealer_parent_class)->get_preferred_height (widget, &minimum_height, &natural_height);

  if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
    natural_height = round (natural_height * priv->current_pos);

  minimum_height = natural_height;

  if (minimum_height_out)
    *minimum_height_out = minimum_height;
  if (natural_height_out)
    *natural_height_out = natural_height;
}

static void
gd_revealer_real_get_preferred_height_for_width (GtkWidget* widget,
                                                 gint width,
                                                 gint* minimum_height_out,
                                                 gint* natural_height_out)
{
  GdRevealer *revealer = GD_REVEALER (widget);
  GdRevealerPrivate *priv = revealer->priv;
  gint minimum_height;
  gint natural_height;

  GTK_WIDGET_CLASS (gd_revealer_parent_class)->get_preferred_height_for_width (widget, width, &minimum_height, &natural_height);
  if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
      natural_height = round (natural_height * priv->current_pos);

  minimum_height = natural_height;

  if (minimum_height_out)
    *minimum_height_out = minimum_height;
  if (natural_height_out)
    *natural_height_out = natural_height;
}

static void
gd_revealer_real_get_preferred_width (GtkWidget* widget,
                                      gint* minimum_width_out,
                                      gint* natural_width_out)
{
  GdRevealer *revealer = GD_REVEALER (widget);
  GdRevealerPrivate *priv = revealer->priv;
  gint minimum_width;
  gint natural_width;

  GTK_WIDGET_CLASS (gd_revealer_parent_class)->get_preferred_width (widget, &minimum_width, &natural_width);

  if (priv->orientation == GTK_ORIENTATION_VERTICAL)
    natural_width = round (natural_width * priv->current_pos);

  minimum_width = natural_width;

  if (minimum_width_out)
    *minimum_width_out = minimum_width;
  if (natural_width_out)
    *natural_width_out = natural_width;
}

static void
gd_revealer_real_get_preferred_width_for_height (GtkWidget* widget,
                                                 gint height,
                                                 gint* minimum_width_out,
                                                 gint* natural_width_out)
{
  GdRevealer *revealer = GD_REVEALER (widget);
  GdRevealerPrivate *priv = revealer->priv;
  gint minimum_width;
  gint natural_width;

  GTK_WIDGET_CLASS (gd_revealer_parent_class)->get_preferred_width_for_height (widget, height, &minimum_width, &natural_width);

  if (priv->orientation == GTK_ORIENTATION_VERTICAL)
    natural_width = round (natural_width * priv->current_pos);

  minimum_width = natural_width;

  if (minimum_width_out)
    *minimum_width_out = minimum_width;
  if (natural_width_out)
    *natural_width_out = natural_width;
}

GtkOrientation
gd_revealer_get_orientation (GdRevealer *revealer)
{
  g_return_val_if_fail (revealer != NULL, 0);

  return revealer->priv->orientation;
}

void
gd_revealer_set_orientation (GdRevealer *revealer,
                             GtkOrientation value)
{
  g_return_if_fail (GD_IS_REVEALER (revealer));

  revealer->priv->orientation = value;
  g_object_notify (G_OBJECT (revealer), "orientation");
}

gint
gd_revealer_get_transition_duration (GdRevealer *revealer)
{
  g_return_val_if_fail (revealer != NULL, 0);

  return revealer->priv->transition_duration;
}

void
gd_revealer_set_transition_duration (GdRevealer *revealer,
                                     gint value)
{
  g_return_if_fail (GD_IS_REVEALER (revealer));

  revealer->priv->transition_duration = value;
  g_object_notify (G_OBJECT (revealer), "transition-duration");
}
