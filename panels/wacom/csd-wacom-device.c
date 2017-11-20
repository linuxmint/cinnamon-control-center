/*
 * Copyright (C) 2011 Red Hat, Inc.
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Author: Bastien Nocera <hadess@hadess.net>
 *
 */

#include "config.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <X11/Xatom.h>
#define GNOME_DESKTOP_USE_UNSTABLE_API
#include <libcinnamon-desktop/gnome-rr.h>

#include <libwacom/libwacom.h>
#include <X11/extensions/XInput.h>
#include <X11/extensions/XInput2.h>

#include "csd-input-helper.h"

#include "csd-enums.h"
#include "csd-wacom-device.h"

#define CSD_WACOM_STYLUS_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), CSD_TYPE_WACOM_STYLUS, CsdWacomStylusPrivate))

#define WACOM_TABLET_SCHEMA "org.cinnamon.settings-daemon.peripherals.wacom"
#define WACOM_DEVICE_CONFIG_BASE "/org/cinnamon/settings-daemon/peripherals/wacom/%s-%s/"
#define WACOM_STYLUS_SCHEMA "org.cinnamon.settings-daemon.peripherals.wacom.stylus"
#define WACOM_ERASER_SCHEMA "org.cinnamon.settings-daemon.peripherals.wacom.eraser"
#define WACOM_BUTTON_SCHEMA "org.cinnamon.settings-daemon.peripherals.wacom.tablet-button"

static struct {
	GnomeRRRotation  rotation;
	CsdWacomRotation rotation_wacom;
	const gchar     *rotation_string;
} rotation_table[] = {
	{ GNOME_RR_ROTATION_0,   CSD_WACOM_ROTATION_NONE, "none" },
	{ GNOME_RR_ROTATION_90,  CSD_WACOM_ROTATION_CCW,  "ccw"  },
	{ GNOME_RR_ROTATION_180, CSD_WACOM_ROTATION_HALF, "half" },
	{ GNOME_RR_ROTATION_270, CSD_WACOM_ROTATION_CW,   "cw"   }
};

static WacomDeviceDatabase *db = NULL;

struct CsdWacomStylusPrivate
{
	CsdWacomDevice *device;
	int id;
	WacomStylusType type;
	char *name;
	const char *icon_name;
	GSettings *settings;
	gboolean has_eraser;
	int num_buttons;
};

static void     csd_wacom_stylus_class_init  (CsdWacomStylusClass *klass);
static void     csd_wacom_stylus_init        (CsdWacomStylus      *wacom_stylus);
static void     csd_wacom_stylus_finalize    (GObject              *object);

G_DEFINE_TYPE (CsdWacomStylus, csd_wacom_stylus, G_TYPE_OBJECT)

static void
csd_wacom_stylus_class_init (CsdWacomStylusClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->finalize = csd_wacom_stylus_finalize;

        g_type_class_add_private (klass, sizeof (CsdWacomStylusPrivate));
}

static void
csd_wacom_stylus_init (CsdWacomStylus *stylus)
{
        stylus->priv = CSD_WACOM_STYLUS_GET_PRIVATE (stylus);
}

static void
csd_wacom_stylus_finalize (GObject *object)
{
        CsdWacomStylus *stylus;
        CsdWacomStylusPrivate *p;

        g_return_if_fail (object != NULL);
        g_return_if_fail (CSD_IS_WACOM_STYLUS (object));

        stylus = CSD_WACOM_STYLUS (object);

        g_return_if_fail (stylus->priv != NULL);

	p = stylus->priv;

        if (p->settings != NULL) {
                g_object_unref (p->settings);
                p->settings = NULL;
        }

        g_free (p->name);
        p->name = NULL;

        G_OBJECT_CLASS (csd_wacom_stylus_parent_class)->finalize (object);
}

static const char *
get_icon_name_from_type (const WacomStylusType *wstylus)
{
	WacomStylusType type = libwacom_stylus_get_type (wstylus);

	switch (type) {
	case WSTYLUS_INKING:
	case WSTYLUS_STROKE:
		/* The stroke pen is the same as the inking pen with
		 * a different nib */
		return "wacom-stylus-inking";
	case WSTYLUS_AIRBRUSH:
		return "wacom-stylus-airbrush";
	case WSTYLUS_MARKER:
		return "wacom-stylus-art-pen";
	case WSTYLUS_CLASSIC:
		return "wacom-stylus-classic";
	default:
		if (!libwacom_stylus_has_eraser (wstylus))
			return "wacom-stylus-no-eraser";
		return "wacom-stylus";
	}
}

static CsdWacomStylus *
csd_wacom_stylus_new (CsdWacomDevice    *device,
		      const WacomStylus *wstylus,
		      GSettings         *settings)
{
	CsdWacomStylus *stylus;

	g_return_val_if_fail (G_IS_SETTINGS (settings), NULL);
	g_return_val_if_fail (wstylus != NULL, NULL);

	stylus = CSD_WACOM_STYLUS (g_object_new (CSD_TYPE_WACOM_STYLUS,
						 NULL));
	stylus->priv->device = device;
	stylus->priv->id = libwacom_stylus_get_id (wstylus);
	stylus->priv->name = g_strdup (libwacom_stylus_get_name (wstylus));
	stylus->priv->settings = settings;
	stylus->priv->type = libwacom_stylus_get_type (wstylus);
	stylus->priv->icon_name = get_icon_name_from_type (wstylus);
	stylus->priv->has_eraser = libwacom_stylus_has_eraser (wstylus);
	stylus->priv->num_buttons = libwacom_stylus_get_num_buttons (wstylus);

	return stylus;
}

GSettings *
csd_wacom_stylus_get_settings (CsdWacomStylus *stylus)
{
	g_return_val_if_fail (CSD_IS_WACOM_STYLUS (stylus), NULL);

	return stylus->priv->settings;
}

const char *
csd_wacom_stylus_get_name (CsdWacomStylus *stylus)
{
	g_return_val_if_fail (CSD_IS_WACOM_STYLUS (stylus), NULL);

	return stylus->priv->name;
}

const char *
csd_wacom_stylus_get_icon_name (CsdWacomStylus *stylus)
{
	g_return_val_if_fail (CSD_IS_WACOM_STYLUS (stylus), NULL);

	return stylus->priv->icon_name;
}

CsdWacomDevice *
csd_wacom_stylus_get_device (CsdWacomStylus *stylus)
{
	g_return_val_if_fail (CSD_IS_WACOM_STYLUS (stylus), NULL);

	return stylus->priv->device;
}

gboolean
csd_wacom_stylus_get_has_eraser (CsdWacomStylus *stylus)
{
	g_return_val_if_fail (CSD_IS_WACOM_STYLUS (stylus), FALSE);

	return stylus->priv->has_eraser;
}

guint
csd_wacom_stylus_get_num_buttons (CsdWacomStylus *stylus)
{
	g_return_val_if_fail (CSD_IS_WACOM_STYLUS (stylus), -1);

	return stylus->priv->num_buttons;
}

CsdWacomStylusType
csd_wacom_stylus_get_stylus_type (CsdWacomStylus *stylus)
{
	g_return_val_if_fail (CSD_IS_WACOM_STYLUS (stylus), WACOM_STYLUS_TYPE_UNKNOWN);

	switch (stylus->priv->type) {
	case WSTYLUS_UNKNOWN:
		return WACOM_STYLUS_TYPE_UNKNOWN;
	case WSTYLUS_GENERAL:
		return WACOM_STYLUS_TYPE_GENERAL;
	case WSTYLUS_INKING:
		return WACOM_STYLUS_TYPE_INKING;
	case WSTYLUS_AIRBRUSH:
		return WACOM_STYLUS_TYPE_AIRBRUSH;
	case WSTYLUS_CLASSIC:
		return WACOM_STYLUS_TYPE_CLASSIC;
	case WSTYLUS_MARKER:
		return WACOM_STYLUS_TYPE_MARKER;
	case WSTYLUS_STROKE:
		return WACOM_STYLUS_TYPE_STROKE;
	case WSTYLUS_PUCK:
		return WACOM_STYLUS_TYPE_PUCK;
	default:
		g_assert_not_reached ();
	}

	return WACOM_STYLUS_TYPE_UNKNOWN;
}

int
csd_wacom_stylus_get_id (CsdWacomStylus *stylus)
{
	g_return_val_if_fail (CSD_IS_WACOM_STYLUS (stylus), -1);

	return stylus->priv->id;
}

/* Tablet buttons */
static CsdWacomTabletButton *
csd_wacom_tablet_button_new (const char               *name,
			     const char               *id,
			     const char               *settings_path,
			     CsdWacomTabletButtonType  type,
			     CsdWacomTabletButtonPos   pos,
			     int                       group_id,
			     int                       idx,
			     int                       status_led)
{
	CsdWacomTabletButton *ret;

	ret = g_new0 (CsdWacomTabletButton, 1);
	ret->name = g_strdup (name);
	ret->id = g_strdup (id);
	if (type != WACOM_TABLET_BUTTON_TYPE_HARDCODED) {
		char *button_settings_path;

		button_settings_path = g_strdup_printf ("%s%s/", settings_path, id);
		ret->settings = g_settings_new_with_path (WACOM_BUTTON_SCHEMA, button_settings_path);
		g_free (button_settings_path);
	}
	ret->group_id = group_id;
	ret->idx = idx;
	ret->type = type;
	ret->pos = pos;
	ret->status_led = status_led;

	return ret;
}

