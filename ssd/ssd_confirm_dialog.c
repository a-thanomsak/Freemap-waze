/* ssd_confirm_dialog.c - ssd confirmation dialog (yes/no).
 *
 * LICENSE:
 *
 *   Copyright 2008 Avi B.S
 *   Copyright 2008 Ehud Shabtai
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
 *   See ssd_confirm_dialog.h.h
 */
#include <string.h>
#include <stdlib.h>
#include "ssd_dialog.h"
#include "ssd_text.h"
#include "ssd_container.h"
#include "ssd_button.h"

#include "roadmap_lang.h"
#include "roadmap_main.h"
#include "roadmap_softkeys.h"
#include "roadmap_screen.h"
#include "ssd_confirm_dialog.h"


typedef struct {
	void *context;
	ConfirmDialogCallback callback;
	BOOL default_yes;
} confirm_dialog_context;

static RoadMapCallback MessageBoxCallback = NULL;

static void kill_messagebox_timer (void) {

   if (MessageBoxCallback) {
      roadmap_main_remove_periodic (MessageBoxCallback);
      MessageBoxCallback = NULL;
   }
}

void ssd_confirm_dialog_close (void)
{
   confirm_dialog_context *data;
   ConfirmDialogCallback callback = NULL;
   SsdWidget dialog;
   int exit_code = dec_ok;
   
   dialog = ssd_dialog_context();
   data = (confirm_dialog_context *) ssd_dialog_context();
   if (data){
      callback = (ConfirmDialogCallback)data->callback;
      if (data->default_yes)
         exit_code = dec_yes;
      else
         exit_code = dec_no;
   }
   callback = (ConfirmDialogCallback)data->callback;
   kill_messagebox_timer ();
   ssd_dialog_hide ("confirm_dialog", dec_ok);
   
   if (!roadmap_screen_refresh())
      roadmap_screen_redraw();
   
   if (callback)
      (*callback)(exit_code, data->context);
}

static int yes_button_callback (SsdWidget widget, const char *new_value) {

	SsdWidget dialog;
	ConfirmDialogCallback callback;
	confirm_dialog_context *data;

	dialog = widget->parent;
	data = (confirm_dialog_context *)dialog->context;

	callback = (ConfirmDialogCallback)data->callback;

	ssd_dialog_hide ("confirm_dialog", dec_yes);

   (*callback)(dec_yes, data->context);

   kill_messagebox_timer ();
	return 0;
}

static int no_button_callback (SsdWidget widget, const char *new_value) {


    SsdWidget dialog;
    ConfirmDialogCallback callback;
	 confirm_dialog_context *data;

    dialog = widget->parent;
    data = (confirm_dialog_context  *)dialog->context;

    callback = (ConfirmDialogCallback)data->callback;

    ssd_dialog_hide ("confirm_dialog", dec_no);

    (*callback)(dec_no, data->context);

    kill_messagebox_timer();
    return 0;
}

#ifndef TOUCH_SCREEN
static int no_softkey_callback (SsdWidget widget, const char *new_value, void *context){
	return no_button_callback(widget->children, new_value);
}

static int yes_softkey_callback (SsdWidget widget, const char *new_value, void *context){
	return yes_button_callback(widget->children, new_value);
}

static void set_soft_keys(SsdWidget dialog, const char * textYes, const char *textNo){

	ssd_widget_set_right_softkey_text(dialog,textNo);
	ssd_widget_set_right_softkey_callback(dialog,no_softkey_callback);
	ssd_widget_set_left_softkey_text(dialog, textYes);
	ssd_widget_set_left_softkey_callback(dialog,yes_softkey_callback);
}
#endif


