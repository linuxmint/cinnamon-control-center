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

#include "gd-header-button.h"

typedef GTypeInterface GdHeaderButtonIface;
typedef GdHeaderButtonIface GdHeaderButtonInterface;
#define GD_HEADER_BUTTON_GET_IFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), GD_TYPE_HEADER_BUTTON, GdHeaderButtonIface))

G_DEFINE_INTERFACE (GdHeaderButton, gd_header_button, GTK_TYPE_BUTTON)

enum {
  PROP_0,
  PROP_LABEL,
  PROP_USE_MARKUP,
  PROP_SYMBOLIC_ICON_NAME
};

static void
gd_header_button_default_init (GdHeaderButtonIface *iface)
{
  GParamSpec *pspec;

  /**
   * GdHeaderButton:label:
   *
   * The label of the #GdHeaderButton object.
   */
  pspec = g_param_spec_string ("label",
                               "Text label",
                               "Label displayed by the button",
                               NULL,
                               G_PARAM_READWRITE |
                               G_PARAM_STATIC_STRINGS);
  g_object_interface_install_property (iface, pspec);

  /**
   * GdHeaderButton:use-markup:
   *
   * Whether the label of the #GdHeaderButton object should use markup.
   */
  pspec = g_param_spec_boolean ("use-markup",
                                "Use markup",
                                "Whether the label should use markup",
                                FALSE,
                                G_PARAM_READWRITE |
                                G_PARAM_STATIC_STRINGS);
  g_object_interface_install_property (iface, pspec);

  /**
   * GdHeaderButton:symbolic-icon-name:
   *
   * The symbolic icon name of the #GdHeaderButton object.
   */
  pspec = g_param_spec_string ("symbolic-icon-name",
                               "Symbolic icon name",
                               "The name of the symbolic icon displayed by the button",
                               NULL,
                               G_PARAM_READWRITE |
                               G_PARAM_STATIC_STRINGS);
  g_object_interface_install_property (iface, pspec);
}

/**
 * gd_header_button_get_label:
 * @self:
 *
 * Returns: (transfer full):
 */
gchar *
gd_header_button_get_label (GdHeaderButton *self)
{
  gchar *label;
  g_object_get (self, "label", &label, NULL);

  return label;
}

/**
 * gd_header_button_set_label:
 * @self:
 * @label: (allow-none):
 *
 */
void
gd_header_button_set_label (GdHeaderButton *self,
                            const gchar    *label)
{
  g_object_set (self, "label", label, NULL);
}

/**
 * gd_header_button_get_symbolic_icon_name:
 * @self:
 *
 * Returns: (transfer full):
 */
gchar *
gd_header_button_get_symbolic_icon_name (GdHeaderButton *self)
{
  gchar *symbolic_icon_name;
  g_object_get (self, "symbolic-icon-name", &symbolic_icon_name, NULL);

  return symbolic_icon_name;
}

/**
 * gd_header_button_set_symbolic_icon_name:
 * @self:
 * @symbolic_icon_name: (allow-none):
 *
 */
void
gd_header_button_set_symbolic_icon_name (GdHeaderButton *self,
                                         const gchar    *symbolic_icon_name)
{
  if (symbolic_icon_name != NULL &&
      !g_str_has_suffix (symbolic_icon_name, "-symbolic"))
    {
      g_warning ("gd_header_button_set_symbolic_icon_name was called with "
                 "a non-symbolic name.");
      return;
    }

  g_object_set (self, "symbolic-icon-name", symbolic_icon_name, NULL);
}

/**
 * gd_header_button_get_use_markup:
 * @self:
 *
 * Returns:
 */
gboolean
gd_header_button_get_use_markup (GdHeaderButton *self)
{
  gboolean use_markup;

  g_object_get (self, "use-markup", &use_markup, NULL);
  return use_markup;
}

/**
 * gd_header_button_set_use_markup:
 * @self:
 * @use_markup:
 *
 */
void
gd_header_button_set_use_markup (GdHeaderButton *self,
                                 gboolean        use_markup)
{
  g_object_set (self, "use-markup", use_markup, NULL);
}

/* generic implementation for all private subclasses */
typedef struct _GdHeaderButtonPrivate GdHeaderButtonPrivate;
struct _GdHeaderButtonPrivate {
  gchar *label;
  gchar *symbolic_icon_name;

  gboolean use_markup;
};

#define GET_PRIVATE(inst) G_TYPE_INSTANCE_GET_PRIVATE (inst, G_OBJECT_TYPE (inst), GdHeaderButtonPrivate)
#define GET_PARENT_CLASS(inst) g_type_class_peek_parent (G_OBJECT_GET_CLASS (inst))