void
csd_wacom_tablet_button_free (CsdWacomTabletButton *button)
{
	g_return_if_fail (button != NULL);

	if (button->settings != NULL)
		g_object_unref (button->settings);
	g_free (button->name);
	g_free (button->id);
	g_free (button);
}

CsdWacomTabletButton *
csd_wacom_tablet_button_copy (CsdWacomTabletButton *button)
{
	CsdWacomTabletButton *ret;

	g_return_val_if_fail (button != NULL, NULL);

	ret = g_new0 (CsdWacomTabletButton, 1);
	ret->name = g_strdup (button->name);
	if (button->settings != NULL)
		ret->settings = g_object_ref (button->settings);
	ret->id = button->id;
	ret->type = button->type;
	ret->group_id = button->group_id;

	return ret;
}

#define CSD_WACOM_DEVICE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), CSD_TYPE_WACOM_DEVICE, CsdWacomDevicePrivate))

/* we support two types of settings:
 * Tablet-wide settings: applied to each tool on the tablet. e.g. rotation
 * Tool-specific settings: applied to one tool only.
 */
#define SETTINGS_WACOM_DIR         "org.cinnamon.settings-daemon.peripherals.wacom"
#define SETTINGS_STYLUS_DIR        "stylus"
#define SETTINGS_ERASER_DIR        "eraser"

struct CsdWacomDevicePrivate
{
	GdkDevice *gdk_device;
	int device_id;
	int opcode;

	CsdWacomDeviceType type;
	char *name;
	char *path;
	char *machine_id;
	const char *icon_name;
	char *layout_path;
	char *tool_name;
	gboolean reversible;
	gboolean is_screen_tablet;
	gboolean is_isd; /* integrated system device */
	gboolean is_fallback;
	GList *styli;
	CsdWacomStylus *last_stylus;
	GList *buttons;
	gint num_rings;
	gint num_strips;
	GHashTable *modes; /* key = int (group), value = int (index) */
	GHashTable *num_modes; /* key = int (group), value = int (index) */
	GSettings *wacom_settings;
};

enum {
	PROP_0,
	PROP_GDK_DEVICE,
	PROP_LAST_STYLUS
};

static void     csd_wacom_device_class_init  (CsdWacomDeviceClass *klass);
static void     csd_wacom_device_init        (CsdWacomDevice      *wacom_device);
static void     csd_wacom_device_finalize    (GObject              *object);

G_DEFINE_TYPE (CsdWacomDevice, csd_wacom_device, G_TYPE_OBJECT)

static GdkFilterReturn
filter_events (XEvent         *xevent,
               GdkEvent       *event,
               CsdWacomDevice *device)
{
	XIEvent             *xiev;
	XIPropertyEvent     *pev;
	XGenericEventCookie *cookie;
	char                *name;
	int                  tool_id;

        /* verify we have a property event */
	if (xevent->type != GenericEvent)
		return GDK_FILTER_CONTINUE;

	cookie = &xevent->xcookie;
	if (cookie->extension != device->priv->opcode)
		return GDK_FILTER_CONTINUE;

	xiev = (XIEvent *) xevent->xcookie.data;

	if (xiev->evtype != XI_PropertyEvent)
		return GDK_FILTER_CONTINUE;

	pev = (XIPropertyEvent *) xiev;

	/* Is the event for us? */
	if (pev->deviceid != device->priv->device_id)
		return GDK_FILTER_CONTINUE;

	name = XGetAtomName (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), pev->property);
	if (name == NULL ||
	    g_strcmp0 (name, WACOM_SERIAL_IDS_PROP) != 0) {
		if (name)
			XFree (name);
		return GDK_FILTER_CONTINUE;
	}
	XFree (name);

	tool_id = xdevice_get_last_tool_id (device->priv->device_id);
	if (tool_id == -1) {
		g_warning ("Failed to get value for changed stylus ID on device '%d'", device->priv->device_id);
		return GDK_FILTER_CONTINUE;
	}
	csd_wacom_device_set_current_stylus (device, tool_id);

	return GDK_FILTER_CONTINUE;
}

static gboolean
setup_property_notify (CsdWacomDevice *device)
{
	Display *dpy;
	XIEventMask evmask;
	int tool_id;

	evmask.deviceid = device->priv->device_id;
	evmask.mask_len = XIMaskLen (XI_PropertyEvent);
	evmask.mask = g_new0 (guchar, evmask.mask_len);
	XISetMask (evmask.mask, XI_PropertyEvent);

	dpy = GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());
	XISelectEvents (dpy, DefaultRootWindow (dpy), &evmask, 1);

	g_free (evmask.mask);

	gdk_window_add_filter (NULL,
			       (GdkFilterFunc) filter_events,
			       device);

	tool_id = xdevice_get_last_tool_id (device->priv->device_id);
	if (tool_id == -1) {
		g_warning ("Failed to get value for changed stylus ID on device '%d", device->priv->device_id);
		return TRUE;
	}
	csd_wacom_device_set_current_stylus (device, tool_id);

	return TRUE;
}

static CsdWacomDeviceType
get_device_type (XDeviceInfo *dev)
{
	CsdWacomDeviceType ret;
        static Atom stylus, cursor, eraser, pad, touch, prop;
        XDevice *device;
        Atom realtype;
        int realformat;
        unsigned long nitems, bytes_after;
        unsigned char *data = NULL;
        int rc;

        ret = WACOM_TYPE_INVALID;

        if ((dev->use == IsXPointer) || (dev->use == IsXKeyboard))
                return ret;

        if (!stylus)
                stylus = XInternAtom (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), "STYLUS", False);
        if (!eraser)
                eraser = XInternAtom (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), "ERASER", False);
        if (!cursor)
                cursor = XInternAtom (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), "CURSOR", False);
        if (!pad)
                pad = XInternAtom (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), "PAD", False);
        if (!touch)
                touch = XInternAtom (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), "TOUCH", False);
        if (!prop)
		prop = XInternAtom (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), "Wacom Tool Type", False);

	if (dev->type == stylus)
		ret = WACOM_TYPE_STYLUS;
	else if (dev->type == eraser)
		ret = WACOM_TYPE_ERASER;
	else if (dev->type == cursor)
		ret = WACOM_TYPE_CURSOR;
	else if (dev->type == pad)
		ret = WACOM_TYPE_PAD;
	else if (dev->type == touch)
		ret = WACOM_TYPE_TOUCH;

	if (ret == WACOM_TYPE_INVALID)
		return ret;

        /* There is currently no good way of detecting the driver for a device
         * other than checking for a driver-specific property.
         * Wacom Tool Type exists on all tools
         */
        gdk_error_trap_push ();
        device = XOpenDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), dev->id);
        if (gdk_error_trap_pop () || (device == NULL))
                return ret;

        gdk_error_trap_push ();

        rc = XGetDeviceProperty (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()),
                                 device, prop, 0, 1, False,
                                 XA_ATOM, &realtype, &realformat, &nitems,
                                 &bytes_after, &data);
        XCloseDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device);

        if (gdk_error_trap_pop () || rc != Success || realtype == None)
                ret = WACOM_TYPE_INVALID;

        XFree (data);

	return ret;
}

/* Finds an output which matches the given EDID information. Any NULL
 * parameter will be interpreted to match any value. */
static GnomeRROutput *
find_output_by_edid (GnomeRRScreen *rr_screen, const gchar *vendor, const gchar *product, const gchar *serial)
{
	GnomeRROutput **rr_outputs;
	GnomeRROutput *retval = NULL;
	guint i;

	rr_outputs = gnome_rr_screen_list_outputs (rr_screen);

	for (i = 0; rr_outputs[i] != NULL; i++) {
		gchar *o_vendor_s;
		gchar *o_product_s;
		int o_product;
		gchar *o_serial_s;
		int o_serial;
		gboolean match;

		if (!gnome_rr_output_is_connected (rr_outputs[i]))
			continue;

		if (!gnome_rr_output_get_ids_from_edid (rr_outputs[i],
						        &o_vendor_s,
						        &o_product,
						        &o_serial))
			continue;

		o_product_s = g_strdup_printf ("%d", o_product);
		o_serial_s  = g_strdup_printf ("%d", o_serial);

		g_debug ("Checking for match between '%s','%s','%s' and '%s','%s','%s'", \
		         vendor, product, serial, o_vendor_s, o_product_s, o_serial_s);

		match = (vendor  == NULL || g_strcmp0 (vendor,  o_vendor_s)  == 0) && \
		        (product == NULL || g_strcmp0 (product, o_product_s) == 0) && \
		        (serial  == NULL || g_strcmp0 (serial,  o_serial_s)  == 0);

		g_free (o_vendor_s);
		g_free (o_product_s);
		g_free (o_serial_s);

		if (match) {
			retval = rr_outputs[i];
			break;
		}
	}

	if (retval == NULL)
		g_debug ("Did not find a matching output for EDID '%s,%s,%s'",
			 vendor, product, serial);

	return retval;
}

