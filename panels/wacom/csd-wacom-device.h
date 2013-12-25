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

#ifndef __CSD_WACOM_DEVICE_MANAGER_H
#define __CSD_WACOM_DEVICE_MANAGER_H

#include <glib-object.h>
#include "csd-enums.h"

G_BEGIN_DECLS

#define NUM_ELEMS_MATRIX 9

#define CSD_TYPE_WACOM_DEVICE         (gsd_wacom_device_get_type ())
#define CSD_WACOM_DEVICE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CSD_TYPE_WACOM_DEVICE, CsdWacomDevice))
#define CSD_WACOM_DEVICE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), CSD_TYPE_WACOM_DEVICE, CsdWacomDeviceClass))
#define CSD_IS_WACOM_DEVICE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CSD_TYPE_WACOM_DEVICE))
#define CSD_IS_WACOM_DEVICE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), CSD_TYPE_WACOM_DEVICE))
#define CSD_WACOM_DEVICE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), CSD_TYPE_WACOM_DEVICE, CsdWacomDeviceClass))

typedef struct CsdWacomDevicePrivate CsdWacomDevicePrivate;

typedef struct
{
        GObject                parent;
        CsdWacomDevicePrivate *priv;
} CsdWacomDevice;

typedef struct
{
        GObjectClass   parent_class;
} CsdWacomDeviceClass;

#define CSD_TYPE_WACOM_STYLUS         (gsd_wacom_stylus_get_type ())
#define CSD_WACOM_STYLUS(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CSD_TYPE_WACOM_STYLUS, CsdWacomStylus))
#define CSD_WACOM_STYLUS_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), CSD_TYPE_WACOM_STYLUS, CsdWacomStylusClass))
#define CSD_IS_WACOM_STYLUS(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CSD_TYPE_WACOM_STYLUS))
#define CSD_IS_WACOM_STYLUS_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), CSD_TYPE_WACOM_STYLUS))
#define CSD_WACOM_STYLUS_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), CSD_TYPE_WACOM_STYLUS, CsdWacomStylusClass))

typedef struct CsdWacomStylusPrivate CsdWacomStylusPrivate;

typedef struct
{
        GObject                parent;
        CsdWacomStylusPrivate *priv;
} CsdWacomStylus;

typedef struct
{
        GObjectClass   parent_class;
} CsdWacomStylusClass;

typedef enum {
	WACOM_STYLUS_TYPE_UNKNOWN,
	WACOM_STYLUS_TYPE_GENERAL,
	WACOM_STYLUS_TYPE_INKING,
	WACOM_STYLUS_TYPE_AIRBRUSH,
	WACOM_STYLUS_TYPE_CLASSIC,
	WACOM_STYLUS_TYPE_MARKER,
	WACOM_STYLUS_TYPE_STROKE,
	WACOM_STYLUS_TYPE_PUCK
} CsdWacomStylusType;

GType            gsd_wacom_stylus_get_type       (void);
GSettings      * gsd_wacom_stylus_get_settings   (CsdWacomStylus *stylus);
const char     * gsd_wacom_stylus_get_name       (CsdWacomStylus *stylus);
const char     * gsd_wacom_stylus_get_icon_name  (CsdWacomStylus *stylus);
CsdWacomDevice * gsd_wacom_stylus_get_device     (CsdWacomStylus *stylus);
gboolean         gsd_wacom_stylus_get_has_eraser (CsdWacomStylus *stylus);
guint            gsd_wacom_stylus_get_num_buttons(CsdWacomStylus *stylus);
int              gsd_wacom_stylus_get_id         (CsdWacomStylus *stylus);
CsdWacomStylusType gsd_wacom_stylus_get_stylus_type (CsdWacomStylus *stylus);

/* Tablet Buttons */
typedef enum {
	WACOM_TABLET_BUTTON_TYPE_NORMAL,
	WACOM_TABLET_BUTTON_TYPE_STRIP,
	WACOM_TABLET_BUTTON_TYPE_RING,
	WACOM_TABLET_BUTTON_TYPE_HARDCODED
} CsdWacomTabletButtonType;

/*
 * Positions of the buttons on the tablet in default right-handed mode
 * (ie with no rotation applied).
 */
typedef enum {
	WACOM_TABLET_BUTTON_POS_UNDEF = 0,
	WACOM_TABLET_BUTTON_POS_LEFT,
	WACOM_TABLET_BUTTON_POS_RIGHT,
	WACOM_TABLET_BUTTON_POS_TOP,
	WACOM_TABLET_BUTTON_POS_BOTTOM
} CsdWacomTabletButtonPos;

#define MAX_GROUP_ID 4

#define CSD_WACOM_NO_LED -1

