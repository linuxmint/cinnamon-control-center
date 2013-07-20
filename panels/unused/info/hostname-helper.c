/*
 * Copyright (C) 2011 Red Hat, Inc
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
 * Foundation, Inc., 51 Franklin Street - Suite 500, Boston, MA 02110-1335, USA.
 *
 */

#include <glib.h>
#include <string.h>

#include "hostname-helper.h"

static char *
allowed_chars (void)
{
	GString *s;
	char i;

	s = g_string_new (NULL);
	for (i = 'a'; i <= 'z'; i++)
		g_string_append_c (s, i);
	for (i = 'A'; i <= 'Z'; i++)
		g_string_append_c (s, i);
	for (i = '0'; i <= '9'; i++)
		g_string_append_c (s, i);
	g_string_append_c (s, '-');

	return g_string_free (s, FALSE);
}

static char *
remove_leading_dashes (char *input)
{
	char *start;

	for (start = input; *start && (*start == '-'); start++)
		;

	g_memmove (input, start, strlen (start) + 1);

	return input;
}

static gboolean
is_empty (const char *input)
{
	if (input == NULL ||
	    *input == '\0')
		return TRUE;
	return FALSE;
}

static char *
remove_trailing_dashes (char *input)
{
	int len;

	len = strlen (input);
	while (len--) {
		if (input[len] == '-')
			input[len] = '\0';
		else
			break;
	}
	return input;
}

static char *
remove_apostrophes (char *input)
{
	char *apo;

	while ((apo = strchr (input, '\'')) != NULL)
		g_memmove (apo, apo + 1, strlen (apo));
	return input;
}

static char *
remove_duplicate_dashes (char *input)
{
	char *dashes;

	while ((dashes = strstr (input, "--")) != NULL)
		g_memmove (dashes, dashes + 1, strlen (dashes));
	return input;
}

#define CHECK	if (is_empty (result)) goto bail

char *
pretty_hostname_to_static (const char *pretty,
			   gboolean    for_display)
{
	char *result;
	char *valid_chars;

	g_return_val_if_fail (pretty != NULL, NULL);
	g_return_val_if_fail (g_utf8_validate (pretty, -1, NULL), NULL);

	g_debug ("Input: '%s'", pretty);

	/* Transform the pretty hostname to ASCII */
	result = g_convert (pretty,
			    -1,
			    "ASCII//TRANSLIT//IGNORE",
			    "UTF-8",
			    NULL,
			    NULL,
			    NULL);
	g_debug ("\ttranslit: '%s'", result);

	CHECK;

	/* Remove apostrophes */
	result = remove_apostrophes (result);
	g_debug ("\tapostrophes: '%s'", result);

	CHECK;

	/* Remove all the not-allowed chars */
	valid_chars = allowed_chars ();
	result = g_strcanon (result, valid_chars, '-');
	g_free (valid_chars);
	g_debug ("\tcanon: '%s'", result);

	CHECK;

	/* Remove the leading dashes */
	result = remove_leading_dashes (result);
	g_debug ("\tleading: '%s'", result);

	CHECK;

	/* Remove trailing dashes */
	result = remove_trailing_dashes (result);
	g_debug ("\ttrailing: '%s'", result);

	CHECK;

	/* Remove duplicate dashes */
	result = remove_duplicate_dashes (result);
	g_debug ("\tduplicate: '%s'", result);

	CHECK;

	/* Lower case */
	if (!for_display) {
		char *tmp;

		tmp = g_ascii_strdown (result, -1);
		g_free (result);
		result = tmp;
	}

	return result;

bail:
	g_free (result);
	return g_strdup ("localhost");
}
#undef CHECK
