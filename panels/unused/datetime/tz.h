/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* Generic timezone utilities.
 *
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Authors: Hans Petter Jansson <hpj@ximian.com>
 * 
 * Largely based on Michael Fulbright's work on Anaconda.
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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA 02110-1335, USA.
 */


#ifndef _E_TZ_H
#define _E_TZ_H

#include <glib.h>

#ifndef __sun
#  define TZ_DATA_FILE "/usr/share/zoneinfo/zone.tab"
#else
#  define TZ_DATA_FILE "/usr/share/lib/zoneinfo/tab/zone_sun.tab"
#endif

typedef struct _TzDB TzDB;
typedef struct _TzLocation TzLocation;
typedef struct _TzInfo TzInfo;


struct _TzDB
{
	GPtrArray  *locations;
	GHashTable *backward;
};

struct _TzLocation
{
	gchar *country;
	gdouble latitude;
	gdouble longitude;
	gchar *zone;
	gchar *comment;

	gdouble dist; /* distance to clicked point for comparison */
};

/* see the glibc info page information on time zone information */
/*  tzname_normal    is the default name for the timezone */
/*  tzname_daylight  is the name of the zone when in daylight savings */
/*  utc_offset       is offset in seconds from utc */
/*  daylight         if non-zero then location obeys daylight savings */

struct _TzInfo
{
	gchar *tzname_normal;
	gchar *tzname_daylight;
	glong utc_offset;
	gint daylight;
};


TzDB      *tz_load_db                 (void);
void       tz_db_free                 (TzDB *db);
char *     tz_info_get_clean_name     (TzDB *tz_db,
				       const char *tz);
GPtrArray *tz_get_locations           (TzDB *db);
void       tz_location_get_position   (TzLocation *loc,
				       double *longitude, double *latitude);
char      *tz_location_get_country    (TzLocation *loc);
gchar     *tz_location_get_zone       (TzLocation *loc);
gchar     *tz_location_get_comment    (TzLocation *loc);
glong      tz_location_get_utc_offset (TzLocation *loc);
gint       tz_location_set_locally    (TzLocation *loc);
TzInfo    *tz_info_from_location      (TzLocation *loc);
void       tz_info_free               (TzInfo *tz_info);

#endif
