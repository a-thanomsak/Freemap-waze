/* ssd_checkbox.c - check box widget
 *
 * LICENSE:
 *
 *   Copyright 2006 Ehud Shabtai
 *
 *   This file is part of RoadMap.
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   RoadMap is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with RoadMap; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * SYNOPSYS:
 *
 *   See ssd_checkbox.h.
 */

#include <string.h>
#include <stdlib.h>

#include "ssd_dialog.h"
#include "ssd_container.h"
#include "ssd_button.h"
#include "ssd_text.h"

#include "ssd_checkbox.h"

struct ssd_checkbox_data {
   SsdCallback callback;
   BOOL selected;
   int style;
};

static const char *yesno[] = {"yes", "no"};

static const char *checked_button[] = {"checkbox_on", "round_checkbox_checked"};
static const char *unchecked_button[] = {"checkbox_off", "round_checkbox_unchecked"};


static int choice_callback (SsdWidget widget, const char *new_value) {

   struct ssd_checkbox_data *data;
   SsdWidget widget_parent;

   widget_parent = widget->parent;

   data = (struct ssd_checkbox_data *)widget_parent->data;

   if (data->selected)
        ssd_button_change_icon(widget,&unchecked_button[data->style],1 );
   else
        ssd_button_change_icon(widget,&checked_button[data->style],1);

   data->selected = !data->selected;

   if (data->callback)
    (*data->callback)(widget, new_value);
   return 1;
}


static const char *get_value (SsdWidget widget) {
  struct ssd_checkbox_data *data;

   widget = widget->parent;

   data = (struct ssd_checkbox_data *)widget->data;
   if (data->selected)
        return yesno[0];
   else
    return yesno[1];
}


static const void *get_data (SsdWidget widget) {
   struct ssd_checkbox_data *data = (struct ssd_checkbox_data *)widget->data;

   data = (struct ssd_checkbox_data *)widget->data;
   if (data->selected)
        return yesno[0];
   else
    return yesno[1];
}


static int set_value (SsdWidget widget, const char *value) {
   struct ssd_checkbox_data *data = (struct ssd_checkbox_data *)widget->data;

   if ((data->callback) && !(*data->callback) (widget, value)) {
      return 0;
   }

   return ssd_widget_set_value (widget, "Label", value);
}


static int set_data (SsdWidget widget, const void *value) {

   struct ssd_checkbox_data *data = (struct ssd_checkbox_data *)widget->data;

    if ((!strcmp((char *)value,"Yes")) ||  (!strcmp((char *)value,"yes"))){
        data->selected = TRUE;
        ssd_button_change_icon(widget->children,&checked_button[data->style],1 );
    }
    else{
        data->selected = FALSE;
        ssd_button_change_icon(widget->children,&unchecked_button[data->style],1 );
    }
    return 1;
}


SsdWidget ssd_checkbox_new (const char *name,
                          BOOL Selected,
                          int flags,
                          SsdCallback callback,
                          int style) {


   SsdWidget button;

   struct ssd_checkbox_data *data =
      (struct ssd_checkbox_data *)calloc (1, sizeof(*data));

   SsdWidget choice =
      ssd_container_new (name, NULL, SSD_MIN_SIZE, SSD_MIN_SIZE, flags);

   SsdWidget text_box =
      ssd_container_new ("text_box", NULL, SSD_MIN_SIZE,
                         SSD_MIN_SIZE,
                         SSD_WS_TABSTOP|SSD_ALIGN_VCENTER);

   data->callback = callback;
   data->selected = Selected;
   data->style = style;
   choice->get_value = get_value;
   choice->get_data = get_data;
   choice->set_value = set_value;
   choice->set_data = set_data;
   choice->data = data;
   choice->bg_color = NULL;

   text_box->callback = choice_callback;
   text_box->bg_color = NULL;

   if (Selected)
    button = ssd_button_new ("checkbox_button", "", &checked_button[data->style], 1,
                   SSD_ALIGN_VCENTER|SSD_WS_TABSTOP, choice_callback);
   else
    button = ssd_button_new ("checkbox_button", "", &unchecked_button[data->style], 1,
                   SSD_ALIGN_VCENTER|SSD_WS_TABSTOP, choice_callback);

   ssd_widget_add (choice, button);

   return choice;
}

