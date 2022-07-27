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
#include "cc-wacom-output-manager.h"
#include "muffin-display-config.h"

typedef struct _CcWacomOutputManager CcWacomOutputManager;

struct _CcWacomOutputManager {
    GObject parent_instance;

    MetaDBusDisplayConfig      *proxy;
    GList          *monitors;
};

G_DEFINE_TYPE (CcWacomOutputManager, cc_wacom_output_manager, G_TYPE_OBJECT)

enum
{
  MONITORS_CHANGED,
  N_SIGNALS,
};

static guint cc_wacom_output_manager_signals[N_SIGNALS] = { 0 };

static void
monitor_info_free (MonitorInfo *info)
{
    g_free (info->connector_name);
    g_free (info->display_name);
    g_free (info->vendor);
    g_free (info->product);
    g_free (info->serial);

    g_slice_free (MonitorInfo, info);
}

gboolean
monitor_info_cmp (MonitorInfo *a, MonitorInfo *b)
{
    return (g_strcmp0 (a->vendor, b->vendor) == 0 &&
            g_strcmp0 (a->product, b->product) == 0 &&
            g_strcmp0 (a->serial, b->serial) == 0);
}

void
monitor_info_spew (MonitorInfo *info)
{
    g_printerr ("connector: %s, display_name: %s, vendor: %s, product: %s, serial: %s\n"
                "x_origin: %d, y_origin: %d, primary? %d, builtin? %d\n",
                info->connector_name, info->display_name, info->vendor, info->product, info->serial,
                info->x, info->y, info->primary, info->builtin);
}

#define MODE_BASE_FORMAT "siiddad"
#define MODE_FORMAT "(" MODE_BASE_FORMAT "a{sv})"
#define MODES_FORMAT "a" MODE_FORMAT
#define MONITOR_SPEC_FORMAT "(ssss)"
#define MONITOR_FORMAT "(" MONITOR_SPEC_FORMAT MODES_FORMAT "a{sv})"
#define MONITORS_FORMAT "a" MONITOR_FORMAT

#define LOGICAL_MONITOR_MONITORS_FORMAT "a" MONITOR_SPEC_FORMAT
#define LOGICAL_MONITOR_FORMAT "(iidub" LOGICAL_MONITOR_MONITORS_FORMAT "a{sv})"
#define LOGICAL_MONITORS_FORMAT "a" LOGICAL_MONITOR_FORMAT

static void
update_monitor_infos (CcWacomOutputManager  *manager,
                      GVariant              *monitors,
                      GVariant              *logical_monitors)
{
    GList *new_monitors = NULL;
    GVariantIter m_iter;
    g_variant_iter_init (&m_iter, monitors);
    while (TRUE)
    {
        g_autoptr(GVariant) variant = NULL;

        if (!g_variant_iter_next (&m_iter, "@"MONITOR_FORMAT, &variant))
            break;

        gchar *connector_name, *vendor, *product, *serial;
        g_autoptr(GVariantIter) modes = NULL;
        g_autoptr(GVariantIter) props = NULL;

        g_variant_get (variant, MONITOR_FORMAT,
                       &connector_name, &vendor, &product, &serial, &modes, &props);

        MonitorInfo *info = g_slice_new0 (MonitorInfo);

        info->connector_name = connector_name;
        info->vendor = vendor;
        info->product = product;
        info->serial = serial;

        while (TRUE)
        {
            const char *s;
            g_autoptr(GVariant) v = NULL;

            if (!g_variant_iter_next (props, "{&sv}", &s, &v))
              break;

            if (g_str_equal (s, "display-name"))
            {
                g_variant_get (v, "s", &info->display_name);
            }

            if (g_str_equal (s, "is-builtin"))
            {
                g_variant_get (v, "b", &info->builtin);
            }
        }

        GVariantIter l_iter;
        g_variant_iter_init (&l_iter, logical_monitors);

        while (TRUE)
        {
            g_autoptr(GVariant) variant = NULL;
            g_autoptr(GVariantIter) monitor_specs = NULL;
            const gchar *lconnector, *lvendor, *lproduct, *lserial;
            gint x, y;
            gboolean primary;
            gboolean found_one = FALSE;

            if (!g_variant_iter_next (&l_iter, "@"LOGICAL_MONITOR_FORMAT, &variant))
              break;

            g_variant_get (variant, LOGICAL_MONITOR_FORMAT,
                           &x,
                           &y,
                           NULL,
                           NULL,
                           &primary,
                           &monitor_specs,
                           NULL);

            while (g_variant_iter_next (monitor_specs, "(&s&s&s&s)", &lconnector, &lvendor, &lproduct, &lserial))
            {
                if (g_strcmp0 (info->connector_name, lconnector) == 0 &&
                    g_strcmp0 (info->vendor, lvendor) == 0 &&
                    g_strcmp0 (info->product, lproduct) == 0 &&
                    g_strcmp0 (info->serial, lserial) == 0) {
                  info->x = x;
                  info->y = y;
                  info->primary = primary;

                  found_one = TRUE;
                  break;
                }
            }

            // Only check the first logical monitor - we're only looking for the origin x, y of the physical one.
            if (found_one)
                break;
        }

        new_monitors = g_list_append (new_monitors, info);
    }

    manager->monitors = new_monitors;
}

