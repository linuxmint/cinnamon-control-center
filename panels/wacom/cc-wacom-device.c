/*
 * Copyright © 2016 Red Hat, Inc.
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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Carlos Garnacho <carlosg@gnome.org>
 *
 */

#include "config.h"

#include <string.h>
#include "cc-wacom-device.h"
#include "muffin-display-config.h"

enum {
	PROP_0,
	PROP_DEVICE,
	N_PROPS
};

GParamSpec *props[N_PROPS] = { 0 };

typedef struct _CcWacomDevice CcWacomDevice;

struct _CcWacomDevice {
	GObject parent_instance;

	CsdDevice *device;
	WacomDevice *wdevice;

    MetaDBusDisplayConfig      *proxy;

    GList          *monitors;
};

static void cc_wacom_device_initable_iface_init (GInitableIface *iface);

G_DEFINE_TYPE_WITH_CODE (CcWacomDevice, cc_wacom_device, G_TYPE_OBJECT,
             G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE,
                        cc_wacom_device_initable_iface_init))


// G_DEFINE_TYPE (CcWacomDevice, cc_wacom_device, G_TYPE_OBJECT)

enum
{
  MONITORS_CHANGED,
  N_SIGNALS,
};

WacomDeviceDatabase *
cc_wacom_device_database_get (void)
{
	static WacomDeviceDatabase *db = NULL;

	if (g_once_init_enter (&db)) {
		gpointer p = libwacom_database_new ();
		g_once_init_leave (&db, p);
	}

	return db;
}


static void
cc_wacom_device_init (CcWacomDevice *device)
{
}

