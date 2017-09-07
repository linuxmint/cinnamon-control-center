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

#include "gd-stack-switcher.h"
#include "gd-header-button.h"

struct _GdStackSwitcherPrivate
{
  GdStack *stack;
  GHashTable *buttons;
  gboolean in_child_changed;
};

enum {
  PROP_0,
  PROP_STACK
};

G_DEFINE_TYPE (GdStackSwitcher, gd_stack_switcher, GTK_TYPE_BOX);

static void
gd_stack_switcher_init (GdStackSwitcher *switcher)
{
  GtkStyleContext *context;
  GdStackSwitcherPrivate *priv;

  priv = G_TYPE_INSTANCE_GET_PRIVATE (switcher, GD_TYPE_STACK_SWITCHER, GdStackSwitcherPrivate);
  switcher->priv = priv;

  priv->stack = NULL;
  priv->buttons = g_hash_table_new (g_direct_hash, g_direct_equal);

  context = gtk_widget_get_style_context (GTK_WIDGET (switcher));
  gtk_style_context_add_class (context, GTK_STYLE_CLASS_LINKED);

  gtk_orientable_set_orientation (GTK_ORIENTABLE (switcher), GTK_ORIENTATION_HORIZONTAL);
}

static void
clear_switcher (GdStackSwitcher *self)
{
  gtk_container_foreach (GTK_CONTAINER (self), (GtkCallback) gtk_widget_destroy, self);
}

static void
on_button_clicked (GtkWidget       *widget,
                   GdStackSwitcher *self)
{
  GtkWidget *child;

  if (!self->priv->in_child_changed)
    {
      child = g_object_get_data (G_OBJECT (widget), "stack-child");
      gd_stack_set_visible_child (self->priv->stack, child);
    }
}

static void
update_button (GdStackSwitcher *self,
               GtkWidget       *widget,
               GtkWidget       *button)
{
  char *title;
  char *symbolic_icon_name;

  gtk_container_child_get (GTK_CONTAINER (self->priv->stack), widget,
                           "title", &title,
                           "symbolic-icon-name", &symbolic_icon_name,
                           NULL);

  gd_header_button_set_symbolic_icon_name (GD_HEADER_BUTTON (button), symbolic_icon_name);
  gd_header_button_set_label (GD_HEADER_BUTTON (button), title);

  gtk_widget_set_visible (button, title != NULL || symbolic_icon_name != NULL);

  if (symbolic_icon_name != NULL)
    gtk_widget_set_size_request (button, -1, -1);
  else
    gtk_widget_set_size_request (button, 100, -1);

  g_free (title);
  g_free (symbolic_icon_name);
}

static void
on_title_icon_updated (GtkWidget       *widget,
                       GParamSpec      *pspec,
                       GdStackSwitcher *self)

{
  GtkWidget *button;

  button = g_hash_table_lookup (self->priv->buttons, widget);
  update_button (self, widget, button);
}

static void
on_position_updated (GtkWidget       *widget,
                     GParamSpec      *pspec,
                     GdStackSwitcher *self)
{
  GtkWidget *button;
  gint position;

  button = g_hash_table_lookup (self->priv->buttons, widget);

  gtk_container_child_get (GTK_CONTAINER (self->priv->stack), widget,
                          "position", &position,
                          NULL);

  gtk_box_reorder_child (GTK_BOX (self), button, position);
}

static void
add_child (GdStackSwitcher *self,
           GtkWidget       *widget)
{
  GtkWidget *button;
  GList *group;
  GtkStyleContext *context;

  button = gd_header_radio_button_new ();
  update_button (self, widget, button);

  group = gtk_container_get_children (GTK_CONTAINER (self));
  if (group != NULL)
    {
      gtk_radio_button_join_group (GTK_RADIO_BUTTON (button), GTK_RADIO_BUTTON (group->data));
      g_list_free (group);
    }

  gtk_container_add (GTK_CONTAINER (self), button);

  g_object_set_data (G_OBJECT (button), "stack-child", widget);
  g_signal_connect (button, "clicked", G_CALLBACK (on_button_clicked), self);
  g_signal_connect (widget, "child-notify::title", G_CALLBACK (on_title_icon_updated), self);
  g_signal_connect (widget, "child-notify::symbolic-icon-name", G_CALLBACK (on_title_icon_updated), self);
  g_signal_connect (widget, "child-notify::position", G_CALLBACK (on_position_updated), self);

  g_hash_table_insert (self->priv->buttons, widget, button);
}

static void
foreach_stack (GtkWidget       *widget,
               GdStackSwitcher *self)
{
  add_child (self, widget);
}

static void
populate_switcher (GdStackSwitcher *self)
{
  gtk_container_foreach (GTK_CONTAINER (self->priv->stack), (GtkCallback)foreach_stack, self);
}

static void
on_child_changed (GtkWidget       *widget,
                  GParamSpec      *pspec,
                  GdStackSwitcher *self)
{
  GtkWidget *child;
  GtkWidget *button;

  child = gd_stack_get_visible_child (GD_STACK (widget));
  button = g_hash_table_lookup (self->priv->buttons, child);
  if (button != NULL)
    {
      self->priv->in_child_changed = TRUE;
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
      self->priv->in_child_changed = FALSE;
    }
}