static void
rebuild_child (GdHeaderButton *self)
{
  GdHeaderButtonPrivate *priv = GET_PRIVATE (self);
  GtkStyleContext *context;
  GtkWidget *button_child, *label;

  gtk_widget_set_valign (GTK_WIDGET (self), GTK_ALIGN_CENTER);

  button_child = gtk_bin_get_child (GTK_BIN (self));
  if (button_child != NULL)
    gtk_widget_destroy (button_child);

  button_child = NULL;
  context = gtk_widget_get_style_context (GTK_WIDGET (self));

  if (priv->symbolic_icon_name != NULL)
    {
      button_child = gtk_image_new_from_icon_name (priv->symbolic_icon_name, GTK_ICON_SIZE_MENU);
      if (priv->label != NULL)
        gtk_widget_set_tooltip_text (GTK_WIDGET (self), priv->label);

      gtk_style_context_remove_class (context, "text-button");
      gtk_style_context_add_class (context, "image-button");
    }
  else if (priv->label != NULL)
    {
      label = gtk_label_new (priv->label);
      gtk_label_set_use_markup (GTK_LABEL (label), priv->use_markup);

      if (GTK_IS_MENU_BUTTON (self))
        {
          GtkWidget *arrow;

          button_child = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
          gtk_container_add (GTK_CONTAINER (button_child), label);

          arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_NONE);
          gtk_container_add (GTK_CONTAINER (button_child), arrow);
        }
      else
        {
          button_child = label;
        }

      gtk_style_context_remove_class (context, "image-button");
      gtk_style_context_add_class (context, "text-button");
    }

  if (button_child)
    {
      gtk_widget_show_all (button_child);
      gtk_container_add (GTK_CONTAINER (self), button_child);
    }
}

static void
button_set_label (GdHeaderButton *self,
                  const gchar    *label)
{
  GdHeaderButtonPrivate *priv = GET_PRIVATE (self);

  if (g_strcmp0 (priv->label, label) != 0)
    {
      g_free (priv->label);
      priv->label = g_strdup (label);

      rebuild_child (self);
      g_object_notify (G_OBJECT (self), "label");
    }
}

static void
button_set_use_markup (GdHeaderButton *self,
                       gboolean        use_markup)
{
  GdHeaderButtonPrivate *priv = GET_PRIVATE (self);

  if (priv->use_markup != use_markup)
    {
      priv->use_markup = use_markup;

      rebuild_child (self);
      g_object_notify (G_OBJECT (self), "use-markup");
    }
}

static void
button_set_symbolic_icon_name (GdHeaderButton *self,
                               const gchar    *symbolic_icon_name)
{
  GdHeaderButtonPrivate *priv = GET_PRIVATE (self);

  if (g_strcmp0 (priv->symbolic_icon_name, symbolic_icon_name) != 0)
    {
      g_free (priv->symbolic_icon_name);
      priv->symbolic_icon_name = g_strdup (symbolic_icon_name);

      rebuild_child (self);
      g_object_notify (G_OBJECT (self), "symbolic-icon-name");
    }
}

static void
gd_header_button_generic_finalize (GObject *object)
{
  GdHeaderButton *self = GD_HEADER_BUTTON (object);
  GdHeaderButtonPrivate *priv = GET_PRIVATE (self);

  g_free (priv->label);
  g_free (priv->symbolic_icon_name);

  G_OBJECT_CLASS (GET_PARENT_CLASS (object))->finalize (object);
}