static GnomeRROutput*
find_builtin_output (GnomeRRScreen *rr_screen)
{
	GnomeRROutput **rr_outputs;
	GnomeRROutput *retval = NULL;
	guint i;

	rr_outputs = gnome_rr_screen_list_outputs (rr_screen);
	for (i = 0; rr_outputs[i] != NULL; i++) {
		if (!gnome_rr_output_is_connected (rr_outputs[i]))
			continue;

		if (gnome_rr_output_is_laptop(rr_outputs[i])) {
			retval = rr_outputs[i];
			break;
		}
	}

	if (retval == NULL)
		g_debug ("Did not find a built-in monitor");

	return retval;
}

static GnomeRROutput *
find_output_by_heuristic (GnomeRRScreen *rr_screen, CsdWacomDevice *device)
{
	GnomeRROutput *rr_output;

	/* TODO: This heuristic will fail for non-Wacom display
	 * tablets and may give the wrong result if multiple Wacom
	 * display tablets are connected.
	 */
	rr_output = find_output_by_edid (rr_screen, "WAC", NULL, NULL);

	if (!rr_output)
		rr_output = find_builtin_output (rr_screen);

	return rr_output;
}

static GnomeRROutput *
find_output_by_display (GnomeRRScreen *rr_screen, CsdWacomDevice *device)
{
	gsize n;
	GSettings *tablet;
	GVariant *display;
	const gchar **edid;
	GnomeRROutput *ret;

	if (device == NULL)
		return NULL;

	ret      = NULL;
	tablet   = device->priv->wacom_settings;
	display  = g_settings_get_value (tablet, "display");
	edid     = g_variant_get_strv (display, &n);

	if (n != 3) {
		g_critical ("Expected 'display' key to store %d values; got %"G_GSIZE_FORMAT".", 3, n);
		goto out;
	}

	if (strlen (edid[0]) == 0 || strlen (edid[1]) == 0 || strlen (edid[2]) == 0)
		goto out;

	ret = find_output_by_edid (rr_screen, edid[0], edid[1], edid[2]);

out:
	g_free (edid);
	g_variant_unref (display);

	return ret;
}

static gboolean
is_on (GnomeRROutput *output)
{
	GnomeRRCrtc *crtc;

	crtc = gnome_rr_output_get_crtc (output);
	if (!crtc)
		return FALSE;
	return gnome_rr_crtc_get_current_mode (crtc) != NULL;
}

static GnomeRROutput *
find_output_by_monitor (GnomeRRScreen *rr_screen,
			GdkScreen     *screen,
			int            monitor)
{
	GnomeRROutput **rr_outputs;
	GnomeRROutput *ret;
	guint i;

	ret = NULL;

	rr_outputs = gnome_rr_screen_list_outputs (rr_screen);

	for (i = 0; rr_outputs[i] != NULL; i++) {
		GnomeRROutput *rr_output;
		GnomeRRCrtc *crtc;
		int x, y;

		rr_output = rr_outputs[i];

		if (!is_on (rr_output))
			continue;

		crtc = gnome_rr_output_get_crtc (rr_output);
		if (!crtc)
			continue;

		gnome_rr_crtc_get_position (crtc, &x, &y);

		if (monitor == gdk_screen_get_monitor_at_point (screen, x, y)) {
			ret = rr_output;
			break;
		}
	}

	if (ret == NULL)
		g_warning ("No output found for monitor %d.", monitor);

	return ret;
}

static void
set_display_by_output (CsdWacomDevice  *device,
                       GnomeRROutput   *rr_output)
{
	GSettings   *tablet;
	GVariant    *c_array;
	GVariant    *n_array;
	gsize        nvalues;
	gchar       *o_vendor_s, *o_product_s, *o_serial_s;
	int          o_product, o_serial;
	const gchar *values[3];
	const gchar **unused_variant;

	tablet  = csd_wacom_device_get_settings (device);
	c_array = g_settings_get_value (tablet, "display");
	unused_variant = g_variant_get_strv (c_array, &nvalues);

	/* these arrays aren't used, only nvalues is */
	g_free (unused_variant);
	g_variant_unref (c_array);

	if (nvalues != 3) {
		g_warning ("Unable set set display property. Got %"G_GSIZE_FORMAT" items; expected %d items.\n", nvalues, 4);
		return;
	}

	if (rr_output == NULL ||
	    !gnome_rr_output_get_ids_from_edid (rr_output,
					        &o_vendor_s,
					        &o_product,
					        &o_serial)) {
		o_vendor_s  = g_strdup ("");
		o_product_s = g_strdup ("");
		o_serial_s  = g_strdup ("");
	} else {
		o_product_s = g_strdup_printf ("%d", o_product);
		o_serial_s  = g_strdup_printf ("%d", o_serial);
	}

	values[0] = o_vendor_s;
	values[1] = o_product_s;
	values[2] = o_serial_s;
	n_array = g_variant_new_strv ((const gchar * const *) &values, 3);
	g_settings_set_value (tablet, "display", n_array);

	g_free (o_vendor_s);
	g_free (o_product_s);
	g_free (o_serial_s);
}

static CsdWacomRotation
get_rotation_wacom (GnomeRRRotation rotation)
{
        guint i;

        for (i = 0; i < G_N_ELEMENTS (rotation_table); i++) {
                if (rotation_table[i].rotation & rotation)
                        return (rotation_table[i].rotation_wacom);
        }
        g_assert_not_reached ();
}

void
csd_wacom_device_set_display (CsdWacomDevice *device,
                              int             monitor)
{
	GError *error = NULL;
	GnomeRRScreen *rr_screen;
	GnomeRROutput *output = NULL;

        g_return_if_fail (CSD_IS_WACOM_DEVICE (device));

	rr_screen = gnome_rr_screen_new (gdk_screen_get_default (), &error);
	if (rr_screen == NULL) {
		g_warning ("Failed to create GnomeRRScreen: %s", error->message);
		g_error_free (error);
		return;
	}

	if (monitor > CSD_WACOM_SET_ALL_MONITORS)
		output = find_output_by_monitor (rr_screen, gdk_screen_get_default (), monitor);
	set_display_by_output (device, output);

	g_object_unref (rr_screen);
}

static GnomeRROutput *
find_output (GnomeRRScreen  *rr_screen,
	     CsdWacomDevice *device)
{
	GnomeRROutput *rr_output;
	rr_output = find_output_by_display (rr_screen, device);

	if (rr_output == NULL) {
		if (csd_wacom_device_is_screen_tablet (device)) {
			rr_output = find_output_by_heuristic (rr_screen, device);
			if (rr_output == NULL)
				g_warning ("No fuzzy match based on heuristics was found.");
			else
				g_warning ("Automatically mapping tablet to heuristically-found display.");
		}
	}

	return rr_output;
}

static void
calculate_transformation_matrix (const GdkRectangle mapped, const GdkRectangle desktop, float matrix[NUM_ELEMS_MATRIX])
{
	float x_scale = (float)mapped.x / desktop.width;
	float y_scale = (float)mapped.y / desktop.height;
	float width_scale  = (float)mapped.width / desktop.width;
	float height_scale = (float)mapped.height / desktop.height;

	matrix[0] = width_scale;
	matrix[1] = 0.0f;
	matrix[2] = x_scale;

	matrix[3] = 0.0f;
	matrix[4] = height_scale;
	matrix[5] = y_scale;

	matrix[6] = 0.0f;
	matrix[7] = 0.0f;
	matrix[8] = 1.0f;

	g_debug ("Matrix is %f,%f,%f,%f,%f,%f,%f,%f,%f.",
	         matrix[0], matrix[1], matrix[2],
	         matrix[3], matrix[4], matrix[5],
	         matrix[6], matrix[7], matrix[8]);

	return;
}