static void
update_from_muffin (CcWacomOutputManager *manager)
{

    if (manager->monitors != NULL) {
        g_list_free_full (g_steal_pointer (&manager->monitors), (GDestroyNotify) monitor_info_free);
    }

    if (g_dbus_proxy_get_name_owner (G_DBUS_PROXY (manager->proxy)) != NULL) {
        GVariant *monitors, *logical_monitors, *properties;
        guint serial;
        GError *error = NULL;

        if (meta_dbus_display_config_call_get_current_state_sync (manager->proxy,
                                                                  &serial,
                                                                  &monitors,
                                                                  &logical_monitors,
                                                                  &properties,
                                                                  NULL,
                                                                  &error)) {
            update_monitor_infos (manager, monitors, logical_monitors);

            g_variant_unref (monitors);
            g_variant_unref (logical_monitors);
            g_variant_unref (properties);
        } else {
            g_critical ("GetCurrentState failed (%d): %s\n", error->code, error->message);
        }
    } else {
        g_critical ("Is Cinnamon running??");
    }

    g_signal_emit (manager, cc_wacom_output_manager_signals[MONITORS_CHANGED], 0);
}

static void
muffin_state_changed (gpointer data)
{
    g_return_if_fail (CC_IS_WACOM_OUTPUT_MANAGER (data));

    CcWacomOutputManager *manager = CC_WACOM_OUTPUT_MANAGER (data);
    update_from_muffin (manager);
}

static void
cc_wacom_output_manager_constructed (GObject *object)
{
    G_OBJECT_CLASS (cc_wacom_output_manager_parent_class)->constructed (object);

    CcWacomOutputManager *manager = CC_WACOM_OUTPUT_MANAGER (object);
    GError *error = NULL;

    manager->proxy = meta_dbus_display_config_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                                     G_DBUS_PROXY_FLAGS_NONE,
                                                                     "org.cinnamon.Muffin.DisplayConfig",
                                                                     "/org/cinnamon/Muffin/DisplayConfig",
                                                                     NULL, &error);

    if (manager->proxy == NULL) {
        if (error != NULL) {
            g_critical ("No connection to session bus: (%d) %s", error->code, error->message);
            return;
        }
    }

    g_signal_connect_object (manager->proxy, "notify::g-name-owner",
                             G_CALLBACK (muffin_state_changed), manager, G_CONNECT_SWAPPED);
    g_signal_connect_object (META_DBUS_DISPLAY_CONFIG (manager->proxy), "monitors-changed",
                             G_CALLBACK (muffin_state_changed), manager, G_CONNECT_SWAPPED);

    update_from_muffin (CC_WACOM_OUTPUT_MANAGER (object));
}

static void
cc_wacom_output_manager_init (CcWacomOutputManager *manage)
{
}

static void
cc_wacom_output_manager_finalize (GObject *object)
{
    CcWacomOutputManager *manager = CC_WACOM_OUTPUT_MANAGER (object);

    if (manager->monitors != NULL) {
        g_list_free_full (g_steal_pointer (&manager->monitors), (GDestroyNotify) monitor_info_free);
    }

    G_OBJECT_CLASS (cc_wacom_output_manager_parent_class)->finalize (object);
}

static void
cc_wacom_output_manager_class_init (CcWacomOutputManagerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->constructed = cc_wacom_output_manager_constructed;
    object_class->finalize = cc_wacom_output_manager_finalize;

    cc_wacom_output_manager_signals[MONITORS_CHANGED] =
        g_signal_new ("monitors-changed",
                      CC_TYPE_WACOM_OUTPUT_MANAGER,
                      G_SIGNAL_RUN_LAST,
                      0, NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);
}

CcWacomOutputManager *
cc_wacom_output_manager_get (void)
{
    static CcWacomOutputManager *singleton = NULL;

    if (singleton == NULL) {
        singleton = g_object_new (CC_TYPE_WACOM_OUTPUT_MANAGER, NULL);
    }

    return singleton;
}

GList *
cc_wacom_output_manager_get_all_monitors (CcWacomOutputManager *manager)
{
    g_return_val_if_fail (CC_IS_WACOM_OUTPUT_MANAGER (manager), NULL);

    return manager->monitors;
}

void
cc_wacom_output_manager_refresh_monitors (CcWacomOutputManager *manager)
{
    g_return_if_fail (CC_IS_WACOM_OUTPUT_MANAGER (manager));

    update_from_muffin (manager);
}
