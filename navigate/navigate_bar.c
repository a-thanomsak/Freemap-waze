 /* navigate_bar.c - implement navigation bar
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
 *   See navigate_bar.h
 */

#include <stdlib.h>
#include <string.h>
#include "roadmap.h"
#include "roadmap_canvas.h"
#include "roadmap_screen_obj.h"
#include "roadmap_file.h"
#include "roadmap_res.h"
#include "roadmap_math.h"
#include "roadmap_lang.h"
#include "roadmap_bar.h"
#include "roadmap_message.h"
#include "roadmap_utf8.h"
#include "roadmap_navigate.h"
#include "ssd/ssd_widget.h"

#include "navigate_main.h"
#include "navigate_bar.h"
#include "roadmap_config.h"

static RoadMapConfigDescriptor RMConfigNavBarDestDistanceFont =
                        ROADMAP_CONFIG_ITEM("Navigation Bar", "Destination Distance Font");
static RoadMapConfigDescriptor RMConfigNavBarDestTimeFont =
                        ROADMAP_CONFIG_ITEM("Navigation Bar", "Destination Time Font");
static RoadMapConfigDescriptor RMConfigNavBarStreetNormalFont =
                        ROADMAP_CONFIG_ITEM("Navigation Bar", "Street Normal Font");
static RoadMapConfigDescriptor RMConfigNavBarStreetSmalllFont =
                        ROADMAP_CONFIG_ITEM("Navigation Bar", "Street Small Font");

typedef struct {
   const char     *image_file;
   int             min_screen_width;
   int             start_y_pos;
   RoadMapGuiPoint instruction_pos;
   RoadMapGuiPoint exit_pos;
   RoadMapGuiRect  distance_rect;
   RoadMapGuiPoint distance_value_pos;
   RoadMapGuiPoint distance_unit_pos;
   int		   street_line1_pos;
   int		   street_line2_pos;
   int             street_start;
   int             street_width;

   RoadMapGuiRect  speed_rect;
   RoadMapGuiPoint speed_pos;
   RoadMapGuiPoint speed_unit_pos;

   RoadMapGuiRect  distance_to_destination_rect;
   RoadMapGuiPoint distance_to_destination_pos;
	RoadMapGuiPoint distance_to_destination_unit_pos;

   RoadMapGuiRect  time_to_destination_rect;
   RoadMapGuiPoint time_to_destination_pos;


} NavigateBarPanel;


#define  NAV_BAR_TEXT_COLOR                   ("#000000")
#define  NAV_BAR_TEXT_COLOR_GREEN             ("#d7ff00")

#define NAV_BAR_TIME_TO_DESTINATION_COLOR     ("#000000")
#define NAV_BAR_DIST_TO_DESTINATION_COLOR     ("#000000")
#define NAV_BAR_SPEED_COLOR                   ("#000000")

#define NAV_BAR_CLOSE_OBJ_NAME 	("close_nav")