int
csd_wacom_device_get_display_monitor (CsdWacomDevice *device)
{
	GError *error = NULL;
	GnomeRRScreen *rr_screen;
	GnomeRROutput *rr_output;
	GnomeRRMode *mode;
	GnomeRRCrtc *crtc;
	gint area[4];

        g_return_val_if_fail (CSD_IS_WACOM_DEVICE (device), CSD_WACOM_SET_ALL_MONITORS);

	rr_screen = gnome_rr_screen_new (gdk_screen_get_default (), &error);
	if (rr_screen == NULL) {
		g_warning ("Failed to create GnomeRRScreen: %s", error->message);
		g_error_free (error);
		return CSD_WACOM_SET_ALL_MONITORS;
	}

	rr_output = find_output (rr_screen, device);
	if (rr_output == NULL) {
		g_object_unref (rr_screen);
		return CSD_WACOM_SET_ALL_MONITORS;
	}

	if (!is_on (rr_output)) {
		g_warning ("Output is not active.");
		g_object_unref (rr_screen);
		return CSD_WACOM_SET_ALL_MONITORS;
	}

	crtc = gnome_rr_output_get_crtc (rr_output);
	gnome_rr_crtc_get_position (crtc, &area[0], &area[1]);

	mode = gnome_rr_crtc_get_current_mode (crtc);
	area[2] = gnome_rr_mode_get_width (mode);
	area[3] = gnome_rr_mode_get_height (mode);

	g_object_unref (rr_screen);

	if (area[2] <= 0 || area[3] <= 0) {
		g_warning ("Output has non-positive area.");
		return CSD_WACOM_SET_ALL_MONITORS;
	}

	g_debug ("Area: %d,%d %dx%d", area[0], area[1], area[2], area[3]);
	return gdk_screen_get_monitor_at_point (gdk_screen_get_default (), area[0], area[1]);
}

gboolean
csd_wacom_device_get_display_matrix (CsdWacomDevice *device, float matrix[NUM_ELEMS_MATRIX])
{
	int monitor;
	GdkRectangle display;
	GdkRectangle desktop;
	GdkScreen *screen = gdk_screen_get_default ();

	matrix[0] = 1.0f;
	matrix[1] = 0.0f;
	matrix[2] = 0.0f;
	matrix[3] = 0.0f;
	matrix[4] = 1.0f;
	matrix[5] = 0.0f;
	matrix[6] = 0.0f;
	matrix[7] = 0.0f;
	matrix[8] = 1.0f;

	monitor = csd_wacom_device_get_display_monitor (device);
	if (monitor < 0)
		return FALSE;

	desktop.x = 0;
	desktop.y = 0;
	desktop.width = gdk_screen_get_width (screen);
	desktop.height = gdk_screen_get_height (screen);

	gdk_screen_get_monitor_geometry (screen, monitor, &display);
	calculate_transformation_matrix (display, desktop, matrix);
	return TRUE;
}

CsdWacomRotation
csd_wacom_device_get_display_rotation (CsdWacomDevice *device)
{
	GError *error = NULL;
	GnomeRRScreen *rr_screen;
	GnomeRROutput *rr_output;
	GnomeRRRotation rotation = GNOME_RR_ROTATION_0;

	rr_screen = gnome_rr_screen_new (gdk_screen_get_default (), &error);
	if (rr_screen == NULL) {
		g_warning ("Failed to create GnomeRRScreen: %s", error->message);
		g_error_free (error);
		return CSD_WACOM_ROTATION_NONE;
	}

	rr_output = find_output (rr_screen, device);
	if (rr_output) {
		GnomeRRCrtc *crtc = gnome_rr_output_get_crtc (rr_output);
		if (crtc)
			rotation = gnome_rr_crtc_get_current_rotation (crtc);
	}
	g_object_unref (rr_screen);

	return get_rotation_wacom (rotation);
}

static void
add_stylus_to_device (CsdWacomDevice *device,
		      const char     *settings_path,
		      int             id)
{
	const WacomStylus *wstylus;

	wstylus = libwacom_stylus_get_for_id (db, id);
	if (wstylus) {
		CsdWacomStylus *stylus;
		char *stylus_settings_path;
		GSettings *settings;

		if (device->priv->type == WACOM_TYPE_STYLUS &&
		    libwacom_stylus_is_eraser (wstylus))
			return;
		if (device->priv->type == WACOM_TYPE_ERASER &&
		    libwacom_stylus_is_eraser (wstylus) == FALSE)
			return;

		stylus_settings_path = g_strdup_printf ("%s0x%x/", settings_path, id);
		if (device->priv->type == WACOM_TYPE_STYLUS) {
			settings = g_settings_new_with_path (WACOM_STYLUS_SCHEMA, stylus_settings_path);
			stylus = csd_wacom_stylus_new (device, wstylus, settings);
		} else {
			settings = g_settings_new_with_path (WACOM_ERASER_SCHEMA, stylus_settings_path);
			stylus = csd_wacom_stylus_new (device, wstylus, settings);
		}
		g_free (stylus_settings_path);
		device->priv->styli = g_list_prepend (device->priv->styli, stylus);
	}
}

int
csd_wacom_device_get_num_modes (CsdWacomDevice *device,
				int             group_id)
{
	int num_modes;

	g_return_val_if_fail (CSD_IS_WACOM_DEVICE (device), -1);
	num_modes = GPOINTER_TO_INT (g_hash_table_lookup (device->priv->num_modes, GINT_TO_POINTER(group_id)));

	return num_modes;
}

int
csd_wacom_device_get_current_mode (CsdWacomDevice *device,
				   int             group_id)
{
	int current_idx;

	g_return_val_if_fail (CSD_IS_WACOM_DEVICE (device), -1);
	current_idx = GPOINTER_TO_INT (g_hash_table_lookup (device->priv->modes, GINT_TO_POINTER(group_id)));
	/* That means that the mode doesn't exist, see csd_wacom_device_add_modes() */
	g_return_val_if_fail (current_idx != 0, -1);

	return current_idx;
}

int
csd_wacom_device_set_next_mode (CsdWacomDevice       *device,
				CsdWacomTabletButton *button)
{
	GList *l;
	int current_idx;
	int num_modes;
	int num_switches;
	int group_id;

	g_return_val_if_fail (CSD_IS_WACOM_DEVICE (device), -1);

	group_id = button->group_id;
	current_idx = 0;
	num_switches = 0;
	num_modes = GPOINTER_TO_INT (g_hash_table_lookup (device->priv->num_modes, GINT_TO_POINTER(group_id)));

	/*
	 * Check if we have multiple mode-switch buttons for that
	 * group, and if so, compute the current index based on
	 * the position in the list...
	 */
	for (l = device->priv->buttons; l != NULL; l = l->next) {
		CsdWacomTabletButton *b = l->data;
		if (b->type != WACOM_TABLET_BUTTON_TYPE_HARDCODED)
			continue;
		if (button->group_id == b->group_id)
			num_switches++;
		if (g_strcmp0 (button->id, b->id) == 0)
			current_idx = num_switches;
	}

	/* We should at least have found the current mode-switch button...
	 * If not, then it means that the given button is not a valid
	 * mode-switch.
	 */
	g_return_val_if_fail (num_switches != 0, -1);

	/* Only one mode-switch? cycle through the modes */
	if (num_switches == 1) {
		current_idx = csd_wacom_device_get_current_mode (device, group_id);
		/* csd_wacom_device_get_current_mode() returns -1 when the mode doesn't exist */
		g_return_val_if_fail (current_idx > 0, -1);

		current_idx++;
	}

	if (current_idx > num_modes)
		current_idx = 1;

	g_hash_table_insert (device->priv->modes, GINT_TO_POINTER (group_id), GINT_TO_POINTER (current_idx));

	return current_idx;
}

static int
flags_to_group (WacomButtonFlags flags)
{
	if (flags & WACOM_BUTTON_RING_MODESWITCH)
		return 1;
	if (flags & WACOM_BUTTON_RING2_MODESWITCH)
		return 2;
	if (flags & WACOM_BUTTON_TOUCHSTRIP_MODESWITCH)
		return 3;
	if (flags & WACOM_BUTTON_TOUCHSTRIP2_MODESWITCH)
		return 4;

	return 0;
}

static GList *
csd_wacom_device_add_ring_modes (WacomDevice      *wacom_device,
				 const char       *settings_path,
				 WacomButtonFlags  direction)
{
	GList *l;
	guint num_modes;
	guint group;
	guint i;
	char *name, *id;

	l = NULL;

	if ((direction & WACOM_BUTTON_POSITION_LEFT) && libwacom_has_ring (wacom_device)) {
		num_modes = libwacom_get_ring_num_modes (wacom_device);
		group = flags_to_group (WACOM_BUTTON_RING_MODESWITCH);
		if (num_modes == 0) {
			/* If no mode is available, we use "left-ring-mode-1" for backward compat */
			l = g_list_append (l, csd_wacom_tablet_button_new (_("Left Ring"),
									   "left-ring-mode-1",
									   settings_path,
									   WACOM_TABLET_BUTTON_TYPE_RING,
									   WACOM_TABLET_BUTTON_POS_LEFT,
									   group,
									   0,
									   CSD_WACOM_NO_LED));
		} else {
			for (i = 1; i <= num_modes; i++) {
				name = g_strdup_printf (_("Left Ring Mode #%d"), i);
				id = g_strdup_printf ("left-ring-mode-%d", i);
				l = g_list_append (l, csd_wacom_tablet_button_new (name,
				                                                   id,
				                                                   settings_path,
				                                                   WACOM_TABLET_BUTTON_TYPE_RING,
										   WACOM_TABLET_BUTTON_POS_LEFT,
				                                                   group,
				                                                   i - 1,
										   CSD_WACOM_NO_LED));
				g_free (name);
				g_free (id);
			}
		}
	} else if ((direction & WACOM_BUTTON_POSITION_RIGHT) && libwacom_has_ring2 (wacom_device)) {
		num_modes = libwacom_get_ring2_num_modes (wacom_device);
		group = flags_to_group (WACOM_BUTTON_RING2_MODESWITCH);
		if (num_modes == 0) {
			/* If no mode is available, we use "right-ring-mode-1" for backward compat */
			l = g_list_append (l, csd_wacom_tablet_button_new (_("Right Ring"),
									   "right-ring-mode-1",
									   settings_path,
									   WACOM_TABLET_BUTTON_TYPE_RING,
									   WACOM_TABLET_BUTTON_POS_RIGHT,
									   group,
									   0,
									   CSD_WACOM_NO_LED));
		} else {
			for (i = 1; i <= num_modes; i++) {
				name = g_strdup_printf (_("Right Ring Mode #%d"), i);
				id = g_strdup_printf ("right-ring-mode-%d", i);
				l = g_list_append (l, csd_wacom_tablet_button_new (name,
				                                                   id,
				                                                   settings_path,
				                                                   WACOM_TABLET_BUTTON_TYPE_RING,
										   WACOM_TABLET_BUTTON_POS_RIGHT,
				                                                   group,
				                                                   i - 1,
										   CSD_WACOM_NO_LED));
				g_free (name);
				g_free (id);
			}
		}
	}

	return l;
}