static void
on_stack_child_added (GtkContainer    *container,
                      GtkWidget       *widget,
                      GdStackSwitcher *self)
{
  add_child (self, widget);
}

static void
on_stack_child_removed (GtkContainer    *container,
                        GtkWidget       *widget,
                        GdStackSwitcher *self)
{
  GtkWidget *button;

  button = g_hash_table_lookup (self->priv->buttons, widget);
  gtk_container_remove (GTK_CONTAINER (self), button);
  g_hash_table_remove (self->priv->buttons, widget);
}

static void
disconnect_stack_signals (GdStackSwitcher *switcher)
{
  GdStackSwitcherPrivate *priv = switcher->priv;

  g_signal_handlers_disconnect_by_func (priv->stack, on_stack_child_added, switcher);

  g_signal_handlers_disconnect_by_func (priv->stack, on_stack_child_removed, switcher);

  g_signal_handlers_disconnect_by_func (priv->stack, on_child_changed, switcher);

  g_signal_handlers_disconnect_by_func (priv->stack, disconnect_stack_signals, switcher);
}

static void
connect_stack_signals (GdStackSwitcher *switcher)
{
  GdStackSwitcherPrivate *priv = switcher->priv;

  g_signal_connect_after (priv->stack, "add",
                          G_CALLBACK (on_stack_child_added), switcher);
  g_signal_connect_after (priv->stack, "remove",
                          G_CALLBACK (on_stack_child_removed), switcher);
  g_signal_connect (priv->stack, "notify::visible-child",
                    G_CALLBACK (on_child_changed), switcher);

  g_signal_connect_swapped (priv->stack, "destroy",
                            G_CALLBACK (disconnect_stack_signals), switcher);
}

/**
 * gd_stack_switcher_set_stack:
 * @switcher: a #GdStackSwitcher
 * @stack: (allow-none): a #GdStack
 *
 * Sets the stack to control.
 *
 **/
void
gd_stack_switcher_set_stack (GdStackSwitcher *switcher,
                             GdStack         *stack)
{
  GdStackSwitcherPrivate *priv;

  g_return_if_fail (GD_IS_STACK_SWITCHER (switcher));
  if (stack)
    g_return_if_fail (GD_IS_STACK (stack));

  priv = switcher->priv;

  if (priv->stack == stack)
    return;

  if (priv->stack)
    {
      disconnect_stack_signals (switcher);
      clear_switcher (switcher);
      g_clear_object (&priv->stack);
    }

  if (stack)
    {
      priv->stack = g_object_ref (stack);
      populate_switcher (switcher);
      connect_stack_signals (switcher);
    }

  gtk_widget_queue_resize (GTK_WIDGET (switcher));

  g_object_notify (G_OBJECT (switcher), "stack");
}

/**
 * gd_stack_switcher_get_stack:
 * @switcher: a #GdStackSwitcher
 *
 * Retrieves the stack. See
 * gd_stack_switcher_set_stack().
 *
 * Return value: (transfer none): the stack, or %NULL if
 *    none has been set explicitly.
 **/
GdStack *
gd_stack_switcher_get_stack (GdStackSwitcher *switcher)
{
  g_return_val_if_fail (GD_IS_STACK_SWITCHER (switcher), NULL);

  return switcher->priv->stack;
}

static void
gd_stack_switcher_get_property (GObject      *object,
                                guint         prop_id,
                                GValue       *value,
                                GParamSpec   *pspec)
{
  GdStackSwitcher *switcher = GD_STACK_SWITCHER (object);
  GdStackSwitcherPrivate *priv = switcher->priv;

  switch (prop_id)
    {
    case PROP_STACK:
      g_value_set_object (value, priv->stack);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gd_stack_switcher_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GdStackSwitcher *switcher = GD_STACK_SWITCHER (object);

  switch (prop_id)
    {
    case PROP_STACK:
      gd_stack_switcher_set_stack (switcher, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gd_stack_switcher_dispose (GObject *object)
{
  GdStackSwitcher *switcher = GD_STACK_SWITCHER (object);

  gd_stack_switcher_set_stack (switcher, NULL);

  G_OBJECT_CLASS (gd_stack_switcher_parent_class)->dispose (object);
}

static void
gd_stack_switcher_class_init (GdStackSwitcherClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->get_property = gd_stack_switcher_get_property;
  object_class->set_property = gd_stack_switcher_set_property;
  object_class->dispose = gd_stack_switcher_dispose;

  g_object_class_install_property (object_class,
                                   PROP_STACK,
                                   g_param_spec_object ("stack",
                                                        "Stack",
                                                        "Stack",
                                                        GD_TYPE_STACK,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (object_class, sizeof (GdStackSwitcherPrivate));
}

GtkWidget *
gd_stack_switcher_new (void)
{
  return GTK_WIDGET (g_object_new (GD_TYPE_STACK_SWITCHER, NULL));
}