static NavigateBarPanel NavigateBarDefaultPanels[] = {

#ifndef TOUCH_SCREEN
   {"nav_panel_wide", 320,0, {0, 19}, {25, 33}, {43, 20, 95, 55}, {52, 25}, {48, 46}, 39,-1, 106, 200, {270, 0, 320, 37}, {276, 8}, {277, 32}, {400, 0, 400, 30}, {400, 400}, {400, 400}, {400, 0, 400, 22}, {400,400} },

   {"nav_panel", 240, 21, {6, 23}, {31, 37}, {0, 54, 78, 89}, {32, 61}, {10, 70}, 48, -1, 78, 161, {190, 8, 240, 41}, {196, 12}, {195, 36}, {181, 66, 240, 83}, {216, 72}, {188, 72}, {95, 66, 180, 83}, {96, 72} },
#else
   {"nav_panel_wide_640", 640,64, {15, 18}, {66, 60}, {0, 95, 123, 170}, {58, 105}, {24, 118}, 106,-1, 95, 430, {560, 8, 640, 70}, {567, 23}, {572, 57}, {544, 90, 640, 150}, {595, 103}, {550, 103}, {530, 118 , 640, 150}, {540,128} },

#ifdef _WIN32
   {"nav_panel_wide", 400,50, {5, 8}, {41, 30}, {0, 50, 87, 110}, {38, 60}, {14, 73}, 66,-1, 95, 302, {350, 8, 400, 44}, {347, 13}, {352, 37}, {480, 57, 480, 78}, {445, 63}, {415, 63}, {480, 78 , 480, 90}, {480,78} },

   {"nav_panel_wide_320", 320,50, {5, 8}, {41, 30}, {0, 50, 87, 110}, {38, 60}, {14, 73}, 66,-1, 95, 222, {270, 8, 320, 44}, {267, 13}, {272, 37}, {480, 57, 480, 78}, {445, 63}, {415, 63}, {480, 78 , 480, 90}, {480,78} },

   {"nav_panel_small", 240, 50, {5, 28}, {41, 50}, {0, 70, 82, 130}, {38, 80}, {14, 93}, 62, 77, 95, 140, {185, 1, 240, 40}, {191, 12}, {195, 37}, {176, 94, 240, 120}, {212, 98}, {186, 98}, {94, 94, 180, 120}, {98, 98} },
#else
   {"nav_panel_wide", 400,50, {5, 8}, {41, 30}, {0, 50, 87, 110}, {38, 60}, {14, 73}, 66,-1, 95, 290, {430, 8, 480, 44}, {427, 13}, {432, 37}, {395, 57, 480, 78}, {445, 63}, {415, 63}, {390, 78 , 480, 90}, {396,78} },

   {"nav_panel_360", 360, 64, {5, 28}, {57, 70}, {0, 100, 123, 170}, {58, 110}, {24, 123}, 88, 115, 135, 215, {280, 1, 365, 50}, {281, 22}, {285, 57}, {200, 130, 280, 180}, {250, 138}, {220, 138}, {130, 130, 199, 180}, {132, 138} },

   {"nav_panel", 320, 50, {5, 40}, {40, 60}, {0, 82, 87, 142}, {41, 94}, {14, 105}, 70, 89, 95, 220, {257, 3, 317, 42}, {262, 14}, {272, 39}, {148, 107, 270, 135}, {230, 110}, {200, 110}, {92, 107, 145, 135}, {96, 110} },

   {"nav_panel_small", 240, 50, {5, 28}, {41, 50}, {0, 70, 82, 130}, {38, 80}, {14, 93}, 62, 77, 95, 140, {185, 1, 240, 40}, {191, 12}, {195, 37}, {176, 94, 240, 120}, {212, 98}, {186, 98}, {94, 94, 180, 120}, {98, 98} },
#endif

#endif
};

static NavigateBarPanel *NavigatePanel = NULL;
static RoadMapScreenSubscriber navigate_prev_after_refresh = NULL;


const char NAVIGATE_DIR_IMG[][40] = {
   "nav_turn_left",
   "nav_turn_right",
   "nav_keep_left",
   "nav_keep_right",
   "nav_continue",
   "nav_roundabout_e",
   "nav_roundabout_e",
   "nav_roundabout_l",
   "nav_roundabout_l",
   "nav_roundabout_s",
   "nav_roundabout_s",
   "nav_roundabout_r",
   "nav_roundabout_r",
   "nav_roundabout_u",
   "nav_roundabout_u",
   "nav_approaching"
};

static RoadMapImage NavigateBarImage;
static RoadMapImage NavigateBarBG;
RoadMapImage NavigateDirections[LAST_DIRECTION];
static int NavigateBarInitialized = 0;
static RoadMapGuiPoint NavigateBarLocation;
static RoadMapGuiPoint NavigateBarLocOffsets = { 0, 0 };

static enum NavigateInstr NavigateBarCurrentInstr = LAST_DIRECTION;
static char NavigateBarCurrentStreet[256] = {0};
static int  NavigateBarCurrentDistance = -1;
static int  NavigateBarEnabled = 0;

static int height;

static int navigate_bar_align_text (char *text, char **line1, char **line2,
                                    int size) {

   int width, ascent, descent;

   roadmap_canvas_get_text_extents
      (text, size, &width, &ascent, &descent, NULL);

   if (width >= 2 * NavigatePanel->street_width) return -1;

   if (width < NavigatePanel->street_width) {

      *line1 = text;
      return 1;

   } else {

      /* Check if we can place the text in two lines */

      char *text_line = text;
      char *text_end = text_line + strlen(text_line);
      char *p1 = text_line + (strlen(text_line) / 2);
      char *p2 = p1;

      while (p1 > text_line) {
         if (*p1 == ' ') {
            break;
         }
         p1 -= 1;
      }
      while (p2 < text_end) {
         if (*p2 == ' ') {
            break;
         }
         p2 += 1;
      }
      if (text_end - p1 > p2 - text_line) {
         p1 = p2;
      }
      if (p1 > text_line) {

         char saved = *p1;
         *p1 = 0;

         roadmap_canvas_get_text_extents
            (text_line, size, &width, &ascent, &descent, NULL);

         if (width < NavigatePanel->street_width) {

            roadmap_canvas_get_text_extents
               (text_line, size, &width, &ascent, &descent, NULL);

            if (width < NavigatePanel->street_width) {

               *line1 = text_line;
               *line2 = p1 + 1;
               return 2;
            }
         }

         *p1 = saved;
      }
   }

   return -1;
}