static GList *
csd_wacom_device_add_strip_modes (WacomDevice      *wacom_device,
				  const char       *settings_path,
				  WacomButtonFlags  direction)
{
	GList *l;
	guint num_modes;
	guint num_strips;
	guint group;
	guint i;
	char *name, *id;

	l = NULL;
	num_strips = libwacom_get_num_strips (wacom_device);
	if (num_strips > 2)
		g_warning ("Unhandled number of touchstrips: %d", num_strips);

	if ((direction & WACOM_BUTTON_POSITION_LEFT) && num_strips >= 1) {
		num_modes = libwacom_get_strips_num_modes (wacom_device);
		group = flags_to_group (WACOM_BUTTON_TOUCHSTRIP_MODESWITCH);
		if (num_modes == 0) {
			/* If no mode is available, we use "left-strip-mode-1" for backward compat */
			l = g_list_append (l, csd_wacom_tablet_button_new (_("Left Touchstrip"),
									   "left-strip-mode-1",
									   settings_path,
									   WACOM_TABLET_BUTTON_TYPE_STRIP,
									   WACOM_TABLET_BUTTON_POS_LEFT,
									   group,
									   0,
									   CSD_WACOM_NO_LED));
		} else {
			for (i = 1; i <= num_modes; i++) {
				name = g_strdup_printf (_("Left Touchstrip Mode #%d"), i);
				id = g_strdup_printf ("left-strip-mode-%d", i);
				l = g_list_append (l, csd_wacom_tablet_button_new (name,
				                                                   id,
				                                                   settings_path,
				                                                   WACOM_TABLET_BUTTON_TYPE_STRIP,
										   WACOM_TABLET_BUTTON_POS_LEFT,
				                                                   group,
				                                                   i - 1,
										   CSD_WACOM_NO_LED));
				g_free (name);
				g_free (id);
			}
		}
	} else if ((direction & WACOM_BUTTON_POSITION_RIGHT) && num_strips >= 2) {
		num_modes = libwacom_get_strips_num_modes (wacom_device);
		group = flags_to_group (WACOM_BUTTON_TOUCHSTRIP2_MODESWITCH);
		if (num_modes == 0) {
			/* If no mode is available, we use "right-strip-mode-1" for backward compat */
			l = g_list_append (l, csd_wacom_tablet_button_new (_("Right Touchstrip"),
									   "right-strip-mode-1",
									   settings_path,
									   WACOM_TABLET_BUTTON_TYPE_STRIP,
									   WACOM_TABLET_BUTTON_POS_RIGHT,
									   group,
									   0,
									   CSD_WACOM_NO_LED));
		} else {
			for (i = 1; i <= num_modes; i++) {
				name = g_strdup_printf (_("Right Touchstrip Mode #%d"), i);
				id = g_strdup_printf ("right-strip-mode-%d", i);
				l = g_list_append (l, csd_wacom_tablet_button_new (name,
				                                                   id,
				                                                   settings_path,
				                                                   WACOM_TABLET_BUTTON_TYPE_STRIP,
										   WACOM_TABLET_BUTTON_POS_RIGHT,
				                                                   group,
				                                                   i - 1,
										   CSD_WACOM_NO_LED));
				g_free (name);
				g_free (id);
			}
		}
	}

	return l;
}

static char *
csd_wacom_device_modeswitch_name (WacomButtonFlags flags,
				  guint button_num)
{
	if (flags & WACOM_BUTTON_RINGS_MODESWITCH) {
		if (flags & WACOM_BUTTON_POSITION_LEFT)
			return g_strdup_printf (_("Left Touchring Mode Switch"));
		else
			return g_strdup_printf (_("Right Touchring Mode Switch"));
	} else if (flags & WACOM_BUTTON_TOUCHSTRIPS_MODESWITCH) {
		if (flags & WACOM_BUTTON_POSITION_LEFT)
			return g_strdup_printf (_("Left Touchstrip Mode Switch"));
		else
			return g_strdup_printf (_("Right Touchstrip Mode Switch"));
	}

	g_warning ("Unhandled modeswitch and direction combination");

	return g_strdup_printf (_("Mode Switch #%d"), button_num);
}

static CsdWacomTabletButtonType
csd_wacom_device_button_pos (WacomButtonFlags flags)
{
	if (flags & WACOM_BUTTON_POSITION_LEFT)
		return WACOM_TABLET_BUTTON_POS_LEFT;
	else if (flags & WACOM_BUTTON_POSITION_RIGHT)
		return WACOM_TABLET_BUTTON_POS_RIGHT;
	else if (flags & WACOM_BUTTON_POSITION_TOP)
		return WACOM_TABLET_BUTTON_POS_TOP;
	else if (flags & WACOM_BUTTON_POSITION_BOTTOM)
		return WACOM_TABLET_BUTTON_POS_BOTTOM;

	g_warning ("Unhandled button position");

	return WACOM_TABLET_BUTTON_POS_UNDEF;
}

static GList *
csd_wacom_device_add_buttons_dir (WacomDevice      *wacom_device,
				  const char       *settings_path,
				  WacomButtonFlags  direction,
				  const char       *button_str,
				  const char       *button_str_id)
{
	GList *l;
	guint num_buttons, i, button_num;
	char *name, *id;

	l = NULL;
	button_num = 1;
	num_buttons = libwacom_get_num_buttons (wacom_device);
	for (i = 'A'; i < 'A' + num_buttons; i++) {
		WacomButtonFlags flags;

		flags = libwacom_get_button_flag (wacom_device, i);
		if (!(flags & direction))
			continue;
		/* Ignore mode switches */
		if (flags & WACOM_BUTTON_MODESWITCH)
			continue;

		name = g_strdup_printf (button_str, button_num++);
		id = g_strdup_printf ("%s%c", button_str_id, i);
		l = g_list_append (l, csd_wacom_tablet_button_new (name,
		                                                   id,
		                                                   settings_path,
		                                                   WACOM_TABLET_BUTTON_TYPE_NORMAL,
		                                                   csd_wacom_device_button_pos (flags),
		                                                   flags_to_group (flags),
		                                                   -1,
		                                                   CSD_WACOM_NO_LED));
		g_free (name);
		g_free (id);
	}

	/* Handle modeswitches */
	for (i = 'A'; i < 'A' + num_buttons; i++) {
		WacomButtonFlags flags;
		char *name, *id;
		int status_led;

		flags = libwacom_get_button_flag (wacom_device, i);
		if (!(flags & direction))
			continue;
		/* Ignore non-mode switches */
		if (!(flags & WACOM_BUTTON_MODESWITCH))
			continue;

		name = csd_wacom_device_modeswitch_name (flags, button_num++);
		id = g_strdup_printf ("%s%c", button_str_id, i);
		status_led = libwacom_get_button_led_group (wacom_device, i);
		l = g_list_append (l, csd_wacom_tablet_button_new (name,
		                                                   id,
		                                                   settings_path,
		                                                   WACOM_TABLET_BUTTON_TYPE_HARDCODED,
		                                                   csd_wacom_device_button_pos (flags),
		                                                   flags_to_group (flags),
		                                                   -1,
		                                                   status_led));
		g_free (name);
		g_free (id);
	}

	/* Handle touch{strips,rings} */
	if (libwacom_has_ring2 (wacom_device) || libwacom_has_ring (wacom_device))
		l = g_list_concat (l, csd_wacom_device_add_ring_modes (wacom_device, settings_path, direction));
	if  (libwacom_get_num_strips (wacom_device) > 0)
		l = g_list_concat (l, csd_wacom_device_add_strip_modes (wacom_device, settings_path, direction));

	return l;
}

