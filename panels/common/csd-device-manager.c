/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2014 Red Hat
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Author: Carlos Garnacho <carlosg@gnome.org>
 */

#include "config.h"

#include <string.h>
#include <gudev/gudev.h>

#include "csd-device-manager.h"
#include "csd-common-enums.h"
#include "csd-input-helper.h"

#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif
#ifdef GDK_WINDOWING_WAYLAND
#include <gdk/gdkwayland.h>
#endif

typedef struct
{
	gchar *name;
	gchar *device_file;
	gchar *vendor_id;
	gchar *product_id;
	CsdDeviceType type;
	guint width;
	guint height;
} CsdDevicePrivate;

G_DEFINE_TYPE_WITH_PRIVATE (CsdDevice, csd_device, G_TYPE_OBJECT)

typedef struct
{
        GObject parent_instance;
	GHashTable *devices;
	GUdevClient *udev_client;
} CsdDeviceManagerPrivate;

enum {
	PROP_NAME = 1,
	PROP_DEVICE_FILE,
	PROP_VENDOR_ID,
	PROP_PRODUCT_ID,
	PROP_TYPE,
	PROP_WIDTH,
	PROP_HEIGHT
};

enum {
	DEVICE_ADDED,
	DEVICE_REMOVED,
	DEVICE_CHANGED,
	N_SIGNALS
};

/* Index matches CsdDeviceType */
const gchar *udev_ids[] = {
	"ID_INPUT_MOUSE",
	"ID_INPUT_KEYBOARD",
	"ID_INPUT_TOUCHPAD",
	"ID_INPUT_TABLET",
	"ID_INPUT_TOUCHSCREEN",
	"ID_INPUT_TABLET_PAD",
};

static guint signals[N_SIGNALS] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE (CsdDeviceManager, csd_device_manager, G_TYPE_OBJECT)

static void
csd_device_init (CsdDevice *device)
{
}