void navigate_bar_resize(void){
   int i;
   int width;

   width = roadmap_canvas_width ();
	height = roadmap_canvas_height();

   for (i=0;
       (unsigned)i<
        sizeof(NavigateBarDefaultPanels)/sizeof(NavigateBarDefaultPanels[0]);
       i++) {

      if (width >= NavigateBarDefaultPanels[i].min_screen_width) {
         NavigatePanel = NavigateBarDefaultPanels + i;
         break;
      }
   }

   if (!NavigatePanel) {
      roadmap_log (ROADMAP_ERROR, "Can't find nav panel for screen width: %d",
            width);
      NavigateBarInitialized = -1;
      return;
   }

   NavigateBarBG =
      (RoadMapImage) roadmap_res_get
         (RES_BITMAP, RES_SKIN|RES_NOCACHE, NavigatePanel->image_file);

   NavigateBarImage =
      (RoadMapImage) roadmap_res_get
         (RES_BITMAP, RES_SKIN, NavigatePanel->image_file);

   if (!NavigateBarBG || !NavigateBarImage) return;

   roadmap_canvas_image_set_mutable (NavigateBarImage);

   NavigateBarLocation.x = 0;
   NavigateBarLocation.y = height - roadmap_canvas_image_height(NavigateBarBG) - roadmap_bar_bottom_height();
}

static void navigate_bar_after_refresh (void) {

	int new_height;


   if (NavigateBarEnabled) {
		new_height = roadmap_canvas_height();
		if (new_height != height){
   			navigate_bar_resize();
   			navigate_bar_set_instruction(NavigateBarCurrentInstr);
   			navigate_bar_set_street(NavigateBarCurrentStreet);
   			navigate_bar_set_distance(NavigateBarCurrentDistance);
   			roadmap_screen_refresh();
		}
		navigate_bar_set_speed();
	    navigate_bar_set_distance_to_destination();
	    navigate_bar_set_time_to_destination();

      navigate_bar_draw ();
   }

   if (navigate_prev_after_refresh) {
      (*navigate_prev_after_refresh) ();
   }
}



void navigate_bar_initialize (void) {

   int i;
   int width;

   if (NavigateBarInitialized) return;

   roadmap_config_declare
      ("preferences", &RMConfigNavBarDestDistanceFont, "14", NULL);
   roadmap_config_declare
      ("preferences", &RMConfigNavBarDestTimeFont, "14", NULL);
   roadmap_config_declare
      ("preferences", &RMConfigNavBarStreetNormalFont, "20", NULL);
   roadmap_config_declare
      ("preferences", &RMConfigNavBarStreetSmalllFont, "17", NULL);

   width = roadmap_canvas_width ();
	height = roadmap_canvas_height();

   for (i=0;
       (unsigned)i<
        sizeof(NavigateBarDefaultPanels)/sizeof(NavigateBarDefaultPanels[0]);
       i++) {

      if (width >= NavigateBarDefaultPanels[i].min_screen_width) {
         NavigatePanel = NavigateBarDefaultPanels + i;
         break;
      }
   }

   if (!NavigatePanel) {
      roadmap_log (ROADMAP_ERROR, "Can't find nav panel for screen width: %d",
            width);
      NavigateBarInitialized = -1;
      return;
   }

   NavigateBarBG =
      (RoadMapImage) roadmap_res_get
         (RES_BITMAP, RES_SKIN|RES_NOCACHE, NavigatePanel->image_file);

   NavigateBarImage =
      (RoadMapImage) roadmap_res_get
         (RES_BITMAP, RES_SKIN, NavigatePanel->image_file);

   if (!NavigateBarBG || !NavigateBarImage) goto error;

   roadmap_canvas_image_set_mutable (NavigateBarImage);

   for (i=0; i<LAST_DIRECTION; i++) {
      NavigateDirections[i] =
         (RoadMapImage) roadmap_res_get
               (RES_BITMAP, RES_SKIN, NAVIGATE_DIR_IMG[i]);

      if (!NavigateDirections[i]) goto error;
   }


   NavigateBarLocation.x = 0;
   NavigateBarLocation.y = height - roadmap_canvas_image_height(NavigateBarBG) - roadmap_bar_bottom_height();


   NavigateBarInitialized = 1;
   navigate_prev_after_refresh =
      roadmap_screen_subscribe_after_refresh (navigate_bar_after_refresh);

   return;

error:
   NavigateBarInitialized = -1;
}