static void
csd_wacom_device_add_buttons (CsdWacomDevice *device,
			      WacomDevice    *wacom_device,
			      const char     *settings_path)
{
	GList *l, *ret;

	ret = NULL;

	l = csd_wacom_device_add_buttons_dir (wacom_device, settings_path, WACOM_BUTTON_POSITION_LEFT, _("Left Button #%d"), "button");
	if (l)
		ret = l;
	l = csd_wacom_device_add_buttons_dir (wacom_device, settings_path, WACOM_BUTTON_POSITION_RIGHT, _("Right Button #%d"), "button");
	if (l)
		ret = g_list_concat (ret, l);
	l = csd_wacom_device_add_buttons_dir (wacom_device, settings_path, WACOM_BUTTON_POSITION_TOP, _("Top Button #%d"), "button");
	if (l)
		ret = g_list_concat (ret, l);
	l = csd_wacom_device_add_buttons_dir (wacom_device, settings_path, WACOM_BUTTON_POSITION_BOTTOM, _("Bottom Button #%d"), "button");
	if (l)
		ret = g_list_concat (ret, l);

	device->priv->buttons = ret;
}

static void
csd_wacom_device_get_modeswitches (WacomDevice      *wacom_device,
				   gint             *num_rings,
				   gint             *num_strips)
{
	*num_strips = libwacom_get_num_strips (wacom_device);

	if (libwacom_has_ring2 (wacom_device))
		*num_rings = 2;
	else if  (libwacom_has_ring (wacom_device))
		*num_rings = 1;
	else
		*num_rings = 0;
}

static void
csd_wacom_device_add_modes (CsdWacomDevice *device,
			    WacomDevice    *wacom_device)
{
	GList *l;

	device->priv->modes = g_hash_table_new (g_direct_hash, g_direct_equal);
	device->priv->num_modes = g_hash_table_new (g_direct_hash, g_direct_equal);

	for (l = device->priv->buttons; l != NULL; l = l->next) {
		CsdWacomTabletButton *button = l->data;

		if (button->group_id > 0)
			g_hash_table_insert (device->priv->modes, GINT_TO_POINTER (button->group_id), GINT_TO_POINTER (1));

		/* See flags_to_group() for group ID/button type matches */
		if (button->group_id == 1) {
			g_hash_table_insert (device->priv->num_modes,
					     GINT_TO_POINTER (button->group_id),
					     GINT_TO_POINTER (libwacom_get_ring_num_modes (wacom_device)));
		} else if (button->group_id == 2) {
			g_hash_table_insert (device->priv->num_modes,
					     GINT_TO_POINTER (button->group_id),
					     GINT_TO_POINTER (libwacom_get_ring2_num_modes (wacom_device)));
		} else if (button->group_id == 3 || button->group_id == 4) {
			g_hash_table_insert (device->priv->num_modes,
					     GINT_TO_POINTER (button->group_id),
					     GINT_TO_POINTER (libwacom_get_strips_num_modes (wacom_device)));
		}
	}
}

static void
csd_wacom_device_update_from_db (CsdWacomDevice *device,
				 WacomDevice    *wacom_device,
				 const char     *identifier)
{
	char *settings_path;
	WacomIntegrationFlags integration_flags;

	settings_path = g_strdup_printf (WACOM_DEVICE_CONFIG_BASE,
					 device->priv->machine_id,
					 libwacom_get_match (wacom_device));
	device->priv->wacom_settings = g_settings_new_with_path (WACOM_TABLET_SCHEMA,
								 settings_path);

	device->priv->name = g_strdup (libwacom_get_name (wacom_device));
	device->priv->layout_path = g_strdup (libwacom_get_layout_filename (wacom_device));
	device->priv->reversible = libwacom_is_reversible (wacom_device);
	integration_flags = libwacom_get_integration_flags (wacom_device);
	device->priv->is_screen_tablet = (integration_flags & WACOM_DEVICE_INTEGRATED_DISPLAY);
	device->priv->is_isd = (integration_flags & WACOM_DEVICE_INTEGRATED_SYSTEM);
	if (device->priv->is_screen_tablet) {
		if (!device->priv->is_isd)
			device->priv->icon_name = "wacom-tablet-cintiq";
		else
			device->priv->icon_name = "wacom-tablet-pc";
	} else {
		device->priv->icon_name = "wacom-tablet";
	}

	if (device->priv->type == WACOM_TYPE_PAD) {
		csd_wacom_device_get_modeswitches (wacom_device,
						   &device->priv->num_rings,
						   &device->priv->num_strips);
		csd_wacom_device_add_buttons (device, wacom_device, settings_path);
		csd_wacom_device_add_modes (device, wacom_device);
	}

	if (device->priv->type == WACOM_TYPE_STYLUS ||
	    device->priv->type == WACOM_TYPE_ERASER) {
		const int *ids;
		int num_styli;
		guint i;

		ids = libwacom_get_supported_styli (wacom_device, &num_styli);
		g_assert (num_styli >= 1);
		for (i = 0; i < num_styli; i++)
			add_stylus_to_device (device, settings_path, ids[i]);
		device->priv->styli = g_list_reverse (device->priv->styli);
	}
	g_free (settings_path);
}

static GObject *
csd_wacom_device_constructor (GType                     type,
                              guint                      n_construct_properties,
                              GObjectConstructParam     *construct_properties)
{
        CsdWacomDevice *device;
        GdkDeviceManager *device_manager;
        XDeviceInfo *device_info;
        WacomDevice *wacom_device;
        int n_devices;
        guint i;

        device = CSD_WACOM_DEVICE (G_OBJECT_CLASS (csd_wacom_device_parent_class)->constructor (type,
												n_construct_properties,
												construct_properties));

	if (device->priv->gdk_device == NULL)
		return G_OBJECT (device);

	device_manager = gdk_display_get_device_manager (gdk_display_get_default ());
	g_object_get (device_manager, "opcode", &device->priv->opcode, NULL);

        g_object_get (device->priv->gdk_device, "device-id", &device->priv->device_id, NULL);

        device_info = XListInputDevices (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), &n_devices);
        if (device_info == NULL) {
		g_warning ("Could not list any input devices through XListInputDevices()");
		goto end;
	}

        for (i = 0; i < n_devices; i++) {
		if (device_info[i].id == device->priv->device_id) {
			device->priv->type = get_device_type (&device_info[i]);
			device->priv->tool_name = g_strdup (device_info[i].name);
			break;
		}
	}

	XFreeDeviceList (device_info);

	if (device->priv->type == WACOM_TYPE_INVALID)
		goto end;

	device->priv->path = xdevice_get_device_node (device->priv->device_id);
	if (device->priv->path == NULL) {
		g_warning ("Could not get the device node path for ID '%d'", device->priv->device_id);
		device->priv->type = WACOM_TYPE_INVALID;
		goto end;
	}

	if (db == NULL)
		db = libwacom_database_new ();

	wacom_device = libwacom_new_from_path (db, device->priv->path, FALSE, NULL);
	if (!wacom_device) {
		WacomError *wacom_error;

		g_debug ("Creating fallback driver for wacom tablet '%s' ('%s')",
			 gdk_device_get_name (device->priv->gdk_device),
			 device->priv->path);

		device->priv->is_fallback = TRUE;
		wacom_error = libwacom_error_new ();
		wacom_device = libwacom_new_from_path (db, device->priv->path, TRUE, wacom_error);
		if (wacom_device == NULL) {
			g_warning ("Failed to create fallback wacom device for '%s': %s (%d)",
				   device->priv->path,
				   libwacom_error_get_message (wacom_error),
				   libwacom_error_get_code (wacom_error));
			libwacom_error_free (&wacom_error);
			device->priv->type = WACOM_TYPE_INVALID;
			goto end;
		}
	}

	csd_wacom_device_update_from_db (device, wacom_device, device->priv->path);
	libwacom_destroy (wacom_device);

	if (device->priv->type == WACOM_TYPE_STYLUS ||
	    device->priv->type == WACOM_TYPE_ERASER) {
		setup_property_notify (device);
	}

end:
        return G_OBJECT (device);
}