static void
cc_wacom_device_set_property (GObject      *object,
			      guint         prop_id,
			      const GValue *value,
			      GParamSpec   *pspec)
{
	CcWacomDevice *device = CC_WACOM_DEVICE (object);

	switch (prop_id) {
	case PROP_DEVICE:
		device->device = g_value_get_object (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
cc_wacom_device_get_property (GObject    *object,
			      guint       prop_id,
			      GValue     *value,
			      GParamSpec *pspec)
{
	CcWacomDevice *device = CC_WACOM_DEVICE (object);

	switch (prop_id) {
	case PROP_DEVICE:
		g_value_set_object (value, device->device);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
cc_wacom_device_finalize (GObject *object)
{
	CcWacomDevice *device = CC_WACOM_DEVICE (object);

	g_clear_pointer (&device->wdevice, libwacom_destroy);

	G_OBJECT_CLASS (cc_wacom_device_parent_class)->finalize (object);
}

static void
cc_wacom_device_class_init (CcWacomDeviceClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

    // object_class->constructed = cc_wacom_device_constructed;
	object_class->set_property = cc_wacom_device_set_property;
	object_class->get_property = cc_wacom_device_get_property;
	object_class->finalize = cc_wacom_device_finalize;

	props[PROP_DEVICE] =
		g_param_spec_object ("device",
				     "device",
				     "device",
				     CSD_TYPE_DEVICE,
				     G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

	g_object_class_install_properties (object_class, N_PROPS, props);
}

static gboolean
cc_wacom_device_initable_init (GInitable     *initable,
                   GCancellable  *cancellable,
                   GError       **error)
{
    CcWacomDevice *device = CC_WACOM_DEVICE (initable);
    WacomDeviceDatabase *wacom_db;
    const gchar *node_path;

    wacom_db = cc_wacom_device_database_get ();
    node_path = csd_device_get_device_file (device->device);
    device->wdevice = libwacom_new_from_path (wacom_db, node_path, FALSE, NULL);

    if (!device->wdevice) {
        g_set_error (error, 0, 0, "Tablet description not found");
        return FALSE;
    }

    return TRUE;
}

static void
cc_wacom_device_initable_iface_init (GInitableIface *iface)
{
    iface->init = cc_wacom_device_initable_init;
}

CcWacomDevice *
cc_wacom_device_new (CsdDevice *device)
{
    return g_initable_new (CC_TYPE_WACOM_DEVICE,
                           NULL, NULL,
                           "device", device,
                           NULL);
}

CcWacomDevice *
cc_wacom_device_new_fake (const gchar *name)
{
	CcWacomDevice *device;
	WacomDevice *wacom_device;

	device = g_object_new (CC_TYPE_WACOM_DEVICE,
			       NULL);

	wacom_device = libwacom_new_from_name (cc_wacom_device_database_get(),
					       name, NULL);
	if (wacom_device == NULL)
		return NULL;

	device->wdevice = wacom_device;

	return device;
}

const gchar *
cc_wacom_device_get_name (CcWacomDevice *device)
{
	g_return_val_if_fail (CC_IS_WACOM_DEVICE (device), NULL);

	return libwacom_get_name (device->wdevice);
}

const gchar *
cc_wacom_device_get_icon_name (CcWacomDevice *device)
{
	WacomIntegrationFlags integration_flags;

	g_return_val_if_fail (CC_IS_WACOM_DEVICE (device), NULL);

	integration_flags = libwacom_get_integration_flags (device->wdevice);

	if (integration_flags & WACOM_DEVICE_INTEGRATED_SYSTEM) {
		return "wacom-tablet-pc";
	} else if (integration_flags & WACOM_DEVICE_INTEGRATED_DISPLAY) {
		return "wacom-tablet-cintiq";
	} else {
		return "wacom-tablet";
	}
}

gboolean
cc_wacom_device_is_reversible (CcWacomDevice *device)
{
	g_return_val_if_fail (CC_IS_WACOM_DEVICE (device), FALSE);

	return libwacom_is_reversible (device->wdevice);
}

WacomIntegrationFlags
cc_wacom_device_get_integration_flags (CcWacomDevice *device)
{
	g_return_val_if_fail (CC_IS_WACOM_DEVICE (device), 0);

	return libwacom_get_integration_flags (device->wdevice);
}

CsdDevice *
cc_wacom_device_get_device (CcWacomDevice *device)
{
	g_return_val_if_fail (CC_IS_WACOM_DEVICE (device), NULL);

	return device->device;
}

GSettings *
cc_wacom_device_get_settings (CcWacomDevice *device)
{
	g_return_val_if_fail (CC_IS_WACOM_DEVICE (device), NULL);

	return csd_device_get_settings (device->device);
}

const gint *
cc_wacom_device_get_supported_tools (CcWacomDevice *device,
				     gint          *n_tools)
{
	*n_tools = 0;

	g_return_val_if_fail (CC_IS_WACOM_DEVICE (device), NULL);

	return libwacom_get_supported_styli (device->wdevice, n_tools);
}

static MonitorInfo *
find_monitor (CcWacomDevice *device)
{
    GList *monitors, *l;
    g_autoptr(GSettings) settings = NULL;
    g_autoptr(GVariant) variant = NULL;
    g_autofree const gchar **edid = NULL;
    gsize n;

    settings = cc_wacom_device_get_settings (device);
    variant = g_settings_get_value (settings, "output");
    edid = g_variant_get_strv (variant, &n);

    if (n != 3) {
        g_critical ("Expected 'output' key to store %d values; got %"G_GSIZE_FORMAT".", 3, n);
        return NULL;
    }

    if (strlen (edid[0]) == 0 || strlen (edid[1]) == 0 || strlen (edid[2]) == 0)
        return NULL;

    monitors = cc_wacom_output_manager_get_all_monitors (cc_wacom_output_manager_get ());

    for (l = monitors; l != NULL; l = l->next)
    {
        MonitorInfo *info = (MonitorInfo *) l->data;

        if (g_strcmp0 (info->vendor, edid[0]) == 0 &&
            g_strcmp0 (info->product, edid[1]) == 0 &&
            g_strcmp0 (info->serial, edid[2]) == 0)
            return info;
    }

    return NULL;
}


MonitorInfo *
cc_wacom_device_get_monitor (CcWacomDevice *device)
{
    MonitorInfo *monitor;

    g_return_val_if_fail (CC_IS_WACOM_DEVICE (device), NULL);

    monitor = find_monitor (device);

    if (monitor == NULL) {
        return NULL;
    }

	return monitor;
}

void
cc_wacom_device_set_monitor (CcWacomDevice *device,
                             MonitorInfo   *monitor)
{
    g_autoptr(GSettings) settings = NULL;
    const gchar *values[] = { "", "", "", NULL };

    g_return_if_fail (CC_IS_WACOM_DEVICE (device));

    settings = cc_wacom_device_get_settings (device);

    if (monitor != NULL) {
        values[0] = monitor->vendor;
        values[1] = monitor->product;
        values[2] = monitor->serial;
    }

    g_settings_set_strv (settings, "output", values);
}

guint
cc_wacom_device_get_num_buttons (CcWacomDevice *device)
{
    g_return_val_if_fail (CC_IS_WACOM_DEVICE (device), 0);

    return libwacom_get_num_buttons (device->wdevice);
}

GSettings *
cc_wacom_device_get_button_settings (CcWacomDevice *device,
				     guint          button)
{
	g_autoptr(GSettings) tablet_settings = NULL;
	GSettings *settings;
	g_autofree gchar *path = NULL;
	g_autofree gchar *button_path = NULL;

	g_return_val_if_fail (CC_IS_WACOM_DEVICE (device), NULL);

	if (button > cc_wacom_device_get_num_buttons (device))
		return NULL;

	tablet_settings = cc_wacom_device_get_settings (device);
	g_object_get (tablet_settings, "path", &path, NULL);

	button_path = g_strdup_printf ("%sbutton%c/", path, 'A' + button);
	settings = g_settings_new_with_path ("org.cinnamon.desktop.peripherals.tablet.pad-button",
					     button_path);

	return settings;
}