void navigate_bar_set_instruction (enum NavigateInstr instr) {


   RoadMapGuiPoint pos0 = {0,0};
   RoadMapGuiPoint pos = NavigatePanel->instruction_pos;
   if (NavigateBarInitialized != 1) return;

   roadmap_canvas_copy_image (NavigateBarImage, &pos0, NULL, NavigateBarBG,
                              CANVAS_COPY_NORMAL);

   roadmap_canvas_copy_image (NavigateBarImage, &pos, NULL,
                              NavigateDirections[(int)instr],
                              CANVAS_COPY_BLEND);

   NavigateBarCurrentInstr = instr;

}


void navigate_bar_set_exit (int exit) {

	char text[3];

	if (exit < 0 || exit > 9) return;

	sprintf (text, "%d", exit);

	roadmap_canvas_create_pen ("nav_bar_pen");
  	roadmap_canvas_set_foreground(NAV_BAR_TEXT_COLOR_GREEN);

   roadmap_canvas_draw_image_text
      (NavigateBarImage, &NavigatePanel->exit_pos, 18, text);
}


void navigate_bar_set_distance (int distance) {

   char str[100];
   char unit_str[20];
   int  distance_far;
   RoadMapGuiPoint position = {0, 0};
   RoadMapPen nav_bar_pen;
   int font_size = 24;

   #ifdef IPHONE
   	font_size = 18;
   #endif

   if (NavigateBarInitialized != 1) return;

   NavigateBarCurrentDistance = distance;
   /* erase the old distance */
   roadmap_canvas_copy_image (NavigateBarImage, &position,
                              &NavigatePanel->distance_rect,
                              NavigateBarBG,
                              CANVAS_COPY_NORMAL);

   distance_far =
      roadmap_math_to_trip_distance(distance);

   if (distance_far > 0) {

      int tenths = roadmap_math_to_trip_distance_tenths(distance);
      if (distance_far < 100)
      	snprintf (str, sizeof(str), "%d.%d", distance_far, tenths % 10);
      else
      	snprintf (str, sizeof(str), "%d", distance_far);
      snprintf (unit_str, sizeof(unit_str), "%s", roadmap_lang_get(roadmap_math_trip_unit()));
   } else {
      if (roadmap_math_distance_to_current(distance) > 1000){
         int tenths = roadmap_math_to_trip_distance_tenths(distance);
         snprintf (str, sizeof(str), "0.%d", tenths % 10);
         snprintf (unit_str, sizeof(unit_str), "%s", roadmap_lang_get(roadmap_math_trip_unit()));
      }
      else{
         snprintf (str, sizeof(str), "%d", roadmap_math_distance_to_current(distance));
         snprintf (unit_str, sizeof(unit_str), "%s",
                  roadmap_lang_get(roadmap_math_distance_unit()));
      }
   }

	nav_bar_pen = roadmap_canvas_create_pen ("nav_bar_pen");
  	roadmap_canvas_set_foreground(NAV_BAR_TEXT_COLOR_GREEN);
  	if (ssd_widget_rtl(NULL)){
  	   position = NavigatePanel->distance_value_pos;
  	   roadmap_canvas_draw_image_text
         (NavigateBarImage, &position, font_size, str);

  	   position = NavigatePanel->distance_unit_pos;
  	   roadmap_canvas_draw_image_text
         (NavigateBarImage, &position, 12, unit_str);
  	}
  	else{
      position.y = NavigatePanel->distance_value_pos.y;
      position.x = NavigatePanel->distance_unit_pos.x;
      roadmap_canvas_draw_image_text
         (NavigateBarImage, &position, font_size, str);

      position.y = NavigatePanel->distance_unit_pos.y;
      position.x =NavigatePanel->distance_value_pos.x+24; // changed from 30 pixels to 24, by Dan.
      roadmap_canvas_draw_image_text
         (NavigateBarImage, &position, 12, unit_str);

  	}
}