static void
csd_wacom_device_set_property (GObject        *object,
                               guint           prop_id,
                               const GValue   *value,
                               GParamSpec     *pspec)
{
        CsdWacomDevice *device;

        device = CSD_WACOM_DEVICE (object);

        switch (prop_id) {
	case PROP_GDK_DEVICE:
		device->priv->gdk_device = g_value_get_pointer (value);
		break;
	case PROP_LAST_STYLUS:
		device->priv->last_stylus = g_value_get_pointer (value);
		break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
csd_wacom_device_get_property (GObject        *object,
                               guint           prop_id,
                               GValue         *value,
                               GParamSpec     *pspec)
{
        CsdWacomDevice *device;

        device = CSD_WACOM_DEVICE (object);

        switch (prop_id) {
	case PROP_GDK_DEVICE:
		g_value_set_pointer (value, device->priv->gdk_device);
		break;
	case PROP_LAST_STYLUS:
		g_value_set_pointer (value, device->priv->last_stylus);
		break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
csd_wacom_device_class_init (CsdWacomDeviceClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->constructor = csd_wacom_device_constructor;
        object_class->finalize = csd_wacom_device_finalize;
        object_class->set_property = csd_wacom_device_set_property;
        object_class->get_property = csd_wacom_device_get_property;

        g_type_class_add_private (klass, sizeof (CsdWacomDevicePrivate));

	g_object_class_install_property (object_class, PROP_GDK_DEVICE,
					 g_param_spec_pointer ("gdk-device", "gdk-device", "gdk-device",
							       G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property (object_class, PROP_LAST_STYLUS,
					 g_param_spec_pointer ("last-stylus", "last-stylus", "last-stylus",
							       G_PARAM_READWRITE));
}

static void
csd_wacom_device_init (CsdWacomDevice *device)
{
        device->priv = CSD_WACOM_DEVICE_GET_PRIVATE (device);
        device->priv->type = WACOM_TYPE_INVALID;

        if (g_file_get_contents ("/etc/machine-id", &device->priv->machine_id, NULL, NULL) == FALSE)
                if (g_file_get_contents ("/var/lib/dbus/machine-id", &device->priv->machine_id, NULL, NULL) == FALSE)
                        device->priv->machine_id = g_strdup ("00000000000000000000000000000000");

        device->priv->machine_id = g_strstrip (device->priv->machine_id);
}

static void
csd_wacom_device_finalize (GObject *object)
{
        CsdWacomDevice *device;
        CsdWacomDevicePrivate *p;

        g_return_if_fail (object != NULL);
        g_return_if_fail (CSD_IS_WACOM_DEVICE (object));

        device = CSD_WACOM_DEVICE (object);

        g_return_if_fail (device->priv != NULL);

	p = device->priv;

        if (p->wacom_settings != NULL) {
                g_object_unref (p->wacom_settings);
                p->wacom_settings = NULL;
        }

        g_list_foreach (p->styli, (GFunc) g_object_unref, NULL);
        g_list_free (p->styli);

        g_list_foreach (p->buttons, (GFunc) csd_wacom_tablet_button_free, NULL);
        g_list_free (p->buttons);

        g_free (p->name);
        p->name = NULL;

        g_free (p->tool_name);
        p->tool_name = NULL;

        g_free (p->path);
        p->path = NULL;

        g_free (p->machine_id);
        p->machine_id = NULL;

        if (p->modes) {
                g_hash_table_destroy (p->modes);
                p->modes = NULL;
        }
        if (p->num_modes) {
                g_hash_table_destroy (p->num_modes);
                p->num_modes = NULL;
        }

	g_clear_pointer (&p->layout_path, g_free);

	gdk_window_remove_filter (NULL,
				  (GdkFilterFunc) filter_events,
				  device);

        G_OBJECT_CLASS (csd_wacom_device_parent_class)->finalize (object);
}

CsdWacomDevice *
csd_wacom_device_new (GdkDevice *device)
{
	return CSD_WACOM_DEVICE (g_object_new (CSD_TYPE_WACOM_DEVICE,
					       "gdk-device", device,
					       NULL));
}

GList *
csd_wacom_device_list_styli (CsdWacomDevice *device)
{
	g_return_val_if_fail (CSD_IS_WACOM_DEVICE (device), NULL);

	return g_list_copy (device->priv->styli);
}

CsdWacomStylus *
csd_wacom_device_get_stylus_for_type (CsdWacomDevice     *device,
				      CsdWacomStylusType  type)
{
	GList *l;

	g_return_val_if_fail (CSD_IS_WACOM_DEVICE (device), NULL);

	for (l = device->priv->styli; l != NULL; l = l->next) {
		CsdWacomStylus *stylus = l->data;

		if (csd_wacom_stylus_get_stylus_type (stylus) == type)
			return stylus;
	}
	return NULL;
}

const char *
csd_wacom_device_get_name (CsdWacomDevice *device)
{
	g_return_val_if_fail (CSD_IS_WACOM_DEVICE (device), NULL);

	return device->priv->name;
}

const char *
csd_wacom_device_get_layout_path (CsdWacomDevice *device)
{
	g_return_val_if_fail (CSD_IS_WACOM_DEVICE (device), NULL);

	return device->priv->layout_path;
}

const char *
csd_wacom_device_get_path (CsdWacomDevice *device)
{
	g_return_val_if_fail (CSD_IS_WACOM_DEVICE (device), NULL);

	return device->priv->path;
}

const char *
csd_wacom_device_get_icon_name (CsdWacomDevice *device)
{
	g_return_val_if_fail (CSD_IS_WACOM_DEVICE (device), NULL);

	return device->priv->icon_name;
}

const char *
csd_wacom_device_get_tool_name (CsdWacomDevice *device)
{
	g_return_val_if_fail (CSD_IS_WACOM_DEVICE (device), NULL);

	return device->priv->tool_name;
}

gboolean
csd_wacom_device_reversible (CsdWacomDevice *device)
{
	g_return_val_if_fail (CSD_IS_WACOM_DEVICE (device), FALSE);

	return device->priv->reversible;
}

gboolean
csd_wacom_device_is_screen_tablet (CsdWacomDevice *device)
{
	g_return_val_if_fail (CSD_IS_WACOM_DEVICE (device), FALSE);

	return device->priv->is_screen_tablet;
}

gboolean
csd_wacom_device_is_isd (CsdWacomDevice *device)
{
	g_return_val_if_fail (CSD_IS_WACOM_DEVICE (device), FALSE);

	return device->priv->is_isd;
}

gboolean
csd_wacom_device_is_fallback (CsdWacomDevice *device)
{
	g_return_val_if_fail (CSD_IS_WACOM_DEVICE (device), FALSE);

	return device->priv->is_fallback;
}

gint
csd_wacom_device_get_num_strips (CsdWacomDevice *device)
{
	g_return_val_if_fail (CSD_IS_WACOM_DEVICE (device), 0);

	return device->priv->num_strips;
}

gint
csd_wacom_device_get_num_rings (CsdWacomDevice *device)
{
	g_return_val_if_fail (CSD_IS_WACOM_DEVICE (device), 0);

	return device->priv->num_rings;
}

GSettings *
csd_wacom_device_get_settings (CsdWacomDevice *device)
{
	g_return_val_if_fail (CSD_IS_WACOM_DEVICE (device), NULL);

	return device->priv->wacom_settings;
}

void
csd_wacom_device_set_current_stylus (CsdWacomDevice *device,
				     int             stylus_id)
{
	GList *l;
	CsdWacomStylus *stylus;

	g_return_if_fail (CSD_IS_WACOM_DEVICE (device));

	/* Don't change anything if the stylus is already set */
	if (device->priv->last_stylus != NULL) {
		CsdWacomStylus *stylus = device->priv->last_stylus;
		if (stylus->priv->id == stylus_id)
			return;
	}

	for (l = device->priv->styli; l; l = l->next) {
		stylus = l->data;

		/* Set a nice default if 0x0 */
		if (stylus_id == 0x0 &&
		    stylus->priv->type == WSTYLUS_GENERAL) {
			g_object_set (device, "last-stylus", stylus, NULL);
			return;
		}

		if (stylus->priv->id == stylus_id) {
			g_object_set (device, "last-stylus", stylus, NULL);
			return;
		}
	}

	/* Setting the default stylus to be the generic one */
	for (l = device->priv->styli; l; l = l->next) {
		stylus = l->data;

		/* Set a nice default if 0x0 */
		if (stylus->priv->type == WSTYLUS_GENERAL) {
			g_debug ("Could not find stylus ID 0x%x for tablet '%s', setting general pen ID 0x%x instead",
				 stylus_id, device->priv->name, stylus->priv->id);
			g_object_set (device, "last-stylus", stylus, NULL);
			return;
		}
	}

	g_warning ("Could not set the current stylus ID 0x%x for tablet '%s', no general pen found",
		   stylus_id, device->priv->name);

	/* Setting the default stylus to be the first one */
	g_assert (device->priv->styli);

	stylus = device->priv->styli->data;
	g_object_set (device, "last-stylus", stylus, NULL);
}

CsdWacomDeviceType
csd_wacom_device_get_device_type (CsdWacomDevice *device)
{
	g_return_val_if_fail (CSD_IS_WACOM_DEVICE (device), WACOM_TYPE_INVALID);

	return device->priv->type;
}

gint *
csd_wacom_device_get_area (CsdWacomDevice *device)
{
	int i, id;
	XDevice *xdevice;
	Atom area, realtype;
	int rc, realformat;
	unsigned long nitems, bytes_after;
	unsigned char *data = NULL;
	gint *device_area;

	g_return_val_if_fail (CSD_IS_WACOM_DEVICE (device), NULL);

	g_object_get (device->priv->gdk_device, "device-id", &id, NULL);

	area = XInternAtom (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), "Wacom Tablet Area", False);

	gdk_error_trap_push ();
	xdevice = XOpenDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), id);
	if (gdk_error_trap_pop () || (device == NULL))
		return NULL;

	gdk_error_trap_push ();
	rc = XGetDeviceProperty (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()),
				 xdevice, area, 0, 4, False,
				 XA_INTEGER, &realtype, &realformat, &nitems,
				 &bytes_after, &data);
	if (gdk_error_trap_pop () || rc != Success || realtype == None || bytes_after != 0 || nitems != 4) {
		XCloseDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), xdevice);
		return NULL;
	}

	device_area = g_new0 (int, nitems);
	for (i = 0; i < nitems; i++)
		device_area[i] = ((long *)data)[i];

	XFree (data);
	XCloseDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), xdevice);

	return device_area;
}