static void
csd_device_set_property (GObject      *object,
			 guint	       prop_id,
			 const GValue *value,
			 GParamSpec   *pspec)
{
	CsdDevicePrivate *priv;

	priv = csd_device_get_instance_private (CSD_DEVICE (object));

	switch (prop_id) {
	case PROP_NAME:
		priv->name = g_value_dup_string (value);
		break;
	case PROP_DEVICE_FILE:
		priv->device_file = g_value_dup_string (value);
		break;
	case PROP_VENDOR_ID:
		priv->vendor_id = g_value_dup_string (value);
		break;
	case PROP_PRODUCT_ID:
		priv->product_id = g_value_dup_string (value);
		break;
	case PROP_TYPE:
		priv->type = g_value_get_flags (value);
		break;
	case PROP_WIDTH:
		priv->width = g_value_get_uint (value);
		break;
	case PROP_HEIGHT:
		priv->height = g_value_get_uint (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
csd_device_get_property (GObject    *object,
			 guint	     prop_id,
			 GValue	    *value,
			 GParamSpec *pspec)
{
	CsdDevicePrivate *priv;

	priv = csd_device_get_instance_private (CSD_DEVICE (object));

	switch (prop_id) {
	case PROP_NAME:
		g_value_set_string (value, priv->name);
		break;
	case PROP_DEVICE_FILE:
		g_value_set_string (value, priv->device_file);
		break;
	case PROP_VENDOR_ID:
		g_value_set_string (value, priv->vendor_id);
		break;
	case PROP_PRODUCT_ID:
		g_value_set_string (value, priv->product_id);
		break;
	case PROP_TYPE:
		g_value_set_flags (value, priv->type);
		break;
	case PROP_WIDTH:
		g_value_set_uint (value, priv->width);
		break;
	case PROP_HEIGHT:
		g_value_set_uint (value, priv->height);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
csd_device_finalize (GObject *object)
{
	CsdDevicePrivate *priv;

	priv = csd_device_get_instance_private (CSD_DEVICE (object));

	g_free (priv->name);
	g_free (priv->vendor_id);
	g_free (priv->product_id);
	g_free (priv->device_file);

	G_OBJECT_CLASS (csd_device_parent_class)->finalize (object);
}

static void
csd_device_class_init (CsdDeviceClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->set_property = csd_device_set_property;
	object_class->get_property = csd_device_get_property;
	object_class->finalize = csd_device_finalize;

	g_object_class_install_property (object_class,
					 PROP_NAME,
					 g_param_spec_string ("name",
							      "Name",
							      "Name",
							      NULL,
							      G_PARAM_READWRITE |
							      G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property (object_class,
					 PROP_DEVICE_FILE,
					 g_param_spec_string ("device-file",
							      "Device file",
							      "Device file",
							      NULL,
							      G_PARAM_READWRITE |
							      G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property (object_class,
					 PROP_VENDOR_ID,
					 g_param_spec_string ("vendor-id",
							      "Vendor ID",
							      "Vendor ID",
							      NULL,
							      G_PARAM_READWRITE |
							      G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property (object_class,
					 PROP_PRODUCT_ID,
					 g_param_spec_string ("product-id",
							      "Product ID",
							      "Product ID",
							      NULL,
							      G_PARAM_READWRITE |
							      G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property (object_class,
					 PROP_TYPE,
					 g_param_spec_flags ("type",
							     "Device type",
							     "Device type",
							     CSD_TYPE_DEVICE_TYPE, 0,
							     G_PARAM_READWRITE |
							     G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property (object_class,
					 PROP_WIDTH,
					 g_param_spec_uint ("width",
							    "Width",
							    "Width",
							    0, G_MAXUINT, 0,
							    G_PARAM_READWRITE |
							    G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property (object_class,
					 PROP_HEIGHT,
					 g_param_spec_uint ("height",
							    "Height",
							    "Height",
							    0, G_MAXUINT, 0,
							    G_PARAM_READWRITE |
							    G_PARAM_CONSTRUCT_ONLY));
}

static void
csd_device_manager_finalize (GObject *object)
{
	CsdDeviceManager *manager = CSD_DEVICE_MANAGER (object);
        CsdDeviceManagerPrivate *priv = csd_device_manager_get_instance_private (manager);

	g_hash_table_destroy (priv->devices);
	g_object_unref (priv->udev_client);

	G_OBJECT_CLASS (csd_device_manager_parent_class)->finalize (object);
}

static GList *
csd_device_manager_real_list_devices (CsdDeviceManager *manager,
				      CsdDeviceType	type)
{
        CsdDeviceManagerPrivate *priv = csd_device_manager_get_instance_private (manager);
	CsdDeviceType device_type;
	GList *devices = NULL;
	GHashTableIter iter;
	CsdDevice *device;
	g_hash_table_iter_init (&iter, priv->devices);

	while (g_hash_table_iter_next (&iter, NULL, (gpointer *) &device)) {

		device_type = csd_device_get_device_type (device);

		if ((device_type & type) == type)
			devices = g_list_prepend (devices, device);
	}

	return devices;
}

static CsdDevice *
csd_device_manager_real_lookup_device (CsdDeviceManager *manager,
                                       GdkDevice	*gdk_device)
{
	CsdDeviceManagerPrivate *priv = csd_device_manager_get_instance_private (manager);
	GdkDisplay *display = gdk_device_get_display (gdk_device);
	const gchar *node_path = NULL;
	GHashTableIter iter;
	CsdDevice *device;

#ifdef GDK_WINDOWING_X11
	if (GDK_IS_X11_DISPLAY (display))
		node_path = xdevice_get_device_node (gdk_x11_device_get_id (gdk_device));
#endif
#ifdef GDK_WINDOWING_WAYLAND
	if (GDK_IS_WAYLAND_DISPLAY (display))
		node_path = g_strdup (gdk_wayland_device_get_node_path (gdk_device));
#endif
	if (!node_path)
		return NULL;

	g_hash_table_iter_init (&iter, priv->devices);

	while (g_hash_table_iter_next (&iter, NULL, (gpointer *) &device)) {
		if (g_strcmp0 (node_path,
			       csd_device_get_device_file (device)) == 0) {
			return device;
		}
	}

	return NULL;
}

static void
csd_device_manager_class_init (CsdDeviceManagerClass *klass)
{
	CsdDeviceManagerClass *manager_class = CSD_DEVICE_MANAGER_CLASS (klass);
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = csd_device_manager_finalize;
	manager_class->list_devices = csd_device_manager_real_list_devices;
	manager_class->lookup_device = csd_device_manager_real_lookup_device;

	signals[DEVICE_ADDED] =
		g_signal_new ("device-added",
			      CSD_TYPE_DEVICE_MANAGER,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (CsdDeviceManagerClass, device_added),
			      NULL, NULL, NULL,
			      G_TYPE_NONE, 1,
			      CSD_TYPE_DEVICE | G_SIGNAL_TYPE_STATIC_SCOPE);

	signals[DEVICE_REMOVED] =
		g_signal_new ("device-removed",
			      CSD_TYPE_DEVICE_MANAGER,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (CsdDeviceManagerClass, device_removed),
			      NULL, NULL, NULL,
			      G_TYPE_NONE, 1,
			      CSD_TYPE_DEVICE | G_SIGNAL_TYPE_STATIC_SCOPE);

	signals[DEVICE_CHANGED] =
		g_signal_new ("device-changed",
			      CSD_TYPE_DEVICE_MANAGER,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (CsdDeviceManagerClass, device_changed),
			      NULL, NULL, NULL,
			      G_TYPE_NONE, 1,
			      CSD_TYPE_DEVICE | G_SIGNAL_TYPE_STATIC_SCOPE);
}

static CsdDeviceType
udev_device_get_device_type (GUdevDevice *device)
{
	CsdDeviceType type = 0;
	gint i;

	for (i = 0; i < G_N_ELEMENTS (udev_ids); i++) {
		if (g_udev_device_get_property_as_boolean (device, udev_ids[i]))
			type |= (1 << i);
	}

	return type;
}

static gboolean
device_is_evdev (GUdevDevice *device)
{
	const gchar *device_file;

	device_file = g_udev_device_get_device_file (device);

	if (!device_file || strstr (device_file, "/event") == NULL)
		return FALSE;

	return g_udev_device_get_property_as_boolean (device, "ID_INPUT");
}

static CsdDevice *
create_device (GUdevDevice *udev_device)
{
	const gchar *vendor, *product, *name;
	guint width, height;
	g_autoptr(GUdevDevice) parent = NULL;

	parent = g_udev_device_get_parent (udev_device);
	g_assert (parent != NULL);

	name = g_udev_device_get_sysfs_attr (parent, "name");
	vendor = g_udev_device_get_property (udev_device, "ID_VENDOR_ID");
	product = g_udev_device_get_property (udev_device, "ID_MODEL_ID");

	if (!vendor || !product) {
		vendor = g_udev_device_get_sysfs_attr (udev_device, "device/id/vendor");
		product = g_udev_device_get_sysfs_attr (udev_device, "device/id/product");
	}

	width = g_udev_device_get_property_as_int (udev_device, "ID_INPUT_WIDTH_MM");
	height = g_udev_device_get_property_as_int (udev_device, "ID_INPUT_HEIGHT_MM");

	return g_object_new (CSD_TYPE_DEVICE,
			     "name", name,
			     "device-file", g_udev_device_get_device_file (udev_device),
			     "type", udev_device_get_device_type (udev_device),
			     "vendor-id", vendor,
			     "product-id", product,
			     "width", width,
			     "height", height,
			     NULL);
}

static void
add_device (CsdDeviceManager *manager,
	    GUdevDevice	     *udev_device)
{
        CsdDeviceManagerPrivate *priv = csd_device_manager_get_instance_private (manager);
	GUdevDevice *parent;
	CsdDevice *device;
	const gchar *syspath;

	parent = g_udev_device_get_parent (udev_device);

	if (!parent)
		return;

	device = create_device (udev_device);
	syspath = g_udev_device_get_sysfs_path (udev_device);
	g_hash_table_insert (priv->devices, g_strdup (syspath), device);
	g_signal_emit_by_name (manager, "device-added", device);
}

static void
remove_device (CsdDeviceManager *manager,
	       GUdevDevice	*udev_device)
{
	CsdDeviceManagerPrivate *priv = csd_device_manager_get_instance_private (manager);
	CsdDevice *device;
	const gchar *syspath;

	syspath = g_udev_device_get_sysfs_path (udev_device);
	device = g_hash_table_lookup (priv->devices, syspath);

	if (!device)
		return;

	g_hash_table_steal (priv->devices, syspath);
	g_signal_emit_by_name (manager, "device-removed", device);

	g_object_unref (device);
}

static void
udev_event_cb (GUdevClient	*client,
	       gchar		*action,
	       GUdevDevice	*device,
	       CsdDeviceManager *manager)
{
	if (!device_is_evdev (device))
		return;

	if (g_strcmp0 (action, "add") == 0) {
		add_device (manager, device);
	} else if (g_strcmp0 (action, "remove") == 0) {
		remove_device (manager, device);
	}
}

static void
csd_device_manager_init (CsdDeviceManager *manager)
{
        CsdDeviceManagerPrivate *priv = csd_device_manager_get_instance_private (manager);
	const gchar *subsystems[] = { "input", NULL };
	g_autoptr(GList) devices = NULL;
	GList *l;

	priv->devices = g_hash_table_new_full (g_str_hash, g_str_equal,
					       (GDestroyNotify) g_free,
					       (GDestroyNotify) g_object_unref);

	priv->udev_client = g_udev_client_new (subsystems);
	g_signal_connect (priv->udev_client, "uevent",
			  G_CALLBACK (udev_event_cb), manager);

	devices = g_udev_client_query_by_subsystem (priv->udev_client,
						    subsystems[0]);

	for (l = devices; l; l = l->next) {
		g_autoptr(GUdevDevice) device = l->data;

		if (device_is_evdev (device))
			add_device (manager, device);
	}
}

CsdDeviceManager *
csd_device_manager_get (void)
{
	CsdDeviceManager *manager;
	GdkScreen *screen;

	screen = gdk_screen_get_default ();
	g_return_val_if_fail (screen != NULL, NULL);

	manager = g_object_get_data (G_OBJECT (screen), "csd-device-manager-data");

	if (!manager) {
                manager = g_object_new (CSD_TYPE_DEVICE_MANAGER,
                                        NULL);

		g_object_set_data_full (G_OBJECT (screen), "csd-device-manager-data",
					manager, (GDestroyNotify) g_object_unref);
	}

	return manager;
}

GList *
csd_device_manager_list_devices (CsdDeviceManager *manager,
				 CsdDeviceType	   type)
{
	g_return_val_if_fail (CSD_IS_DEVICE_MANAGER (manager), NULL);

	return CSD_DEVICE_MANAGER_GET_CLASS (manager)->list_devices (manager, type);
}

CsdDeviceType
csd_device_get_device_type (CsdDevice *device)
{
	CsdDevicePrivate *priv;

	g_return_val_if_fail (CSD_IS_DEVICE (device), 0);

	priv = csd_device_get_instance_private (device);

	return priv->type;
}

void
csd_device_get_device_ids (CsdDevice	*device,
			   const gchar **vendor,
			   const gchar **product)
{
	CsdDevicePrivate *priv;

	g_return_if_fail (CSD_IS_DEVICE (device));

	priv = csd_device_get_instance_private (device);

	if (vendor)
		*vendor = priv->vendor_id;
	if (product)
		*product = priv->product_id;
}

GSettings *
csd_device_get_settings (CsdDevice *device)
{
	const gchar *schema = NULL, *vendor, *product;
	CsdDeviceType type;
	g_autofree gchar *path = NULL;

	g_return_val_if_fail (CSD_IS_DEVICE (device), NULL);

	type = csd_device_get_device_type (device);

	if (type & (CSD_DEVICE_TYPE_TOUCHSCREEN | CSD_DEVICE_TYPE_TABLET)) {
		csd_device_get_device_ids (device, &vendor, &product);

		if (type & CSD_DEVICE_TYPE_TOUCHSCREEN) {
			schema = "org.cinnamon.desktop.peripherals.touchscreen";
			path = g_strdup_printf ("/org/cinnamon/desktop/peripherals/touchscreens/%s:%s/",
						vendor, product);
		} else if (type & CSD_DEVICE_TYPE_TABLET) {
			schema = "org.cinnamon.desktop.peripherals.tablet";
			path = g_strdup_printf ("/org/cinnamon/desktop/peripherals/tablets/%s:%s/",
						vendor, product);
		}
	} else if (type & (CSD_DEVICE_TYPE_MOUSE | CSD_DEVICE_TYPE_TOUCHPAD)) {
		schema = "org.cinnamon.desktop.peripherals.mouse";
	} else if (type & CSD_DEVICE_TYPE_KEYBOARD) {
		schema = "org.cinnamon.desktop.peripherals.keyboard";
	} else {
		return NULL;
	}

	if (path) {
		return g_settings_new_with_path (schema, path);
	} else {
		return g_settings_new (schema);
	}
}

const gchar *
csd_device_get_name (CsdDevice *device)
{
	CsdDevicePrivate *priv;

	g_return_val_if_fail (CSD_IS_DEVICE (device), NULL);

	priv = csd_device_get_instance_private (device);

	return priv->name;
}

const gchar *
csd_device_get_device_file (CsdDevice *device)
{
	CsdDevicePrivate *priv;

	g_return_val_if_fail (CSD_IS_DEVICE (device), NULL);

	priv = csd_device_get_instance_private (device);

	return priv->device_file;
}

gboolean
csd_device_get_dimensions (CsdDevice *device,
			   guint     *width,
			   guint     *height)
{
	CsdDevicePrivate *priv;

	g_return_val_if_fail (CSD_IS_DEVICE (device), FALSE);

	priv = csd_device_get_instance_private (device);

	if (width)
		*width = priv->width;
	if (height)
		*height = priv->height;

	return priv->width > 0 && priv->height > 0;
}

CsdDevice *
csd_device_manager_lookup_gdk_device (CsdDeviceManager *manager,
				      GdkDevice	       *gdk_device)
{
	CsdDeviceManagerClass *klass;

	g_return_val_if_fail (CSD_IS_DEVICE_MANAGER (manager), NULL);
	g_return_val_if_fail (GDK_IS_DEVICE (gdk_device), NULL);

	klass = CSD_DEVICE_MANAGER_GET_CLASS (manager);
	if (!klass->lookup_device)
		return NULL;

	return klass->lookup_device (manager, gdk_device);
}