static void create_confirm_dialog (BOOL default_yes, const char * textYes, const char *textNo) {
   SsdWidget text;
   SsdWidget dialog,widget_title;
   SsdWidget text_con;
   SsdSize dlg_size;
   // const char *question_icon[] = {"question"};

#ifndef TOUCH_SCREEN
   int yes_flags = 0;
   int no_flags = 0;

   if(default_yes)
      yes_flags|= SSD_WS_DEFWIDGET;
   else
      no_flags |= SSD_WS_DEFWIDGET;
#endif

   dialog = ssd_dialog_new ("confirm_dialog", "", NULL,
         SSD_CONTAINER_BORDER|SSD_DIALOG_FLOAT|
         SSD_ALIGN_CENTER|SSD_CONTAINER_TITLE|SSD_ALIGN_VCENTER|SSD_ROUNDED_CORNERS|SSD_POINTER_NONE);
   ssd_widget_set_color (dialog, "#000000", "#ff0000000");


   ssd_widget_get_size( dialog, &dlg_size, NULL );
   /* Spacer */
   ssd_widget_add (dialog,
      ssd_container_new ("spacer1", NULL, 0, 15, SSD_END_ROW));

   /*
   image_container = ssd_container_new ("image_container", NULL,
                                  SSD_MIN_SIZE,
                                  SSD_MIN_SIZE,
                                  SSD_ALIGN_RIGHT);

   ssd_widget_set_color (image_container, "#000000", "#ff0000000");
   ssd_widget_add (image_container,
        ssd_button_new ("question", "question", question_icon , 1,
                           SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER ,
                           NULL));
   // Image container
   ssd_widget_add (dialog,image_container);
    */

   text_con = ssd_container_new ("text_container", NULL,
                                  ( dlg_size.width * 9 )/10,	/* 90% of dialog width */
                                  SSD_MIN_SIZE,
                                  SSD_END_ROW|SSD_ALIGN_CENTER );
   ssd_widget_set_color (text_con, "#000000", "#ff0000000");


   // Text box
   text =  ssd_text_new ("text", "", 16, SSD_END_ROW|SSD_WIDGET_SPACE);

   ssd_widget_add (text_con,text);

  ssd_widget_add(dialog, text_con);

  widget_title = ssd_widget_get( dialog, "title_text" );
  ssd_text_set_font_size( widget_title, 20 );

#ifdef TOUCH_SCREEN

  ssd_dialog_add_vspace( dialog, 10, SSD_START_NEW_ROW );

    ssd_widget_add (dialog,
    ssd_button_label (roadmap_lang_get ("Yes"), textYes,
                        SSD_ALIGN_CENTER| SSD_WS_TABSTOP,
                        yes_button_callback));

   ssd_widget_add (dialog,
		   ssd_button_label (roadmap_lang_get ("No"), textNo,
                        SSD_ALIGN_CENTER| SSD_WS_TABSTOP,
                        no_button_callback));

#else
	set_soft_keys(dialog, textYes, textNo);

#endif
    ssd_widget_add (dialog,
      ssd_container_new ("spacer2", NULL, 0, 10, SSD_START_NEW_ROW|SSD_WIDGET_SPACE));
}

void ssd_confirm_dialog_custom (const char *title, const char *text, BOOL default_yes, ConfirmDialogCallback callback, void *context,const char *textYes, const char *textNo) {

SsdWidget dialog;
  confirm_dialog_context *data =
    (confirm_dialog_context  *)calloc (1, sizeof(*data));

  data->default_yes = default_yes;
  dialog = ssd_dialog_activate ("confirm_dialog", NULL);
  title = roadmap_lang_get (title);
  text  = roadmap_lang_get (text);

  if (!dialog) {
      create_confirm_dialog (default_yes,textYes,textNo);
      dialog = ssd_dialog_activate ("confirm_dialog", NULL);
  }
  else{
#ifdef TOUCH_SCREEN
   //set button text & softkeys
   SsdWidget buttonYes;
   SsdWidget buttonNo;
   buttonYes = ssd_widget_get(dialog, roadmap_lang_get ("Yes")); // change the buttons to custom text
   ssd_button_change_text(buttonYes, textYes);
   buttonNo = ssd_widget_get(dialog, roadmap_lang_get ("No"));
   ssd_button_change_text(buttonNo, textNo);
#else //Non touch
   set_soft_keys(dialog, textYes, textNo); // change softkeys text to custom text
   ssd_dialog_refresh_current_softkeys();
#endif
  }

  if (title[0] == 0){
       ssd_widget_hide(ssd_widget_get(dialog, "title_bar"));  
  }
  else{
       ssd_widget_show(ssd_widget_get(dialog, "title_bar"));
  }
   
  data->callback = callback;
  data->context = context;

  dialog->set_value (dialog, title);
  ssd_widget_set_value (dialog, "text", text);
  dialog->context = data;
  ssd_dialog_draw ();
}


void ssd_confirm_dialog (const char *title, const char *text, BOOL default_yes, ConfirmDialogCallback callback, void *context) {

	ssd_confirm_dialog_custom(title, text, default_yes, callback, context, roadmap_lang_get("Yes"),roadmap_lang_get("No")); 

}

void ssd_confirm_dialog_custom_timeout (const char *title, const char *text, BOOL default_yes, ConfirmDialogCallback callback, void *context,const char *textYes, const char *textNo, int seconds) {
   ssd_confirm_dialog_custom (title, text,default_yes,callback,context,textYes,textNo);
   MessageBoxCallback = ssd_confirm_dialog_close;
   roadmap_main_set_periodic (seconds * 1000, ssd_confirm_dialog_close );
}


void ssd_confirm_dialog_timeout (const char *title, const char *text, BOOL default_yes, ConfirmDialogCallback callback, void *context, int seconds) {

   ssd_confirm_dialog (title, text,default_yes,callback, context);
   MessageBoxCallback = ssd_confirm_dialog_close;
   roadmap_main_set_periodic (seconds * 1000, ssd_confirm_dialog_close );
}