const char *
csd_wacom_device_type_to_string (CsdWacomDeviceType type)
{
	switch (type) {
	case WACOM_TYPE_INVALID:
		return "Invalid";
	case WACOM_TYPE_STYLUS:
		return "Stylus";
	case WACOM_TYPE_ERASER:
		return "Eraser";
	case WACOM_TYPE_CURSOR:
		return "Cursor";
	case WACOM_TYPE_PAD:
		return "Pad";
	case WACOM_TYPE_TOUCH:
		return "Touch";
	default:
		return "Unknown type";
	}
}

GList *
csd_wacom_device_get_buttons (CsdWacomDevice *device)
{
	g_return_val_if_fail (CSD_IS_WACOM_DEVICE (device), NULL);

	return g_list_copy (device->priv->buttons);
}

static CsdWacomTabletButton *
find_button_with_id (CsdWacomDevice *device,
		     const char     *id)
{
	GList *l;

	for (l = device->priv->buttons; l != NULL; l = l->next) {
		CsdWacomTabletButton *button = l->data;

		if (g_strcmp0 (button->id, id) == 0)
			return button;
	}
	return NULL;
}

static CsdWacomTabletButton *
find_button_with_index (CsdWacomDevice *device,
			const char     *id,
			int             index)
{
	CsdWacomTabletButton *button;
	char *str;

	str = g_strdup_printf ("%s-mode-%d", id, index);
	button = find_button_with_id (device, str);
	g_free (str);

	return button;
}

CsdWacomTabletButton *
csd_wacom_device_get_button (CsdWacomDevice   *device,
			     int               button,
			     GtkDirectionType *dir)
{
	int index;

	if (button <= 26) {
		char *id;
		CsdWacomTabletButton *ret;
		int physical_button;

		/* mouse_button = physical_button < 4 ? physical_button : physical_button + 4 */
		if (button > 4)
			physical_button = button - 4;
		else
			physical_button = button;

		id = g_strdup_printf ("button%c", 'A' + physical_button - 1);
		ret = find_button_with_id (device, id);
		g_free (id);

		return ret;
	}

	switch (button) {
	case 90:
	case 92:
	case 94:
	case 96:
		*dir = GTK_DIR_UP;
		break;
	case 91:
	case 93:
	case 95:
	case 97:
		*dir = GTK_DIR_DOWN;
		break;
	default:
		;;
	}

	/* The group ID is implied by the button number */
	switch (button) {
	case 90:
	case 91:
		index = GPOINTER_TO_INT (g_hash_table_lookup (device->priv->modes, GINT_TO_POINTER (1)));
		return find_button_with_index (device, "left-ring", index);
	case 92:
	case 93:
		index = GPOINTER_TO_INT (g_hash_table_lookup (device->priv->modes, GINT_TO_POINTER (2)));
		return find_button_with_index (device, "right-ring", index);
	case 94:
	case 95:
		index = GPOINTER_TO_INT (g_hash_table_lookup (device->priv->modes, GINT_TO_POINTER (3)));
		return find_button_with_index (device, "left-strip", index);
	case 96:
	case 97:
		index = GPOINTER_TO_INT (g_hash_table_lookup (device->priv->modes, GINT_TO_POINTER (4)));
		return find_button_with_index (device, "right-strip", index);
	default:
		return NULL;
	}
}

CsdWacomRotation
csd_wacom_device_rotation_name_to_type (const char *rotation)
{
        guint i;

	g_return_val_if_fail (rotation != NULL, CSD_WACOM_ROTATION_NONE);

        for (i = 0; i < G_N_ELEMENTS (rotation_table); i++) {
                if (strcmp (rotation_table[i].rotation_string, rotation) == 0)
                        return (rotation_table[i].rotation_wacom);
        }

	return CSD_WACOM_ROTATION_NONE;
}

const char *
csd_wacom_device_rotation_type_to_name (CsdWacomRotation type)
{
        guint i;

        for (i = 0; i < G_N_ELEMENTS (rotation_table); i++) {
                if (rotation_table[i].rotation_wacom == type)
                        return (rotation_table[i].rotation_string);
        }

	return "none";
}

CsdWacomDevice *
csd_wacom_device_create_fake (CsdWacomDeviceType  type,
			      const char         *name,
			      const char         *tool_name)
{
	CsdWacomDevice *device;
	CsdWacomDevicePrivate *priv;
	WacomDevice *wacom_device;

	device = CSD_WACOM_DEVICE (g_object_new (CSD_TYPE_WACOM_DEVICE, NULL));

	if (db == NULL)
		db = libwacom_database_new ();

	wacom_device = libwacom_new_from_name (db, name, NULL);
	if (wacom_device == NULL)
		return NULL;

	priv = device->priv;
	priv->type = type;
	priv->tool_name = g_strdup (tool_name);
	csd_wacom_device_update_from_db (device, wacom_device, name);
	libwacom_destroy (wacom_device);

	return device;
}

GList *
csd_wacom_device_create_fake_cintiq (void)
{
	CsdWacomDevice *device;
	GList *devices;

	device = csd_wacom_device_create_fake (WACOM_TYPE_STYLUS,
					       "Wacom Cintiq 21UX2",
					       "Wacom Cintiq 21UX2 stylus");
	devices = g_list_prepend (NULL, device);

	device = csd_wacom_device_create_fake (WACOM_TYPE_ERASER,
					       "Wacom Cintiq 21UX2",
					       "Wacom Cintiq 21UX2 eraser");
	devices = g_list_prepend (devices, device);

	device = csd_wacom_device_create_fake (WACOM_TYPE_PAD,
					       "Wacom Cintiq 21UX2",
					       "Wacom Cintiq 21UX2 pad");
	devices = g_list_prepend (devices, device);

	return devices;
}

GList *
csd_wacom_device_create_fake_bt (void)
{
	CsdWacomDevice *device;
	GList *devices;

	device = csd_wacom_device_create_fake (WACOM_TYPE_STYLUS,
					       "Wacom Graphire Wireless",
					       "Graphire Wireless stylus");
	devices = g_list_prepend (NULL, device);

	device = csd_wacom_device_create_fake (WACOM_TYPE_ERASER,
					       "Wacom Graphire Wireless",
					       "Graphire Wireless eraser");
	devices = g_list_prepend (devices, device);

	device = csd_wacom_device_create_fake (WACOM_TYPE_PAD,
					       "Wacom Graphire Wireless",
					       "Graphire Wireless pad");
	devices = g_list_prepend (devices, device);

	device = csd_wacom_device_create_fake (WACOM_TYPE_CURSOR,
					       "Wacom Graphire Wireless",
					       "Graphire Wireless cursor");
	devices = g_list_prepend (devices, device);

	return devices;
}

GList *
csd_wacom_device_create_fake_x201 (void)
{
	CsdWacomDevice *device;
	GList *devices;

	device = csd_wacom_device_create_fake (WACOM_TYPE_STYLUS,
					       "Wacom Serial Tablet WACf004",
					       "Wacom Serial Tablet WACf004 stylus");
	devices = g_list_prepend (NULL, device);

	device = csd_wacom_device_create_fake (WACOM_TYPE_ERASER,
					       "Wacom Serial Tablet WACf004",
					       "Wacom Serial Tablet WACf004 eraser");
	devices = g_list_prepend (devices, device);

	return devices;
}

GList *
csd_wacom_device_create_fake_intuos4 (void)
{
	CsdWacomDevice *device;
	GList *devices;

	device = csd_wacom_device_create_fake (WACOM_TYPE_STYLUS,
					       "Wacom Intuos4 6x9",
					       "Wacom Intuos4 6x9 stylus");
	devices = g_list_prepend (NULL, device);

	device = csd_wacom_device_create_fake (WACOM_TYPE_ERASER,
					       "Wacom Intuos4 6x9",
					       "Wacom Intuos4 6x9 eraser");
	devices = g_list_prepend (devices, device);

	device = csd_wacom_device_create_fake (WACOM_TYPE_PAD,
					       "Wacom Intuos4 6x9",
					       "Wacom Intuos4 6x9 pad");
	devices = g_list_prepend (devices, device);

	device = csd_wacom_device_create_fake (WACOM_TYPE_CURSOR,
					       "Wacom Intuos4 6x9",
					       "Wacom Intuos4 6x9 cursor");
	devices = g_list_prepend (devices, device);

	return devices;
}