void navigate_bar_set_street (const char *street) {

//#define NORMAL_SIZE 20
//#define SMALL_SIZE  16

   int width, ascent, descent;
   int size;
   char *line1 = NULL;
   char *line2 = NULL;
   char *text = NULL;
   int num_lines;
   int i;
   int font_size_normal = roadmap_config_get_integer( &RMConfigNavBarStreetNormalFont );
   int font_size_small = roadmap_config_get_integer( &RMConfigNavBarStreetSmalllFont );
   RoadMapPen nav_bar_pen;

   //street = " ???? ???? ?????? ????? ????? 18";
   if (NavigateBarInitialized != 1) return;

   if (street == NULL)
   	return;

   strncpy_safe (NavigateBarCurrentStreet, street, sizeof(NavigateBarCurrentStreet));

   text = strdup(street);

   size = font_size_normal;

   num_lines = navigate_bar_align_text (text, &line1, &line2, size);

   if ((num_lines < 0)  || ((num_lines>1) && (NavigatePanel->street_line2_pos == -1))){
      /* Try again with a smaller font size */
	   text = strdup(street);
      size = font_size_small;
      num_lines = navigate_bar_align_text (text, &line1, &line2, size);
   }

#ifdef IPHONE
   if (num_lines > 1)
      size -= 2;
#endif

   if ((num_lines < 0) || ((num_lines > 1) && (NavigatePanel->street_line2_pos == -1)))
      text = strdup(street);
   /* Cut some text until it fits */
   while ((num_lines < 0) || ((num_lines > 1) && (NavigatePanel->street_line2_pos == -1))) {

      char *end = text + strlen(text) - 1;
      while ((end > text) && (*end != ' ')) end--;
      if (end == text) {

         roadmap_log (ROADMAP_ERROR, "Can't align street in nav bar: %s",
                      street);

         free(text);
         return;
      }
      	*end = '\0';
      num_lines = navigate_bar_align_text (text, &line1, &line2, size);
   }


   for (i=0; i < num_lines; i++) {

      char *line;
      RoadMapGuiPoint position;

      position.x = NavigatePanel->street_start;
      position.y = NavigatePanel->street_line1_pos;

		if ((num_lines == 1) && (NavigatePanel->street_line2_pos != -1)){
			position.y = (NavigatePanel->street_line1_pos + NavigatePanel->street_line2_pos) / 2;
		}

      if (i ==0 ) {
         line = line1;
      } else {
         line = line2;
         position.y = NavigatePanel->street_line2_pos;
      }

      	roadmap_canvas_get_text_extents
         	(line, size, &width, &ascent, &descent, NULL);

      	if (ssd_widget_rtl(NULL))
      	   position.x = NavigatePanel->street_width - width + NavigatePanel->street_start;
      	else
      	   position.x = NavigatePanel->street_start;

   		nav_bar_pen = roadmap_canvas_create_pen ("nav_bar_pen");
  	   	roadmap_canvas_set_foreground(NAV_BAR_TEXT_COLOR);
      	roadmap_canvas_draw_image_text (NavigateBarImage, &position, size, line);
   }
   free(text);
}

void navigate_bar_set_mode (int mode) {
   if (NavigateBarEnabled == mode) return;
   NavigateBarEnabled = mode;
}


void navigate_bar_draw (void) {

   RoadMapGuiPoint draw_pos;
   if (NavigateBarInitialized != 1) return;

   draw_pos.x = NavigateBarLocation.x + NavigateBarLocOffsets.x;
   draw_pos.y = NavigateBarLocation.y + NavigateBarLocOffsets.y;
#ifdef TOUCH_SCREEN
   roadmap_screen_obj_offset( NAV_BAR_CLOSE_OBJ_NAME, NavigateBarLocOffsets.x, NavigateBarLocOffsets.y );
#endif

   roadmap_canvas_draw_image ( NavigateBarImage, &draw_pos, 0,
         IMAGE_NORMAL );
}