typedef struct
{
	char                     *name;
	char                     *id;
	GSettings                *settings;
	CsdWacomTabletButtonType  type;
	CsdWacomTabletButtonPos   pos;
	int                       group_id, idx;
	int                       status_led;
} CsdWacomTabletButton;

void                  gsd_wacom_tablet_button_free (CsdWacomTabletButton *button);
CsdWacomTabletButton *gsd_wacom_tablet_button_copy (CsdWacomTabletButton *button);

/* Device types to apply a setting to */
typedef enum {
	WACOM_TYPE_INVALID =     0,
        WACOM_TYPE_STYLUS  =     (1 << 1),
        WACOM_TYPE_ERASER  =     (1 << 2),
        WACOM_TYPE_CURSOR  =     (1 << 3),
        WACOM_TYPE_PAD     =     (1 << 4),
        WACOM_TYPE_TOUCH   =     (1 << 5),
        WACOM_TYPE_ALL     =     WACOM_TYPE_STYLUS | WACOM_TYPE_ERASER | WACOM_TYPE_CURSOR | WACOM_TYPE_PAD | WACOM_TYPE_TOUCH
} CsdWacomDeviceType;

/* We use -1 for entire screen when setting/getting monitor value */
#define CSD_WACOM_SET_ALL_MONITORS -1

GType gsd_wacom_device_get_type     (void);

void     gsd_wacom_device_set_display         (CsdWacomDevice    *device,
                                               int                monitor);
gint     gsd_wacom_device_get_display_monitor (CsdWacomDevice *device);
gboolean gsd_wacom_device_get_display_matrix  (CsdWacomDevice *device,
                                               float           matrix[NUM_ELEMS_MATRIX]);
CsdWacomRotation gsd_wacom_device_get_display_rotation (CsdWacomDevice *device);

CsdWacomDevice * gsd_wacom_device_new              (GdkDevice *device);
GList          * gsd_wacom_device_list_styli       (CsdWacomDevice *device);
const char     * gsd_wacom_device_get_name         (CsdWacomDevice *device);
const char     * gsd_wacom_device_get_layout_path  (CsdWacomDevice *device);
const char     * gsd_wacom_device_get_path         (CsdWacomDevice *device);
const char     * gsd_wacom_device_get_icon_name    (CsdWacomDevice *device);
const char     * gsd_wacom_device_get_tool_name    (CsdWacomDevice *device);
gboolean         gsd_wacom_device_reversible       (CsdWacomDevice *device);
gboolean         gsd_wacom_device_is_screen_tablet (CsdWacomDevice *device);
gboolean         gsd_wacom_device_is_isd           (CsdWacomDevice *device);
gboolean         gsd_wacom_device_is_fallback      (CsdWacomDevice *device);
gint             gsd_wacom_device_get_num_strips   (CsdWacomDevice *device);
gint             gsd_wacom_device_get_num_rings    (CsdWacomDevice *device);
GSettings      * gsd_wacom_device_get_settings     (CsdWacomDevice *device);
void             gsd_wacom_device_set_current_stylus (CsdWacomDevice *device,
						      int             stylus_id);
CsdWacomStylus * gsd_wacom_device_get_stylus_for_type (CsdWacomDevice     *device,
						       CsdWacomStylusType  type);

CsdWacomDeviceType gsd_wacom_device_get_device_type (CsdWacomDevice *device);
gint           * gsd_wacom_device_get_area          (CsdWacomDevice *device);
const char     * gsd_wacom_device_type_to_string    (CsdWacomDeviceType type);
GList          * gsd_wacom_device_get_buttons       (CsdWacomDevice *device);
CsdWacomTabletButton *gsd_wacom_device_get_button   (CsdWacomDevice   *device,
						     int               button,
						     GtkDirectionType *dir);
int gsd_wacom_device_get_num_modes                  (CsdWacomDevice   *device,
						     int               group_id);
int gsd_wacom_device_get_current_mode               (CsdWacomDevice   *device,
						     int               group_id);
int gsd_wacom_device_set_next_mode                  (CsdWacomDevice       *device,
						     CsdWacomTabletButton *button);
CsdWacomRotation gsd_wacom_device_rotation_name_to_type (const char *rotation);
const char     * gsd_wacom_device_rotation_type_to_name (CsdWacomRotation type);


/* Helper and debug functions */
CsdWacomDevice * gsd_wacom_device_create_fake (CsdWacomDeviceType  type,
					       const char         *name,
					       const char         *tool_name);

GList * gsd_wacom_device_create_fake_cintiq   (void);
GList * gsd_wacom_device_create_fake_bt       (void);
GList * gsd_wacom_device_create_fake_x201     (void);
GList * gsd_wacom_device_create_fake_intuos4  (void);

G_END_DECLS

#endif /* __CSD_WACOM_DEVICE_MANAGER_H */
