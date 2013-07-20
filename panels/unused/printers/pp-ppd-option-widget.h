/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright 2012  Red Hat, Inc,
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
 * Author: Marek Kasik <mkasik@redhat.com>
 */

#ifndef __PP_PPD_OPTION_WIDGET_H__
#define __PP_PPD_OPTION_WIDGET_H__

#include <gtk/gtk.h>
#include <cups/cups.h>
#include <cups/ppd.h>

G_BEGIN_DECLS

#define PP_TYPE_PPD_OPTION_WIDGET                  (pp_ppd_option_widget_get_type ())
#define PP_PPD_OPTION_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PP_TYPE_PPD_OPTION_WIDGET, PpPPDOptionWidget))
#define PP_PPD_OPTION_WIDGET_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass),  PP_TYPE_PPD_OPTION_WIDGET, PpPPDOptionWidgetClass))
#define PP_IS_PPD_OPTION_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PP_TYPE_PPD_OPTION_WIDGET))
#define PP_IS_PPD_OPTION_WIDGET_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass),  PP_TYPE_PPD_OPTION_WIDGET))
#define PP_PPD_OPTION_WIDGET_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj),  PP_TYPE_PPD_OPTION_WIDGET, PpPPDOptionWidgetClass))

typedef struct _PpPPDOptionWidget         PpPPDOptionWidget;
typedef struct _PpPPDOptionWidgetClass    PpPPDOptionWidgetClass;
typedef struct PpPPDOptionWidgetPrivate   PpPPDOptionWidgetPrivate;

struct _PpPPDOptionWidget
{
  GtkHBox parent_instance;

  PpPPDOptionWidgetPrivate *priv;
};

struct _PpPPDOptionWidgetClass
{
  GtkHBoxClass parent_class;

  void (*changed) (PpPPDOptionWidget *widget);
};

GType	     pp_ppd_option_widget_get_type (void) G_GNUC_CONST;

GtkWidget   *pp_ppd_option_widget_new      (ppd_option_t *source,
                                            const gchar  *printer_name);

G_END_DECLS

#endif /* __PP_PPD_OPTION_WIDGET_H__ */