static void
gd_header_button_generic_set_property (GObject      *object,
                                       guint         prop_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  GdHeaderButton *self = GD_HEADER_BUTTON (object);

  switch (prop_id)
    {
    case PROP_LABEL:
      button_set_label (self, g_value_get_string (value));
      break;
    case PROP_USE_MARKUP:
      button_set_use_markup (self, g_value_get_boolean (value));
      break;
    case PROP_SYMBOLIC_ICON_NAME:
      button_set_symbolic_icon_name (self, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gd_header_button_generic_get_property (GObject      *object,
                                       guint         prop_id,
                                       GValue       *value,
                                       GParamSpec   *pspec)
{
  GdHeaderButton *self = GD_HEADER_BUTTON (object);
  GdHeaderButtonPrivate *priv = GET_PRIVATE (self);

  switch (prop_id)
    {
    case PROP_LABEL:
      g_value_set_string (value, priv->label);
      break;
    case PROP_USE_MARKUP:
      g_value_set_boolean (value, priv->use_markup);
      break;
    case PROP_SYMBOLIC_ICON_NAME:
      g_value_set_string (value, priv->symbolic_icon_name);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gd_header_button_generic_iface_init (GdHeaderButtonIface *iface)
{
}

static void
gd_header_button_generic_class_init (gpointer klass)
{
  GObjectClass *oclass = G_OBJECT_CLASS (klass);

  oclass->get_property = gd_header_button_generic_get_property;
  oclass->set_property = gd_header_button_generic_set_property;
  oclass->finalize = gd_header_button_generic_finalize;

  g_object_class_override_property (oclass, PROP_LABEL, "label");
  g_object_class_override_property (oclass, PROP_USE_MARKUP, "use-markup");
  g_object_class_override_property (oclass, PROP_SYMBOLIC_ICON_NAME, "symbolic-icon-name");

  g_type_class_add_private (klass, sizeof (GdHeaderButtonPrivate));
}

/* private subclasses */
typedef GtkButtonClass GdHeaderSimpleButtonClass;
G_DEFINE_TYPE_WITH_CODE (GdHeaderSimpleButton, gd_header_simple_button, GTK_TYPE_BUTTON,
                         G_IMPLEMENT_INTERFACE (GD_TYPE_HEADER_BUTTON, gd_header_button_generic_iface_init))

static void
gd_header_simple_button_class_init (GdHeaderSimpleButtonClass *klass)
{
  gd_header_button_generic_class_init (klass);
}

static void
gd_header_simple_button_init (GdHeaderSimpleButton *self)
{
}

typedef GtkToggleButtonClass GdHeaderToggleButtonClass;
G_DEFINE_TYPE_WITH_CODE (GdHeaderToggleButton, gd_header_toggle_button, GTK_TYPE_TOGGLE_BUTTON,
                         G_IMPLEMENT_INTERFACE (GD_TYPE_HEADER_BUTTON, gd_header_button_generic_iface_init))

static void
gd_header_toggle_button_class_init (GdHeaderToggleButtonClass *klass)
{
  gd_header_button_generic_class_init (klass);
}

static void
gd_header_toggle_button_init (GdHeaderToggleButton *self)
{
}

typedef GtkMenuButtonClass GdHeaderMenuButtonClass;
G_DEFINE_TYPE_WITH_CODE (GdHeaderMenuButton, gd_header_menu_button, GTK_TYPE_MENU_BUTTON,
                         G_IMPLEMENT_INTERFACE (GD_TYPE_HEADER_BUTTON, gd_header_button_generic_iface_init))

static void
gd_header_menu_button_class_init (GdHeaderMenuButtonClass *klass)
{
  gd_header_button_generic_class_init (klass);
}

static void
gd_header_menu_button_init (GdHeaderMenuButton *self)
{
}

typedef GtkRadioButtonClass GdHeaderRadioButtonClass;
G_DEFINE_TYPE_WITH_CODE (GdHeaderRadioButton, gd_header_radio_button, GTK_TYPE_RADIO_BUTTON,
                         G_IMPLEMENT_INTERFACE (GD_TYPE_HEADER_BUTTON, gd_header_button_generic_iface_init))

static void
gd_header_radio_button_constructed (GObject *object)
{
  GdHeaderRadioButton *self = (GdHeaderRadioButton *) (object);

  G_OBJECT_CLASS (GET_PARENT_CLASS (object))->constructed (object);

  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (self), FALSE);
}

static void
gd_header_radio_button_class_init (GdHeaderRadioButtonClass *klass)
{
  GObjectClass *oclass = G_OBJECT_CLASS (klass);

  oclass->constructed = gd_header_radio_button_constructed;

  gd_header_button_generic_class_init (klass);
}

static void
gd_header_radio_button_init (GdHeaderRadioButton *self)
{
}

/**
 * gd_header_simple_button_new:
 *
 * Returns: (transfer none):
 */
GtkWidget *
gd_header_simple_button_new (void)
{
  return g_object_new (GD_TYPE_HEADER_SIMPLE_BUTTON, NULL);
}

/**
 * gd_header_toggle_button_new:
 *
 * Returns: (transfer none):
 */
GtkWidget *
gd_header_toggle_button_new (void)
{
  return g_object_new (GD_TYPE_HEADER_TOGGLE_BUTTON, NULL);
}

/**
 * gd_header_radio_button_new:
 *
 * Returns: (transfer none):
 */
GtkWidget *
gd_header_radio_button_new (void)
{
  return g_object_new (GD_TYPE_HEADER_RADIO_BUTTON, NULL);
}

/**
 * gd_header_menu_button_new:
 *
 * Returns: (transfer none):
 */
GtkWidget *
gd_header_menu_button_new (void)
{
  return g_object_new (GD_TYPE_HEADER_MENU_BUTTON, NULL);
}