void navigate_bar_set_time_to_destination () {
   char text[256];
   RoadMapGuiPoint position = {0, 0};
   RoadMapPen nav_bar_pen;
   int font_size = roadmap_config_get_integer( &RMConfigNavBarDestTimeFont );


   if (NavigateBarInitialized != 1) return;

   if (!navigate_main_ETA_enabled())
      return;

   if (!roadmap_message_format (text, sizeof(text), "%A|%@"))
  		return;

  	nav_bar_pen = roadmap_canvas_create_pen ("nav_bar_pen");

  	roadmap_canvas_set_foreground(NAV_BAR_TIME_TO_DESTINATION_COLOR);

   /* erase the old distance */
   roadmap_canvas_copy_image (NavigateBarImage, &position,
                              &NavigatePanel->time_to_destination_rect,
                              NavigateBarBG,
                              CANVAS_COPY_NORMAL);



   roadmap_canvas_draw_image_text
      (NavigateBarImage, &NavigatePanel->time_to_destination_pos, font_size, text);


}

void navigate_bar_set_distance_to_destination () {

   char text[256];
   char distance[100];
   RoadMapGuiPoint position = {0, 0};
   RoadMapPen nav_bar_pen;
   char * pch;
   int width, ascent, descent;
   int units_offset = 0;
   int font_size = roadmap_config_get_integer( &RMConfigNavBarDestDistanceFont );

   if (NavigateBarInitialized != 1) return;

	if (!roadmap_message_format (distance, sizeof(distance), "%D (%W)|%D"))
		return;

	pch = strtok (distance," ,.");
   sprintf(text, "%s", pch);
	roadmap_canvas_get_text_extents
         (text, font_size, &width, &ascent, &descent, NULL);

   /* erase the old distance */
   roadmap_canvas_copy_image (NavigateBarImage, &position,
                              &NavigatePanel->distance_to_destination_rect,
                              NavigateBarBG,
                              CANVAS_COPY_NORMAL);


   nav_bar_pen = roadmap_canvas_create_pen ("nav_bar_pen");

  	roadmap_canvas_set_foreground(NAV_BAR_DIST_TO_DESTINATION_COLOR);

  	if (ssd_widget_rtl(NULL)){
  	   position = NavigatePanel->distance_to_destination_pos;
  	   roadmap_canvas_draw_image_text
         (NavigateBarImage, &position, font_size, text);

  	   pch = strtok (NULL," ,.");
  	   sprintf(text, "%s",pch);

  	    // We must separate the cases of the km and meters units representation on the bar
		if ( utf8_strlen( text ) < 3 )
			units_offset = 10;

		position = NavigatePanel->distance_to_destination_unit_pos;
		position.x += units_offset;
		position.y += 1;			// The font will be smaller - place the units a bit lower
		roadmap_canvas_draw_image_text( NavigateBarImage, &position, font_size-1, text );
  	}
  	else{
  	  position = NavigatePanel->distance_to_destination_unit_pos;
      position.x -= 5;
      roadmap_canvas_draw_image_text
         (NavigateBarImage, &position, font_size, text);

      pch = strtok (NULL," ,.");
      sprintf(text, "%s",pch);

      roadmap_canvas_draw_image_text
         (NavigateBarImage, &NavigatePanel->distance_to_destination_pos, font_size, text);
  	}

}

void navigate_bar_set_speed () {

   char str[100];
   char unit_str[100];
   RoadMapGuiPoint position = {0, 0};
   RoadMapGpsPosition pos;
   RoadMapPen nav_bar_pen;
   int speed;
   int font_size = 24;

   #ifdef IPHONE
   	font_size = 20;
   #endif

   if (NavigateBarInitialized != 1) return;

  roadmap_navigate_get_current (&pos, NULL, NULL);
  speed = pos.speed;

  if (speed == -1) return;

   /* erase the old distance */
   roadmap_canvas_copy_image (NavigateBarImage, &position,
                              &NavigatePanel->speed_rect,
                              NavigateBarBG,
                              CANVAS_COPY_NORMAL);

   snprintf (str, sizeof(str), "%3d", roadmap_math_to_speed_unit(speed));

   nav_bar_pen = roadmap_canvas_create_pen ("nav_bar_pen");
  	roadmap_canvas_set_foreground(NAV_BAR_SPEED_COLOR);

   roadmap_canvas_draw_image_text
      (NavigateBarImage, &NavigatePanel->speed_pos, font_size, str);

   snprintf (unit_str, sizeof(unit_str), "%s",  roadmap_lang_get(roadmap_math_speed_unit()));

   roadmap_canvas_draw_image_text
      (NavigateBarImage, &NavigatePanel->speed_unit_pos, 10, unit_str);

}

void navigate_bar_set_draw_offsets ( int offset_x, int offset_y )
{
	NavigateBarLocOffsets.x = offset_x;
	NavigateBarLocOffsets.y = offset_y;
}
