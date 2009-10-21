/* navigate_main.c - main navigate plugin file
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
 *   See navigate_main.h
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_pointer.h"
#include "roadmap_plugin.h"
#include "roadmap_line.h"
#include "roadmap_display.h"
#include "roadmap_message.h"
#include "roadmap_voice.h"
#include "roadmap_messagebox.h"
#include "roadmap_canvas.h"
#include "roadmap_street.h"
#include "roadmap_trip.h"
#include "roadmap_navigate.h"
#include "roadmap_screen.h"
#include "roadmap_line_route.h"
#include "roadmap_math.h"
#include "roadmap_point.h"
#include "roadmap_layer.h"
#include "roadmap_adjust.h"
#include "roadmap_lang.h"
#include "roadmap_sound.h"
#include "roadmap_locator.h"
#include "roadmap_config.h"
#include "roadmap_skin.h"
#include "roadmap_main.h"
#include "roadmap_square.h"
#include "roadmap_view.h"
#include "roadmap_softkeys.h"
#include "roadmap_tile.h"
#include "roadmap_search.h"
#include "roadmap_res.h"
#include "roadmap_tile_manager.h"
#include "roadmap_tile_status.h"

#ifdef SSD
#include "ssd/ssd_dialog.h"
#include "ssd/ssd_text.h"
#include "ssd/ssd_button.h"
#include "ssd/ssd_container.h"
#include "ssd/ssd_bitmap.h"
#include "ssd/ssd_generic_list_dialog.h"
#include "ssd/ssd_progress_msg_dialog.h"
#include "ssd/ssd_confirm_dialog.h"
#include "ssd/ssd_popup.h"
#else
#include "roadmap_dialog.h"
#endif

//FIXME remove when navigation will support plugin lines
#include "editor/editor_plugin.h"

#include "Realtime/Realtime.h"

#include "navigate_plugin.h"
#include "navigate_bar.h"
#include "navigate_instr.h"
#include "navigate_traffic.h"
#include "navigate_cost.h"
#include "navigate_route.h"
#include "navigate_zoom.h"
#include "navigate_route_trans.h"
#include "navigate_main.h"

#define ROUTE_PEN_WIDTH 5
//#define TEST_ROUTE_CALC 1
#define NAVIGATE_PREFETCH_DISTANCE 10000

#define MAX_MINUTES_TO_RESUME_NAV   120

extern const char NAVIGATE_DIR_IMG[][40];

static RoadMapConfigDescriptor NavigateConfigRouteColor =
                    ROADMAP_CONFIG_ITEM("Navigation", "RouteColor");

static RoadMapConfigDescriptor NavigateConfigAlt1RouteColor =
                    ROADMAP_CONFIG_ITEM("Navigation", "Alt1outeColor");

static RoadMapConfigDescriptor NavigateConfigAlt2RouteColor =
                    ROADMAP_CONFIG_ITEM("Navigation", "Alt2outeColor");

static RoadMapConfigDescriptor NavigateConfigAlt3RouteColor =
                    ROADMAP_CONFIG_ITEM("Navigation", "Alt3RouteColor");

static RoadMapConfigDescriptor NavigateConfigPossibleRouteColor =
                    ROADMAP_CONFIG_ITEM("Navigation", "PossibleRouteColor");

RoadMapConfigDescriptor NavigateConfigAutoZoom =
                  ROADMAP_CONFIG_ITEM("Routing", "Auto zoom");

RoadMapConfigDescriptor NavigateConfigNavigationGuidance =
                  ROADMAP_CONFIG_ITEM("Navigation", "Navigation guidance");

RoadMapConfigDescriptor NavigateConfigNavigationGuidanceEnabled =
                  ROADMAP_CONFIG_ITEM("Navigation", "Navigation guidance enabled");

RoadMapConfigDescriptor NavigateConfigEtaEnabled =
                  ROADMAP_CONFIG_ITEM("Navigation", "ETA enabled");

RoadMapConfigDescriptor NavigateConfigLastPos =
                  ROADMAP_CONFIG_ITEM("Navigation", "Last position");

RoadMapConfigDescriptor NavigateConfigNavigating =
                  ROADMAP_CONFIG_ITEM("Navigation", "Is navigating");

RoadMapConfigDescriptor NavigateConfigNavigateTime =
                  ROADMAP_CONFIG_ITEM("Navigation", "Navigate time");

static RoadMapConfigDescriptor ShowDisclaimerCfg =
                  ROADMAP_CONFIG_ITEM("Navigation", "Show disclaimer");

static void set_last_nav_time ();

int NavigateEnabled = 0;
int NavigatePluginID = -1;
static int NavigateTrackEnabled = 0;
static int NavigateDisplayALtRoute = 0;
static int NavigateAltId = 0;
static int NavigateTrackFollowGPS = 0;
static BOOL CalculatingRoute = FALSE;
static BOOL ReCalculatingRoute = FALSE;
static RoadMapPen NavigatePen[2];
static RoadMapPen NavigateAltPens[3][2];
static RoadMapPen NavigatePenEst[2];
static void navigate_update (RoadMapPosition *position, PluginLine *current);
static void navigate_get_next_line
          (PluginLine *current, int direction, PluginLine *next);

static int navigate_line_in_route (PluginLine *current, int direction);
static void navigate_progress_message_delayed(void);
static void navigate_progress_message_hide_delayed(void);

static RoadMapCallback NextMessageUpdate;

static int NavigateDistanceToDest;
static int NavigateETA;
static int NavigateDistanceToTurn;
static int NavigateDistanceToNext;
static int NavigateETAToTurn;
static int NavigateFlags;
static int NavigateETADiff;
static time_t NavigateETATime;
static time_t NavigateOfftrackTime = 0;
static time_t NavigateRerouteTime = 0;
static int NavigateLength;
static int NavigateTrackTime;

RoadMapNavigateRouteCB NavigateCallbacks = {
   &navigate_update,
   &navigate_get_next_line,
   &navigate_line_in_route
};


static NavigateSegment *NavigateSegments;
static int NavigateNumSegments = 0;
static int NavigateNumInstSegments = 0;
static int NavigateCurrentSegment = 0;
static int NavigateCurrentRequestSegment = 0;
static NavigateSegment *NavigateDetour;
static int NavigateDetourSize = 0;
static int NavigateDetourEnd = 0;
static PluginLine NavigateDestination = PLUGIN_LINE_NULL;
static int NavigateDestPoint;
static RoadMapPosition NavigateDestPos;
static char NavigateDestStreet[256] = {0};
static RoadMapPosition NavigateSrcPos;
static int NavigateNextAnnounce;
static int NavigateIsByServer = 0;
static int NavigatePendingSegment = -1;

static PluginLine NavigateFromLinePending = PLUGIN_LINE_NULL;
static PluginLine NavigateFromLineLast = PLUGIN_LINE_NULL;
static int NavigateFromPointPending = -1;
static int NavigateFromPointLast = -1;

static RoadMapPosition *NavigateOutlinePoints;
static int NavigateNumOutlinePoints = 0;

static RoadMapPosition *NavigateOriginalRoutePoints = NULL;
static int NavigateNumOriginalRoutePoints = 0;

static RoadMapCallback NavigateNextLoginCb = NULL;

static const char *ExitName[] = {
	"First", "Second", "Third", "Fourth", "Fifth", "Sixth", "Seventh"
};
#define MaxExitName ((int)(sizeof (ExitName) / sizeof (ExitName[0])))

static int navigate_num_segments (void) {

	return NavigateNumSegments - NavigateDetourEnd + NavigateDetourSize;
}

static NavigateSegment * navigate_segment (int i) {

	if (i < NavigateDetourSize) {
		return NavigateDetour + i;
	}
	return NavigateSegments + i - NavigateDetourSize + NavigateDetourEnd;
}


BOOL navigate_main_ETA_enabled(){

   if (roadmap_config_match(&NavigateConfigEtaEnabled, "yes"))
      return TRUE;
   else
      return FALSE;

}

BOOL navgiate_main_voice_guidance_enabled(){

   if (roadmap_config_match(&NavigateConfigNavigationGuidanceEnabled, "yes"))
      return TRUE;
   else
      return FALSE;
}

static int navigate_find_track_points (PluginLine *from_line, int *from_point,
                                     	PluginLine *to_line, int *to_point,
                                     	int *from_direction, int recalc_route) {

   const RoadMapPosition *position = NULL;
   RoadMapPosition from_position;
   RoadMapPosition to_position;
   PluginLine line;
   int distance;
   int from_tmp;
   int to_tmp;
   int direction = ROUTE_DIRECTION_NONE;

   *from_point = -1;


   if (NavigateTrackFollowGPS || recalc_route) {

      RoadMapGpsPosition pos;

#ifndef J2ME
      //FIXME remove when navigation will support plugin lines
      editor_plugin_set_override (0);
#endif

      if ((roadmap_navigate_get_current (&pos, &line, &direction) != -1) &&
          (roadmap_plugin_get_id(&line) == ROADMAP_PLUGIN_ID)) {

         roadmap_adjust_position (&pos, &NavigateSrcPos);

			roadmap_square_set_current (line.square);
         roadmap_line_points (line.line_id, &from_tmp, &to_tmp);

         if (direction == ROUTE_DIRECTION_WITH_LINE) {

            *from_point = to_tmp;
         } else {

            *from_point = from_tmp;
         }

      } else {

    	 if ( roadmap_gps_have_reception() )
    	 {
    		 position = roadmap_trip_get_position ( "GPS" );
    	 }
    	 else
    	 {
    		 position = roadmap_trip_get_position ( "Location" );
    	 }

         if (position) NavigateSrcPos = *position;
         direction = ROUTE_DIRECTION_NONE;
      }

#ifndef J2ME
      //FIXME remove when navigation will support plugin lines
      editor_plugin_set_override (1);
#endif

   } else {

      position = roadmap_trip_get_position ("Departure");
      NavigateSrcPos = *position;
   }

   if (*from_point == -1) {

      if (!position)
      {
         roadmap_messagebox("Error", "Current position is unknown");
         return -1;
      }

#ifndef J2ME
      //FIXME remove when navigation will support plugin lines
      editor_plugin_set_override (0);
#endif

      if ((roadmap_navigate_retrieve_line
               (position, 0, 200, &line, &distance, LAYER_ALL_ROADS) == -1) ||
            (roadmap_plugin_get_id (&line) != ROADMAP_PLUGIN_ID)) {

#ifndef J2ME
         //FIXME remove when navigation will support plugin lines
         editor_plugin_set_override (1);
#endif

			roadmap_log (ROADMAP_ERROR, "Failed to find a valid road near origin %d,%d", position->longitude, position->latitude);
         roadmap_messagebox
            ("Error", "Can't find a road near departure point.");

         return -1;
      }

#ifndef J2ME
      //FIXME remove when navigation will support plugin lines
      editor_plugin_set_override (1);
#endif

   }

   *from_line = line;

   if (direction == ROUTE_DIRECTION_NONE) {

		roadmap_square_set_current (line.square);
      switch (roadmap_plugin_get_direction (from_line, ROUTE_CAR_ALLOWED)) {
         case ROUTE_DIRECTION_ANY:
         case ROUTE_DIRECTION_NONE:
            roadmap_line_points (from_line->line_id, &from_tmp, &to_tmp);
            roadmap_point_position (from_tmp, &from_position);
            roadmap_point_position (to_tmp, &to_position);

            if (roadmap_math_distance (position, &from_position) <
                  roadmap_math_distance (position, &to_position)) {
               *from_point = from_tmp;
               direction = ROUTE_DIRECTION_AGAINST_LINE;
            } else {
               *from_point = to_tmp;
               direction = ROUTE_DIRECTION_WITH_LINE;
            }
            break;
         case ROUTE_DIRECTION_WITH_LINE:
            roadmap_line_points (from_line->line_id, &from_tmp, from_point);
            direction = ROUTE_DIRECTION_WITH_LINE;
            break;
         case ROUTE_DIRECTION_AGAINST_LINE:
            roadmap_line_points (from_line->line_id, from_point, &from_tmp);
            direction = ROUTE_DIRECTION_AGAINST_LINE;
            break;
         default:
            roadmap_line_points (from_line->line_id, &from_tmp, from_point);
            direction = ROUTE_DIRECTION_WITH_LINE;
      }
   }
   if (from_direction) *from_direction = direction;


   if (to_line == NULL ||
   	 to_line->plugin_id != INVALID_PLUGIN_ID) {
      /* we already calculated the destination point */
      return 0;
   }

   position = roadmap_trip_get_position ("Destination");
   if (!position) return -1;

   NavigateDestPos = *position;

#ifndef J2ME
   //FIXME remove when navigation will support plugin lines
   editor_plugin_set_override (0);
#endif

   if ((roadmap_navigate_retrieve_line
            (position, 0, 50, &line, &distance, LAYER_ALL_ROADS) == -1) ||
         (roadmap_plugin_get_id (&line) != ROADMAP_PLUGIN_ID)) {

      //roadmap_messagebox ("Error", "Can't find a road near destination point.");
		roadmap_log (ROADMAP_WARNING, "Failed to find a valid road near destination %d,%d", position->longitude, position->latitude);
      to_line->plugin_id = -1;
      *to_point = 0;

#ifndef J2ME
      //FIXME remove when navigation will support plugin lines
      editor_plugin_set_override (1);
#endif

      return 0;
   }

#ifndef J2ME
   //FIXME remove when navigation will support plugin lines
   editor_plugin_set_override (1);
#endif
   *to_line = line;

   switch (roadmap_plugin_get_direction (to_line, ROUTE_CAR_ALLOWED)) {
      case ROUTE_DIRECTION_ANY:
      case ROUTE_DIRECTION_NONE:
         roadmap_line_points (to_line->line_id, &from_tmp, &to_tmp);
         roadmap_point_position (from_tmp, &from_position);
         roadmap_point_position (to_tmp, &to_position);

         if (roadmap_math_distance (position, &from_position) <
             roadmap_math_distance (position, &to_position)) {
            *to_point = from_tmp;
         } else {
            *to_point = to_tmp;
         }
         break;
      case ROUTE_DIRECTION_WITH_LINE:
         roadmap_line_points (to_line->line_id, to_point, &to_tmp);
         break;
      case ROUTE_DIRECTION_AGAINST_LINE:
         roadmap_line_points (to_line->line_id, &to_tmp, to_point);
         break;
      default:
         roadmap_line_points (to_line->line_id, &to_tmp, to_point);
   }

   return 0;
}


static int navigate_find_track_points_in_scale (PluginLine *from_line, int *from_point,
                                       			PluginLine *to_line, int *to_point,
                                       			int *from_direction, int recalc_route, int scale) {

	int prev_scale = roadmap_square_get_screen_scale ();
	int rc;

	roadmap_square_set_screen_scale (scale);
	rc = navigate_find_track_points (from_line, from_point, to_line, to_point, from_direction, recalc_route);
	roadmap_square_set_screen_scale (prev_scale);

	return rc;
}


static void navigate_main_suspend_navigation()
{
   if( !NavigateTrackEnabled)
      return;


	NavigatePendingSegment = -1;
   NavigateTrackEnabled = 0;
   navigate_bar_set_mode( NavigateTrackEnabled);
   roadmap_navigate_end_route();
}


static void refresh_eta (BOOL initial) {

	/* recalculate ETA according to possibly changing traffic info */
   int prev_eta = NavigateETA + NavigateETAToTurn;
   int num_segments;
   int i = NavigateCurrentSegment;
   NavigateSegment *segment;
   int group_id;

	if (!NavigateTrackEnabled) {
		return;
	}

	if (initial) {
		prev_eta = 0;
		NavigateETADiff = 0;
	}

	if (NavigateNumInstSegments < NavigateNumSegments)
		return;

   num_segments = navigate_num_segments ();
   if (NavigateCurrentSegment >= num_segments)
   	return;

   segment = navigate_segment (i);
   group_id = segment->group_id;

	if (!NavigateIsByServer) {
		navigate_instr_calc_cross_time (segment, num_segments - i);
	}

	/* ETA to end of current segment */
   NavigateETAToTurn = (int) (1.0 * segment->cross_time * NavigateDistanceToNext /
                             (segment->distance + 1));

	/* ETA to next turn */
   while (++i < num_segments) {
   	segment = navigate_segment (i);
      if (segment->group_id != group_id) break;
      NavigateETAToTurn += segment->cross_time;
   }

	/* ETA from next turn to destination */
   NavigateETA = 0;
   while (i < num_segments) {

      NavigateETA            += segment->cross_time;
      segment = navigate_segment (++i);
   }

	if (prev_eta) {
		NavigateETADiff += NavigateETA + NavigateETAToTurn - prev_eta;
	}


	if ((NavigateETADiff < -180 ||
		 NavigateETADiff > 180)  && !initial){

		char msg[1000];

		if (NavigateETADiff > 0) {
			snprintf (msg, sizeof (msg), "%s %d %s.",
						 roadmap_lang_get ("Due to new traffic information, ETA is longer by"),
						 (NavigateETADiff + 30) / 60,
						 roadmap_lang_get ("minutes"));
		} else {
			snprintf (msg, sizeof (msg), "%s %d %s.",
						 roadmap_lang_get ("Due to new traffic information, ETA is shorter by"),
						 (-NavigateETADiff + 30) / 60,
						 roadmap_lang_get ("minutes"));
		}
      roadmap_messagebox_timeout ("Route information", msg, 7);
		roadmap_log (ROADMAP_DEBUG, "Major ETA change!! (%+d seconds)", NavigateETADiff);
		NavigateETADiff = 0;
	}

	NavigateETATime = time(NULL);
}


static void navigate_get_plugin_line (PluginLine *line, const NavigateSegment *segment) {

	line->fips = roadmap_locator_active ();
	line->plugin_id = ROADMAP_PLUGIN_ID;
	line->square = segment->square;
	line->line_id = segment->line;
	line->cfcc = segment->cfcc;
}


static void navigate_display_street (int isegment) {

	PluginLine					segment_line;
   PluginStreetProperties	properties;
   NavigateSegment			*segment;
   int							num_segments = navigate_num_segments ();

	// skip empty street names
	while (isegment < num_segments) {
		segment = navigate_segment (isegment);
		if (!segment->is_instrumented) break;
      navigate_get_plugin_line (&segment_line, segment);
      roadmap_plugin_get_street_properties (&segment_line, &properties, 0);
      if (properties.street && properties.street[0]) break;
      isegment++;
	}

	if (isegment >= num_segments) return;

   if (segment->is_instrumented) {
      navigate_bar_set_street (properties.street);
      NavigatePendingSegment = -1;
   } else if (NavigatePendingSegment != isegment && isegment < num_segments) {
   	navigate_bar_set_street ("");
   	NavigatePendingSegment = isegment;
   	roadmap_tile_request (segment->square, ROADMAP_TILE_STATUS_PRIORITY_NEXT_TURN, 1, NULL);
   }
}


static void navigate_main_format_messages (void) {

   int distance_to_destination;
   int distance_to_destination_far;
   int ETA;
   char str[100];
   RoadMapGpsPosition pos;

   (*NextMessageUpdate) ();

   if (!NavigateTrackEnabled) return;

   distance_to_destination = NavigateDistanceToDest + NavigateDistanceToTurn;
   ETA = NavigateETA + NavigateETAToTurn + 60;

   distance_to_destination_far =
      roadmap_math_to_trip_distance(distance_to_destination);

   if (distance_to_destination_far > 0) {
      roadmap_message_set ('D', "%d %s",
            distance_to_destination_far,
            roadmap_lang_get(roadmap_math_trip_unit()));
   } else {
      roadmap_message_set ('D', "%d %s",
            roadmap_math_distance_to_current(distance_to_destination),
            roadmap_lang_get(roadmap_math_distance_unit()));
   };

   sprintf (str, "%d:%02d", ETA / 3600, (ETA % 3600) / 60);
   roadmap_message_set ('T', str); // 1:25

   if (	 ETA>3600 ) // hours > 0
   		sprintf (str, "%d %s %02d %s", ETA/3600, roadmap_lang_get("hr."), (ETA % 3600)/60, roadmap_lang_get("min."));
   else
   		sprintf (str, "%d %s", (ETA % 3600)/60, roadmap_lang_get("min."));

   roadmap_message_set ('@', str); // 1 hr. 25 min.

   roadmap_navigate_get_current (&pos, NULL, NULL);
   roadmap_message_set ('S', "%3d %s",
         roadmap_math_to_speed_unit(pos.speed),
         roadmap_lang_get(roadmap_math_speed_unit()));

}


static void navigate_copy_points (void) {

	if (NavigateNumOutlinePoints > 0) {
		NavigateNumOriginalRoutePoints = NavigateNumOutlinePoints;
		NavigateOriginalRoutePoints = malloc (NavigateNumOriginalRoutePoints * sizeof (RoadMapPosition));
		memcpy (NavigateOriginalRoutePoints, NavigateOutlinePoints, NavigateNumOriginalRoutePoints * sizeof (RoadMapPosition));
  		NavigateNumOutlinePoints = 0;
	}
}


static void navigate_free_points (void) {

	if (NavigateOriginalRoutePoints) {
		free (NavigateOriginalRoutePoints);
		NavigateOriginalRoutePoints = NULL;
	}
	NavigateNumOriginalRoutePoints = 0;
}


static void navigate_show_message (void) {

   char msg[200] = {0};
   int trip_distance;

   if (NavigateFlags & CHANGED_DESTINATION) {
      snprintf(msg + strlen(msg), sizeof(msg) - strlen(msg), "%s\n",
            roadmap_lang_get ("Unable to provide route to destination. Taking you to nearest location."));
   }
   if (NavigateFlags & CHANGED_DEPARTURE) {
      snprintf(msg + strlen(msg), sizeof(msg) - strlen(msg), "%s\n",
            roadmap_lang_get ("Showing route using alternative departure point."));
   }

	trip_distance = roadmap_math_to_trip_distance_tenths(NavigateLength);
	if (navigate_main_ETA_enabled())
	   snprintf(msg + strlen(msg), sizeof(msg) - strlen(msg),
	            "%.1f %s,\n%.1f %s",
	            trip_distance/10.0,
	            roadmap_lang_get(roadmap_math_trip_unit()),
	            NavigateTrackTime/60.0,
	            roadmap_lang_get ("minutes"));
	else
      snprintf(msg + strlen(msg), sizeof(msg) - strlen(msg),
               "%.1f %s",
               trip_distance/10.0,
               roadmap_lang_get(roadmap_math_trip_unit()));

   ssd_progress_msg_dialog_hide();
   roadmap_messagebox_timeout ("Route found", msg, 5);
   focus_on_me();
}

static void close_disclaimer (void) {

   roadmap_main_remove_periodic (close_disclaimer);
   ssd_dialog_hide("navigate_disclaimer", dec_close);
   if (!roadmap_screen_refresh())
      roadmap_screen_redraw();
}

static int navigate_disclaimer_callback (SsdWidget widget, const char *new_value)
{

    if (!strcmp(widget->name, "OK") || !strcmp(widget->name, "Cancel"))
    {

        ssd_dialog_hide_current(dec_ok);

        if (!strcmp(widget->name, "OK"))
        {

        }
        else
        {
        		navigate_main_stop_navigation ();
        		//navigate_progress_message_hide_delayed ();
        		roadmap_screen_redraw ();
        }
        return 1;

    }


    close_disclaimer();
    return 1;
}



static void navigate_show_disclaimer (void) {

    const char *description;
    SsdWidget text;
    char msg[300] = {0};
    int trip_distance;
    SsdWidget widget_title;

    SsdWidget dialog = ssd_dialog_new("navigate_disclaimer",
            roadmap_lang_get("Route found"),
            NULL,
            SSD_CONTAINER_BORDER|SSD_CONTAINER_TITLE|SSD_DIALOG_FLOAT|
            SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER|SSD_ROUNDED_CORNERS);

    ssd_widget_add(dialog, ssd_container_new("spacer1", NULL, 0, 5,   SSD_END_ROW));
    if (NavigateFlags & CHANGED_DESTINATION) {
        snprintf(msg + strlen(msg), sizeof(msg) - strlen(msg), "%s\n",
              roadmap_lang_get ("Unable to provide route to destination. Taking you to nearest location."));
    }
    if (NavigateFlags & CHANGED_DEPARTURE) {
        snprintf(msg + strlen(msg), sizeof(msg) - strlen(msg), "%s\n",
              roadmap_lang_get ("Showing route using alternative departure point."));
    }
    text = ssd_text_new("text", msg, 16, SSD_END_ROW|SSD_WIDGET_SPACE);

    ssd_widget_add(dialog, text);

    trip_distance = roadmap_math_to_trip_distance_tenths(NavigateLength);
    if (navigate_main_ETA_enabled())
        snprintf(msg + strlen(msg), sizeof(msg) - strlen(msg),
                 "%.1f %s.\n%.1f %s",
                 trip_distance/10.0,
                 roadmap_lang_get(roadmap_math_trip_unit()),
                 NavigateTrackTime/60.0,
                 roadmap_lang_get ("minutes"));
    else
        snprintf(msg + strlen(msg), sizeof(msg) - strlen(msg),
                 "%.1f %s",
                 trip_distance/10.0,
                 roadmap_lang_get(roadmap_math_trip_unit()));

     ssd_progress_msg_dialog_hide();


	text = ssd_text_new("text", msg, 18, SSD_END_ROW|SSD_WIDGET_SPACE|SSD_ALIGN_CENTER);

   ssd_widget_add(dialog, text);

   ssd_widget_add(dialog, ssd_container_new("spacer1", NULL, 0, 5,   SSD_END_ROW));

   description = roadmap_lang_get("\nNote: route may not be optimal, but waze learns quickly...");
   text = ssd_text_new("text", description, 14, SSD_END_ROW|SSD_WIDGET_SPACE|SSD_START_NEW_ROW);
   ssd_widget_add(dialog, text);

   widget_title = ssd_widget_get( dialog, "title_text" );
   ssd_text_set_font_size( widget_title, 24 );

   /* Spacer */
   ssd_widget_add(dialog, ssd_container_new("spacer1", NULL, 0, 20,   SSD_END_ROW));

   ssd_widget_add(dialog, ssd_button_label("OK",
            roadmap_lang_get("Go"),
            SSD_WS_TABSTOP|SSD_ALIGN_CENTER, navigate_disclaimer_callback));

//   ssd_widget_add(dialog, ssd_button_label("Cancel",
//            roadmap_lang_get("Teach Waze"),
//            SSD_WS_TABSTOP|SSD_ALIGN_CENTER, navigate_disclaimer_callback));

   roadmap_main_set_periodic (5 * 1000, close_disclaimer);
   ssd_dialog_activate("navigate_disclaimer", NULL);
}

static BOOL show_disclaimer(){
   if (roadmap_config_match(&ShowDisclaimerCfg, "yes"))
      return TRUE;
   else
      return FALSE;
}

void navigate_main_on_route (int flags, int length, int track_time,
									  NavigateSegment *segments, int num_segment, int num_instrumented,
									  RoadMapPosition *outline_points, int num_outline_points) {

   int gps_state;
   BOOL gps_active;
   RoadMapGpsPosition position;
   PluginLine from_line;
   PluginLine next_line;
   int from_direction;
   int from_point;

	NavigateFromLineLast = NavigateFromLinePending;
	NavigateFromPointLast = NavigateFromPointPending;

	NavigateRerouteTime = 0;

	NavigateSegments = segments;
	NavigateNumSegments = num_segment;
	NavigateNumInstSegments = num_instrumented;
	NavigateDetourSize = 0;
	NavigateDetourEnd = 0;
   NavigateCurrentSegment = 0;
   NavigateCurrentRequestSegment = 0;
	roadmap_log (ROADMAP_DEBUG, "NavigateCurrentSegment = %d", NavigateCurrentSegment);

	navigate_free_points ();
	NavigateOutlinePoints = outline_points;
	NavigateNumOutlinePoints = num_outline_points;

   navigate_bar_initialize ();

#ifdef SSD
   ssd_dialog_hide ("Route calc", dec_close);
#else
   roadmap_dialog_hide ("Route calc");
#endif
   NavigateFlags = flags;
   NavigateLength = length;
   NavigateTrackTime = track_time;

   NavigateTrackEnabled = 1;
   NavigateDisplayALtRoute = 0;
   navigate_bar_set_mode (NavigateTrackEnabled);

	if (roadmap_navigate_get_current (&position, &from_line, &from_direction) == 0) {
		navigate_get_next_line (&from_line, from_direction, &next_line);
		navigate_update ((RoadMapPosition *)&position, &from_line);
	} else if (navigate_find_track_points_in_scale (&from_line, &from_point, NULL, NULL, &from_direction, 0, 0) == 0) {
		navigate_get_next_line (&from_line, from_direction, &next_line);
	}

   if (NavigateTrackFollowGPS) {
      roadmap_trip_stop ();
      roadmap_navigate_route (NavigateCallbacks);
   }

   gps_state = roadmap_gps_reception_state();
   gps_active = (gps_state != GPS_RECEPTION_NA) && (gps_state != GPS_RECEPTION_NONE);

   if (!gps_active) {
      NavigateDistanceToDest = length;
   } else {
      NavigateDistanceToDest = length - NavigateDistanceToTurn ;
   }
   NavigateDistanceToTurn = 0;
   NavigateDistanceToNext = 0;
   NavigateETAToTurn = 0;
   refresh_eta(TRUE);
   navigate_main_format_messages();
   //navigate_bar_set_time_to_destination();
   roadmap_screen_redraw ();
   roadmap_config_set_position (&NavigateConfigLastPos, &NavigateDestPos);
   roadmap_config_set_integer (&NavigateConfigNavigating, 1);
   roadmap_config_save (0);

	if (!(flags & RECALC_ROUTE)) {

	   //if (flags & GRAPH_IGNORE_TURNS) {
		if (navigate_cost_allow_unknowns () && show_disclaimer()) {
	      navigate_show_disclaimer ();
	   } else {
	   	navigate_show_message ();
	   }
	}
}


void navigate_main_update_route (int num_instrumented) {

	if (NavigateEnabled) {
		NavigateNumInstSegments = num_instrumented;
		if (NavigatePendingSegment != -1) {
			navigate_display_street (NavigatePendingSegment);
		}
		roadmap_screen_redraw ();
	}
}


static void navigate_main_on_segments (NavigateRouteRC rc, const NavigateRouteResult *res, const NavigateRouteSegments *segments) {

	CalculatingRoute = FALSE;
	ReCalculatingRoute = FALSE;

	navigate_progress_message_hide_delayed ();

	if (rc != route_succeeded) {

		switch (rc) {
			case route_server_error:
				// message already displayed
				break;
			case route_inconsistent:
			default:
			   roadmap_log (ROADMAP_ERROR, "The service failed to provide a valid route rc=%d", rc);
				roadmap_messagebox ("Error", "The service failed to provide a valid route (This may be a bug).");
		}

		return;
	}

	if (res->route_status == ROUTE_ORIGINAL) {
		navigate_main_on_route (res->flags, res->total_length, res->total_time, segments->segments,
										  segments->num_segments, segments->num_instrumented,
										  res->geometry.points, res->geometry.num_points);
      NavigateOfftrackTime = 0;
	}

	else if (res->route_status == ROUTE_UPDATE) {
		refresh_eta (TRUE);
		if (navigate_main_ETA_enabled())
		   roadmap_messagebox_timeout ("ETA Update", "Due to change in traffic conditions ETA was updated", 5);
	}

	// add support for new alternative here...
}


static int navigate_main_recalc_route () {

   int track_time = -1;
   PluginLine from_line;
   int from_point;
   int flags;
   int num_new;
	int num_total;
   time_t timeNow = time(NULL);

	if (NavigateRerouteTime > time (NULL) - 60) {
		return -1;
	}

	if (NavigateOfftrackTime == 0) {
		NavigateOfftrackTime = timeNow;
	}

   if (navigate_route_load_data () < 0) {
      return -1;
   }

   if (navigate_find_track_points_in_scale
         (&from_line, &from_point,
          &NavigateDestination, &NavigateDestPoint, NULL, 1, 0) < 0) {

      return -1;
   }

	if (roadmap_plugin_same_line(&from_line, &NavigateFromLineLast) &&
		 from_point == NavigateFromPointLast) {

		roadmap_log (ROADMAP_WARNING, "Trying to recalc from same line at %d/%d", from_line.square, from_line.line_id);
		return -1;
	}

	/* TODO: Remove the comment to not recalculate route until you get on it for the first time
 	if ((NavigateFlags & CHANGED_DEPARTURE) && !NavigateIsByServer) {
   	return -1;
   }*/

   navigate_main_suspend_navigation ();

   NavigateFromLinePending = from_line;
	NavigateFromPointPending = from_point;

   roadmap_main_set_cursor (ROADMAP_CURSOR_WAIT);

   flags = (NavigateFlags | RECALC_ROUTE) /*& ~ALLOW_ALTERNATE_SOURCE*/;

   if (NavigateIsByServer &&
       timeNow < NavigateOfftrackTime + 60 &&
       !RealTimeLoginState ()) {
       flags = (flags | USE_LAST_RESULTS) & ~ALLOW_DESTINATION_CHANGE;

	   navigate_cost_reset ();
   	roadmap_log (ROADMAP_INFO, "Calculating short reroute..");
	   track_time =
	      navigate_route_get_segments
	            (&from_line, from_point, &NavigateDestination, &NavigateDestPoint,
	             &NavigateDetour, &num_total, &num_new,
	             &flags, NavigateSegments, NavigateNumSegments);
	   flags = (flags & ~USE_LAST_RESULTS) | ALLOW_DESTINATION_CHANGE;

	   if (track_time > 0) {
	   	NavigateDetourEnd = NavigateNumSegments - (num_total - num_new);
	   	NavigateDetourSize = num_new;
	   }

   }


	if (track_time < 0) {
	   if (RealTimeLoginState ()) {
	   	NavigateIsByServer = 1;
   		roadmap_log (ROADMAP_INFO, "Requesting reroute..");
   		navigate_copy_points ();
	   	ReCalculatingRoute = TRUE;
	   	roadmap_main_set_periodic( 300, navigate_progress_message_delayed );
			roadmap_main_set_periodic( 30000, navigate_progress_message_hide_delayed );
	   	navigate_route_request (&from_line, from_point, &NavigateDestination,
	   									&NavigateSrcPos, &NavigateDestPos, NavigateDestStreet, flags,
	   									-1, 1, NULL, navigate_main_on_segments, navigate_main_update_route);
   		roadmap_navigate_resume_route ();
	   	NavigateRerouteTime = time (NULL);
   		roadmap_main_set_cursor (ROADMAP_CURSOR_NORMAL);
	   	return -1;

	   } else {
	   	NavigateIsByServer = 0;
   		NavigateNumOutlinePoints = 0;
		   navigate_cost_reset ();
   		roadmap_log (ROADMAP_INFO, "Calculating long reroute..");
		   track_time =
		      navigate_route_get_segments
		            (&from_line, from_point, &NavigateDestination, &NavigateDestPoint,
		             &NavigateSegments, &NavigateNumSegments, &num_new,
		             &flags, NavigateSegments, NavigateNumSegments);
	   }
	}

   roadmap_main_set_cursor (ROADMAP_CURSOR_NORMAL);
   roadmap_navigate_resume_route ();

   if (track_time <= 0) {
      return -1;
   }

   navigate_bar_initialize ();

   NavigateFlags = flags;

   navigate_instr_prepare_segments (navigate_segment, navigate_num_segments (), num_new,
                                   &NavigateSrcPos, &NavigateDestPos);
	//NavigateNumInstSegments = NavigateNumSegments;
   NavigateTrackEnabled = 1;
   navigate_bar_set_mode (NavigateTrackEnabled);
   NavigateCurrentSegment = 0;
   NavigateCurrentRequestSegment = 0;
	roadmap_log (ROADMAP_DEBUG, "NavigateCurrentSegment = %d", NavigateCurrentSegment);
   return 0;
}

static int navigate_address_cb (const RoadMapPosition *point,
                                address_info_ptr       ai) {

   roadmap_trip_set_point ("Destination", point);

   strncpy_safe (NavigateDestStreet, ai->street, sizeof(NavigateDestStreet));

   if( -1 == navigate_main_calc_route ())
      return -1;

   // Navigation started, send realtime message
   Realtime_ReportOnNavigation(point, ai);

   return 0;
}

int main_navigator(  const RoadMapPosition *point,
                     address_info_ptr       ai)
{ return navigate_address_cb( point, ai);}


/****** Route calculation progress dialog ******/
#ifdef SSD

static void show_progress_dialog (void) {

   SsdWidget dialog = ssd_dialog_activate ("Route calc", NULL);

   if (!dialog) {
      SsdWidget group;
	  SsdWidget text;

      dialog = ssd_dialog_new ("Route calc",
            roadmap_lang_get("Route calculation"), NULL,
            SSD_CONTAINER_BORDER|SSD_CONTAINER_TITLE|SSD_DIALOG_FLOAT|
            SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER|SSD_ROUNDED_CORNERS);

      group = ssd_container_new ("Progress group", NULL,
                  SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_WIDGET_SPACE|SSD_END_ROW);
      ssd_widget_set_color (group, NULL, NULL);
      text = ssd_text_new ("Label",
            				roadmap_lang_get("Calculating route, please wait..."), -1,
                        	SSD_END_ROW);
      ssd_widget_add (group, text);

	  text = ssd_text_new ("Label", "%", -1, 0);
      ssd_widget_add (group,text);

	  text = ssd_text_new ("Progress", "", -1, 0);

      ssd_widget_add (group, text);
      ssd_widget_add (dialog, group);

      ssd_dialog_activate ("Route calc", NULL);
      ssd_dialog_draw ();
   }

   ssd_widget_set_value (dialog, "Progress", "0");

   ssd_dialog_draw ();
}

#else

static void cancel_calc (const char *name, void *data) {
}

static void show_progress_dialog (void) {

   if (roadmap_dialog_activate ("Route calc", NULL, 1)) {

      roadmap_dialog_new_label ("Calculating", "Calculating route, please wait...");
      roadmap_dialog_new_progress ("Calculating", "Progress");

      roadmap_dialog_add_button ("Cancel", cancel_calc);

      roadmap_dialog_complete (0);
   }

   roadmap_dialog_set_progress ("Calculating", "Progress", 0);

   roadmap_main_flush ();
}
#endif


int navigate_line_in_route
          (PluginLine *line, int direction) {
   int count = 5;
   int isegment;
   PluginLine segment_line;

   if (!NavigateTrackEnabled) return 0;

	for (isegment = NavigateCurrentSegment; isegment < NavigateNumSegments; isegment++) {

	   const NavigateSegment *segment = navigate_segment (isegment);
		navigate_get_plugin_line (&segment_line, segment);
      if ((direction == segment->line_direction) &&
            roadmap_plugin_same_line(&segment_line, line))
         return 1;
      segment++;
      count--;
   }

   return 0;
}

void navigate_update (RoadMapPosition *position, PluginLine *current) {

   int announce = 0;
   int num_segments;
   const NavigateSegment *segment = NULL;
   const NavigateSegment *next_segment = NULL;
   const NavigateSegment *prev_segment = NULL;
   int i;
   int group_id;
   const char *inst_text = "";
   const char *inst_voice = NULL;
   const char *inst_roundabout = NULL;
   int roundabout_exit = 0;
   RoadMapSoundList sound_list;
   const int ANNOUNCES[] = { 800, 200, 40 };
#ifdef __SYMBIAN32__
   const int ANNOUNCE_PREPARE_FACTORS[] = { 400, 400, 150 };
#else
   const int ANNOUNCE_PREPARE_FACTORS[] = { 200, 200, 100 };
#endif
   int announce_distance = 0;
   int distance_to_prev;
   int distance_to_next;
   RoadMapGpsPosition pos;

	//printf ("navigate_update(): current is %d/%d\n", current->square, current->line_id);

#ifdef J2ME
#define NAVIGATE_COMPENSATE 20
#else
#define NAVIGATE_COMPENSATE 10
#endif


#ifdef TEST_ROUTE_CALC
   static int test_counter;

   if ((++test_counter % 300) == 0) {
      navigate_main_test(1);
      return;
   }
#endif

	//printf ("navigate_update (%d.%d, %d/%d)\n",
	//			position->longitude, position->latitude,
	//			current->square, current->line_id);

   if (!NavigateTrackEnabled) return;

   num_segments = navigate_num_segments ();
   segment = navigate_segment (NavigateCurrentSegment);
   group_id = segment->group_id;

	if (!segment->is_instrumented) {

		roadmap_square_set_current (segment->square);
		NavigateDistanceToNext = roadmap_line_length (segment->line);
	} else {
	   if (segment->line_direction == ROUTE_DIRECTION_WITH_LINE) {

	      NavigateDistanceToNext =
	         navigate_instr_calc_length (position, segment, LINE_END);
	   } else {

	      NavigateDistanceToNext =
	         navigate_instr_calc_length (position, segment, LINE_START);
	   }
	}

   distance_to_prev = segment->distance - NavigateDistanceToNext;
   for (i = NavigateCurrentSegment - 1; i >= 0; i--) {
   	prev_segment = navigate_segment (i);
   	if (prev_segment->group_id != group_id) break;
   	distance_to_prev += prev_segment->distance;
   }

   NavigateETAToTurn = (int) (1.0 * segment->cross_time * NavigateDistanceToNext /
                             (segment->distance + 1));

	NavigateDistanceToTurn = NavigateDistanceToNext;
	for (i = NavigateCurrentSegment + 1; i < num_segments; i++) {
   	next_segment = navigate_segment (i);
      if (next_segment->group_id != group_id) break;
      segment = next_segment;
      NavigateDistanceToTurn += segment->distance;
      NavigateETAToTurn += segment->cross_time;
   }
   if (NavigateETATime + 60 <= time(NULL) && !NavigateIsByServer) {
   	refresh_eta (FALSE);
   }

   //printf ("next in %d turn in %d eta %d\n", NavigateDistanceToNext, NavigateDistanceToTurn, NavigateETAToTurn);

	//printf ("NavigateCurrentSegment = %d, NavigateDistanceToNext = %d, NavigateDistanceToTurn = %d\n", NavigateCurrentSegment, NavigateDistanceToNext, NavigateDistanceToTurn);
   distance_to_next = 0;

   if (i < num_segments) {
      group_id = next_segment->group_id;
      distance_to_next = next_segment->distance;
      while (++i < num_segments) {
      	next_segment = navigate_segment (i);
         if (next_segment->group_id != group_id) break;
         distance_to_next += next_segment->distance;
      }
   }

   if (roadmap_config_match(&NavigateConfigAutoZoom, "yes")) {
      const char *focus = roadmap_trip_get_focus_name ();

      if (focus && !strcmp (focus, "GPS")) {

         navigate_zoom_update (NavigateDistanceToTurn,
                               distance_to_prev,
                               distance_to_next);
      }
   }

   navigate_bar_set_distance (NavigateDistanceToTurn);

   switch (segment->instruction) {

      case TURN_LEFT:
         inst_text = "Turn left";
         inst_voice = "TurnLeft";
         break;
      case ROUNDABOUT_LEFT:
         inst_text = "At the roundabout, turn left";
         inst_roundabout = "Roundabout";
         inst_voice = "TurnLeft";
         break;
      case KEEP_LEFT:
         inst_text = "Keep left";
         inst_voice = "KeepLeft";
         break;
      case TURN_RIGHT:
         inst_text = "Turn right";
         inst_voice = "TurnRight";
         break;
      case ROUNDABOUT_RIGHT:
         inst_text = "At the roundabout, turn right";
         inst_roundabout = "Roundabout";
         inst_voice = "TurnRight";
         break;
      case KEEP_RIGHT:
         inst_text = "Keep right";
         inst_voice = "KeepRight";
         break;
      case APPROACHING_DESTINATION:
         inst_text = "Approaching destination";
         break;
      case CONTINUE:
         inst_text = "continue straight";
         inst_voice = "Straight";
         break;
      case ROUNDABOUT_STRAIGHT:
         inst_text = "At the roundabout, continue straight";
         inst_roundabout = "Roundabout";
         inst_voice = "Straight";
         break;
      case ROUNDABOUT_ENTER:
      {
		 inst_text = "At the roundabout, exit";
		 inst_roundabout = "Roundabout";
		 roundabout_exit = segment->exit_no;

    	 if ( roadmap_lang_rtl() )
    	 {
			 inst_voice = "Exit";
    	 }
    	 else
    	 {
			 inst_voice = "";
    	 }
         break;
      }
      default:
         break;
   }

   roadmap_navigate_get_current (&pos, NULL, NULL);

   if ((segment->instruction == APPROACHING_DESTINATION) &&
        NavigateDistanceToTurn <= 20 + pos.speed * 1) {

      sound_list = roadmap_sound_list_create (0);
      roadmap_sound_list_add (sound_list, "Arrive");
      if (navgiate_main_voice_guidance_enabled() && roadmap_config_match(&NavigateConfigNavigationGuidance, "yes")) {
      	roadmap_sound_play_list (sound_list);
      }

      if (roadmap_config_match(&NavigateConfigAutoZoom, "yes")) {
         const char *focus = roadmap_trip_get_focus_name ();
         /* We used auto zoom, so now we need to reset it */

         if (focus && !strcmp (focus, "GPS")) {

            roadmap_screen_zoom_reset ();
         }
      }

      navigate_main_stop_navigation ();
      return;
   }

   roadmap_message_set ('I', inst_text);

   if (NavigateNextAnnounce == -1) {

      unsigned int i;

      for (i=0; i<sizeof(ANNOUNCES)/sizeof(ANNOUNCES[0]) - 1; i++) {

         if (NavigateDistanceToTurn > ANNOUNCES[i]) {
            NavigateNextAnnounce = i + 1;
            break;
         }
      }

      if (NavigateNextAnnounce == -1) {
         NavigateNextAnnounce = sizeof(ANNOUNCES)/sizeof(ANNOUNCES[0]);
      }
   }

   if (NavigateNextAnnounce > 0 &&
      (NavigateDistanceToTurn <=
        (ANNOUNCES[NavigateNextAnnounce - 1] + pos.speed * ANNOUNCE_PREPARE_FACTORS[NavigateNextAnnounce - 1] / 100))) {
      unsigned int i;

      announce_distance = ANNOUNCES[NavigateNextAnnounce - 1];
      NavigateNextAnnounce = 0;

      if (inst_voice) {
         announce = 1;
      }

      for (i=0; i<sizeof(ANNOUNCES)/sizeof(ANNOUNCES[0]); i++) {
         if ((ANNOUNCES[i] < announce_distance) &&
             (NavigateDistanceToTurn > ANNOUNCES[i])) {
            NavigateNextAnnounce = i + 1;
            break;
         }
      }
   }

   if (announce) {
      PluginStreetProperties properties;
      PluginLine segment_line;

		navigate_get_plugin_line (&segment_line, segment);
      roadmap_plugin_get_street_properties (&segment_line, &properties, 0);

      roadmap_message_set ('#', properties.address);
      roadmap_message_set ('N', properties.street);
      //roadmap_message_set ('T', properties.street_t2s);
      roadmap_message_set ('C', properties.city);

      if (navgiate_main_voice_guidance_enabled() && roadmap_config_match(&NavigateConfigNavigationGuidance, "yes")) {
         sound_list = roadmap_sound_list_create (0);
      	if (!NavigateNextAnnounce) {
         	roadmap_message_unset ('t');
      	} else {

        	 	char distance_str[100];
         	int distance_far =
            	roadmap_math_to_trip_distance(announce_distance);

         	roadmap_sound_list_add (sound_list, "within");

	         if (distance_far > 0) {
    	        	roadmap_message_set ('t', "%d %s",
        	          distance_far, roadmap_math_trip_unit());

            	sprintf(distance_str, "%d", distance_far);
	            roadmap_sound_list_add (sound_list, distance_str);
    	        	roadmap_sound_list_add (sound_list, roadmap_math_trip_unit());
        	 	} else {
            	roadmap_message_set ('t', "%d %s",
                	  announce_distance, roadmap_math_distance_unit());

            	sprintf(distance_str, "%d", announce_distance);
            	roadmap_sound_list_add (sound_list, distance_str);
	            roadmap_sound_list_add (sound_list, roadmap_math_distance_unit());
    	     	};
      	}

			if (inst_roundabout) {
      		roadmap_sound_list_add (sound_list, inst_roundabout);
			}
			if ( inst_voice[0] ){
				roadmap_sound_list_add (sound_list, inst_voice);
			}
      	if (inst_roundabout) {
	      	if (roundabout_exit > 0 && roundabout_exit <= MaxExitName) {
	      		roadmap_sound_list_add (sound_list, ExitName[roundabout_exit - 1]);
	      	} else  if (roundabout_exit == -1) {
	      		roadmap_sound_list_add (sound_list, "Marked");
	      	}
      	}
      	//roadmap_voice_announce ("Driving Instruction");

      	roadmap_sound_play_list (sound_list);
      }
   }

}

void navigate_main_stop_navigation(void)
{
   if( !NavigateTrackEnabled)
      return;

   navigate_main_suspend_navigation ();
   roadmap_trip_remove_point ("Destination");
   roadmap_config_set_integer (&NavigateConfigNavigating, 0);
   roadmap_config_save(1);

   roadmap_screen_redraw();
}

void navigate_main_stop_navigation_menu(void)
{
	navigate_main_stop_navigation();
	ssd_dialog_hide_all(dec_close);
}

static void navigate_request_segments (void) {

	int distance = 0;
	int i;
   int num_segments = navigate_num_segments ();

	if (NavigateCurrentRequestSegment >= num_segments) return;

	for (i = NavigateCurrentSegment; i < num_segments; i++) {
		NavigateSegment *segment = navigate_segment (i);
		if (i > NavigateCurrentRequestSegment) {
   		roadmap_tile_request (segment->square, ROADMAP_TILE_STATUS_PRIORITY_PREFETCH, 1, NULL);
			NavigateCurrentRequestSegment = i;
		}
		distance += segment->distance;
		if (distance > NAVIGATE_PREFETCH_DISTANCE) break;
	}
}

void navigate_get_next_line
          (PluginLine *current, int direction, PluginLine *next) {

   int new_instruction = 0;
   PluginLine segment_line;
   NavigateSegment *segment;
   NavigateSegment *next_segment;
   int num_segments;
   int i;

	roadmap_log (ROADMAP_DEBUG, "navigate_get_next_line(): current is %d/%d", current->square, current->line_id);

   if (!NavigateTrackEnabled) {

      if (navigate_main_recalc_route () != -1) {

         roadmap_trip_stop ();
      }

      return;
   }

   num_segments = navigate_num_segments ();

   /* Ugly hack as we don't support navigation through editor lines */
   if (roadmap_plugin_get_id (current) != ROADMAP_PLUGIN_ID) {
		navigate_get_plugin_line (next, navigate_segment (NavigateCurrentSegment+1));
      return;
   }

	if (NavigateCurrentSegment == 0) {
		new_instruction = 1;
	}

	segment = navigate_segment (NavigateCurrentSegment);
	navigate_get_plugin_line (&segment_line, segment);
   if (!roadmap_plugin_same_line
         (current, &segment_line)) {

      for (i=NavigateCurrentSegment+1; i < num_segments; i++) {

         next_segment = navigate_segment (i);
         if (!next_segment->is_instrumented) {
         	if (i == NavigateCurrentSegment+1) {
	         	// the next segment is not loaded -- can't sync
	         	roadmap_log (ROADMAP_ERROR, "Cannot reorute because segments are not instrumented");
	      		next->plugin_id = INVALID_PLUGIN_ID;
	         	return;
         	} else {
         		roadmap_log (ROADMAP_DEBUG, "Stop searching for route match on uninstrumented segment %d", i);
         		break;
         	}
         }
			navigate_get_plugin_line (&segment_line, next_segment);
         if (roadmap_plugin_same_line
            (current, &segment_line)) {

            if (next_segment->group_id !=
                  segment->group_id) {

               new_instruction = 1;
            }

            NavigateCurrentSegment = i;
            segment = next_segment;
				roadmap_log (ROADMAP_DEBUG, "NavigateCurrentSegment = %d", NavigateCurrentSegment);
            NavigateFlags &= ~CHANGED_DEPARTURE;
            if (i >= NavigateDetourSize && NavigateOfftrackTime) {
            	roadmap_log (ROADMAP_INFO, "Back on track");
            	NavigateOfftrackTime = 0;
            }
            break;
         }
      }
   }

   if ((NavigateCurrentSegment < num_segments) &&
       !roadmap_plugin_same_line
         (current, &segment_line)) {

      NavigateNextAnnounce = -1;

      roadmap_log (ROADMAP_DEBUG, "Recalculating route...");

      if (navigate_main_recalc_route () == -1) {

			// Why is that needed? Causes a redraw which focuses on last trip (departure?)
         //roadmap_trip_start ();
         return;
      }
		num_segments = navigate_num_segments ();
		segment = navigate_segment (NavigateCurrentSegment);
	}

   if ((NavigateCurrentSegment+1) >= num_segments) {

      next->plugin_id = INVALID_PLUGIN_ID;
   } else {

		navigate_get_plugin_line (next, navigate_segment (NavigateCurrentSegment+1));
   }

   if (new_instruction || !NavigateCurrentSegment) {
      int group_id;

      /* new driving instruction */

		for (i = NavigateCurrentSegment + 1; i < num_segments; i++) {
      	next_segment = navigate_segment (i);
         if (next_segment->group_id != segment->group_id) break;
         segment = next_segment;
      }
      roadmap_log (ROADMAP_DEBUG, "Group id is %d, next segment is %d", segment->group_id,
      			(int)(segment - NavigateSegments));

      navigate_bar_set_instruction (segment->instruction);
      if (segment->instruction == ROUNDABOUT_ENTER ||
      	 segment->instruction == ROUNDABOUT_EXIT) {
      	navigate_bar_set_exit (segment->exit_no);
      }

      group_id = segment->group_id;

      if (i < num_segments) {
         /* we need the name of the next street */
         segment = navigate_segment (i++);
      }
      while (i < num_segments &&
      		 segment->context == SEG_ROUNDABOUT) {
      	/* skip roundabout segments for street name */
      	segment = navigate_segment (i++);
      }
     	navigate_display_street (i - 1);

      NavigateNextAnnounce = -1;

      NavigateDistanceToDest = 0;
      NavigateETA = 0;
      NavigateETADiff = 0;

      if (segment->group_id != group_id) {

         /* Update distance to destination and ETA
          * excluding current group (computed in navigate_update)
          */

         while (i <= num_segments) {

            NavigateDistanceToDest += segment->distance;
            NavigateETA            += segment->cross_time;
            segment = navigate_segment (i++);
         }
         NavigateETATime = time(NULL);
      }
   }

	navigate_request_segments ();
   return;
}


int navigate_is_enabled (void) {
   return NavigateEnabled;
}

int navigate_track_enabled(void){
	return NavigateTrackEnabled;
}

int navigate_offtrack(void){
	return NavigateOfftrackTime;
}


int navigate_is_line_on_route(int square_id, int line_id, int from_line, int to_line){

   int i;
   int line_from_point;
   int line_to_point;
   NavigateSegment *segment;
   int num_segments;

   if (!NavigateTrackEnabled)
      return 0;

   num_segments = navigate_num_segments ();
   for (i=NavigateCurrentSegment+1; i < num_segments; i++) {
   	segment = navigate_segment (i);
      if (segment->square == square_id &&
            segment->line == line_id) {

         if (from_line == -1 && to_line == -1)
         	return 1;

         roadmap_square_set_current (square_id);
         if (segment->line_direction == ROUTE_DIRECTION_WITH_LINE)
         	roadmap_line_points (line_id, &line_from_point, &line_to_point);
         else
         	roadmap_line_points (line_id, &line_to_point, &line_from_point);
         if ((line_from_point == from_line) && (line_to_point ==to_line))
            return 1;
      }
   }

   return 0;

}

void navigate_get_waypoint (int distance, RoadMapPosition *way_point) {

	int num_segments = navigate_num_segments ();
	int i = NavigateCurrentSegment;
   NavigateSegment *segment = NULL;

   assert(NavigateTrackEnabled);

   if (distance == -1) {
      *way_point = NavigateDestPos;
      return;
   }

   distance -= NavigateDistanceToNext;

   while ((distance > 0) &&
      (++i < num_segments)) {
      segment = navigate_segment (i);
      distance -= segment->distance;
   }

   if (distance > 0) segment = navigate_segment (i-1);

   if (segment->line_direction == ROUTE_DIRECTION_WITH_LINE) {
      *way_point = segment->to_pos;
   } else {
      *way_point = segment->from_pos;
   }
}

static void navigate_main_init_pens (void) {

   RoadMapPen pen;

   pen = roadmap_canvas_create_pen ("NavigatePen1");
   roadmap_canvas_set_foreground
      (roadmap_config_get (&NavigateConfigRouteColor));
   roadmap_canvas_set_thickness (ROUTE_PEN_WIDTH);
   NavigatePen[0] = pen;

   pen = roadmap_canvas_create_pen ("NavigatePen2");
   roadmap_canvas_set_foreground
      (roadmap_config_get (&NavigateConfigRouteColor));
   roadmap_canvas_set_thickness (ROUTE_PEN_WIDTH);
   NavigatePen[1] = pen;

   pen = roadmap_canvas_create_pen ("NavigatePenEst1");
   roadmap_canvas_set_foreground
      (roadmap_config_get (&NavigateConfigPossibleRouteColor));
   roadmap_canvas_set_thickness (ROUTE_PEN_WIDTH);
   NavigatePenEst[0] = pen;

   pen = roadmap_canvas_create_pen ("NavigatePenEst2");
   roadmap_canvas_set_foreground
      (roadmap_config_get (&NavigateConfigPossibleRouteColor));
   roadmap_canvas_set_thickness (ROUTE_PEN_WIDTH);
   NavigatePenEst[1] = pen;

   pen = roadmap_canvas_create_pen ("NavigateAlt1Pen1-0");
    roadmap_canvas_set_foreground
       (roadmap_config_get (&NavigateConfigAlt1RouteColor));
    roadmap_canvas_set_thickness (ROUTE_PEN_WIDTH);
    NavigateAltPens[0][0] = pen;

    pen = roadmap_canvas_create_pen ("NavigateAlt1Pen1-1");
     roadmap_canvas_set_foreground
        (roadmap_config_get (&NavigateConfigAlt1RouteColor));
     roadmap_canvas_set_thickness (ROUTE_PEN_WIDTH);
     NavigateAltPens[0][0] = pen;

     pen = roadmap_canvas_create_pen ("NavigateAlt1Pen2-0");
     roadmap_canvas_set_foreground
        (roadmap_config_get (&NavigateConfigAlt2RouteColor));
     roadmap_canvas_set_thickness (ROUTE_PEN_WIDTH);
     NavigateAltPens[1][0] = pen;

     pen = roadmap_canvas_create_pen ("NavigateAlt1Pen2-1");
      roadmap_canvas_set_foreground
         (roadmap_config_get (&NavigateConfigAlt2RouteColor));
      roadmap_canvas_set_thickness (ROUTE_PEN_WIDTH);
      NavigateAltPens[1][1] = pen;

      pen = roadmap_canvas_create_pen ("NavigateAlt1Pen3-0");
      roadmap_canvas_set_foreground
         (roadmap_config_get (&NavigateConfigAlt3RouteColor));
      roadmap_canvas_set_thickness (ROUTE_PEN_WIDTH);
      NavigateAltPens[2][0] = pen;

      pen = roadmap_canvas_create_pen ("NavigateAlt1Pen3-0");
      roadmap_canvas_set_foreground
         (roadmap_config_get (&NavigateConfigAlt3RouteColor));
      roadmap_canvas_set_thickness (ROUTE_PEN_WIDTH);
      NavigateAltPens[2][1] = pen;
}

void navigate_main_shutdown (void) {
#ifdef IPHONE
   if (!roadmap_main_is_in_background()) //should we keep the navigation state?
#endif //IPHONE
	{
		if ( roadmap_config_match(&NavigateConfigNavigating,"1")&&
		   ( navigate_is_auto_zoom()))
		{                              // if in autozoom and navigating, reset zoom to default
			roadmap_math_zoom_reset(); // so next time it won't start out in strange zoom
		}
   		roadmap_config_set_integer (&NavigateConfigNavigating, 0);
   		roadmap_trip_remove_point ("Destination");

      set_last_nav_time();
   }
}

void toggle_navigation_guidance(){
	if (roadmap_config_match(&NavigateConfigNavigationGuidance, "yes")){
		ssd_bitmap_splash("splash_sound_off", 1);
		roadmap_config_set(&NavigateConfigNavigationGuidance, "no");
	}else{
		ssd_bitmap_splash("splash_sound_on", 1);
		roadmap_config_set(&NavigateConfigNavigationGuidance, "yes");
	}
}

int navigation_guidance_state(){
		if (roadmap_config_match(&NavigateConfigNavigationGuidance, "yes"))
			return 1;
		else
			return 0;
}



void navigate_resume_navigation (int exit_code, void *context){

   if (exit_code != dec_yes){
        roadmap_config_set_integer (&NavigateConfigNavigating, 0);
        roadmap_config_save (TRUE);
   } else {

   	navigate_main_calc_route ();
   }
   if (NavigateNextLoginCb) {
   	NavigateNextLoginCb ();
   	NavigateNextLoginCb = NULL;
   }
}


void navigate_main_login_cb(void){
   ssd_confirm_dialog("Resume navigation",roadmap_lang_get("Navigation was discontinued before reaching destination. Would you like to resume last route?"),TRUE, navigate_resume_navigation, NULL );
}

static BOOL short_time_since_last_nav () {
   time_t now;
   int last_nav_time = roadmap_config_get_integer(&NavigateConfigNavigateTime);

   if (last_nav_time == -1) //for crashes
      return TRUE;

   now = time(NULL);
   if ((now - last_nav_time) < (MAX_MINUTES_TO_RESUME_NAV * 60))
      return TRUE;
   else
      return FALSE;

}

static void set_last_nav_time () {
   int time_now = (int)time (NULL);
   roadmap_config_set_integer (&NavigateConfigNavigateTime, time_now);
   roadmap_config_save (0);
}


void navigate_main_initialize (void) {

   roadmap_config_declare
      ("schema", &NavigateConfigRouteColor,  "#b3a1f6a0", NULL);
   roadmap_config_declare
      ("schema", &NavigateConfigPossibleRouteColor,  "#9933FF", NULL);

   roadmap_config_declare
      ("schema", &NavigateConfigAlt1RouteColor,  "#fb27ea", NULL);
   roadmap_config_declare
      ("schema", &NavigateConfigAlt2RouteColor,  "#71c113", NULL);
   roadmap_config_declare
      ("schema", &NavigateConfigAlt3RouteColor,  "#3dbce0", NULL);

   roadmap_config_declare_enumeration
      ("user", &NavigateConfigAutoZoom, NULL, "yes", "no", NULL);
   roadmap_config_declare_enumeration
      ("user", &NavigateConfigNavigationGuidance, NULL, "yes", "no", NULL);
   roadmap_config_declare_enumeration
      ("preferences", &NavigateConfigNavigationGuidanceEnabled, NULL, "yes", "no", NULL);
   roadmap_config_declare_enumeration
      ("preferences", &NavigateConfigEtaEnabled, NULL, "yes", "no", NULL);
   roadmap_config_declare_enumeration
      ("user", &NavigateConfigNavigationGuidance, NULL, "yes", "no", NULL);

   roadmap_config_declare
      ("session",  &NavigateConfigLastPos, "0, 0", NULL);
   roadmap_config_declare
      ("session",  &NavigateConfigNavigating, "0", NULL);
   roadmap_config_declare
      ("session",  &NavigateConfigNavigateTime, "-1", NULL);

   roadmap_config_declare_enumeration
      ("preferences", &ShowDisclaimerCfg, NULL, "no", "yes", NULL);

   navigate_main_init_pens ();

   navigate_cost_initialize ();

   NavigatePluginID = navigate_plugin_register ();
   navigate_traffic_initialize ();

   navigate_main_set (1);

   NextMessageUpdate =
      roadmap_message_register (navigate_main_format_messages);

   roadmap_skin_register (navigate_main_init_pens);

   if (roadmap_config_get_integer (&NavigateConfigNavigating) &&
       short_time_since_last_nav()) {

      RoadMapPosition pos;
      roadmap_config_get_position (&NavigateConfigLastPos, &pos);
      roadmap_trip_set_focus ("GPS");
      roadmap_trip_set_point ("Destination", &pos);

      NavigateNextLoginCb = Realtime_NotifyOnLogin (navigate_main_login_cb);
   }
   else{
   	  roadmap_trip_remove_point ("Destination");
      roadmap_config_set_integer (&NavigateConfigNavigating, 0);
      roadmap_config_save(1);
   }

   roadmap_config_set_integer (&NavigateConfigNavigateTime, -1);
   roadmap_config_save (0);
}


void navigate_main_set (int status) {

   if (status && NavigateEnabled) {
      return;
   } else if (!status && !NavigateEnabled) {
      return;
   }

   NavigateEnabled = status;
}


#ifdef TEST_ROUTE_CALC
#include "roadmap_shape.h"
int navigate_main_test (int test_count) {

   int track_time;
   PluginLine from_line;
   int from_point;
   int line;
   int lines_count = roadmap_line_count();
   RoadMapPosition pos;
   int flags;
   static int itr = 0;
   const char *focus = roadmap_trip_get_focus_name ();

   if (!itr) {
      srand(0);
   }

   NavigateTrackFollowGPS = focus && !strcmp (focus, "GPS");

   if (navigate_route_load_data () < 0) {

      roadmap_messagebox("Error", "Error loading navigation data.");
      return -1;
   }

   if (test_count) test_count++;

   while (1) {
      int first_shape, last_shape;

      printf ("Iteration: %d\n", itr++);
         if (test_count) {
            test_count--;
            if (!test_count) break;
         }

      line = (int) (lines_count * (rand() / (RAND_MAX + 1.0)));
      roadmap_line_from (line, &pos);
      roadmap_line_shapes (line, -1, &first_shape, &last_shape);
      if (first_shape != -1) {
         last_shape = (first_shape + last_shape) / 2;
         roadmap_shape_get_position (first_shape, &pos);
         while (first_shape != last_shape) {
            roadmap_shape_get_position (++first_shape, &pos);
         }
      }

      if (!NavigateTrackFollowGPS) {
         roadmap_trip_set_point ("Departure", &pos);
      }

      line = (int) (lines_count * (rand() / (RAND_MAX + 1.0)));
      roadmap_line_from (line, &pos);
      roadmap_line_shapes (line, -1, &first_shape, &last_shape);
      if (first_shape != -1) {
         last_shape = (first_shape + last_shape) / 2;
         roadmap_shape_get_position (first_shape, &pos);
         while (first_shape != last_shape) {
            roadmap_shape_get_position (++first_shape, &pos);
         }
      }
      roadmap_trip_set_point ("Destination", &pos);

      NavigateDestination.plugin_id = INVALID_PLUGIN_ID;
      navigate_main_suspend_navigation ();

      NavigateNumSegments = MAX_NAV_SEGEMENTS;

      if (navigate_find_track_points_in_scale
            (&from_line, &from_point, &NavigateDestination, &NavigateDestPoint, 1, 0)) {

         printf ("Error finding navigate points.\n");
         continue;
      }

      flags = NEW_ROUTE|RECALC_ROUTE;
      navigate_cost_reset ();
      track_time =
         navigate_route_get_segments
         (&from_line, from_point, &NavigateDestination, NavigateDestPoint,
          NavigateSegments, &NavigateNumSegments,
          &flags);

      if (track_time <= 0) {
         navigate_main_suspend_navigation ();
         if (track_time < 0) {
            printf("Error calculating route.\n");
         } else {
            printf("Error - Can't find a route.\n");
         }

         continue;
      } else {
         char msg[200] = {0};
         int i;
         int length = 0;

         NavigateCalcTime = time(NULL);

         navigate_instr_prepare_segments (NavigateSegments, NavigateNumSegments,
               &NavigateSrcPos, &NavigateDestPos);
			NavigateNumInstSegments = NavigateNumSegments;

         track_time = 0;
         for (i=0; i<NavigateNumSegments; i++) {
            length += NavigateSegments[i].distance;
            track_time += NavigateSegments[i].cross_time;
         }

         NavigateFlags = flags;

         if (flags & GRAPH_IGNORE_TURNS) {
            snprintf(msg, sizeof(msg), "%s\n",
                  roadmap_lang_get ("The calculated route may have incorrect turn instructions."));
         }

         if (flags & CHANGED_DESTINATION) {
            snprintf(msg + strlen(msg), sizeof(msg) - strlen(msg), "%s\n",
                  roadmap_lang_get ("Unable to provide route to destination. Taking you to nearest location."));
         }

         if (flags & CHANGED_DEPARTURE) {
            snprintf(msg + strlen(msg), sizeof(msg) - strlen(msg), "%s\n",
                  roadmap_lang_get ("Showing route using alternative departure point."));
         }

         snprintf(msg + strlen(msg), sizeof(msg) - strlen(msg),
               "%s: %.1f %s\n%s: %.1f %s",
               roadmap_lang_get ("Length"),
               length/1000.0,
               roadmap_lang_get ("Km"),
               roadmap_lang_get ("Time"),
               track_time/60.0,
               roadmap_lang_get ("minutes"));

         NavigateTrackEnabled = 1;
		   NavigateAnnounceSegment = -1;
         navigate_bar_set_mode (NavigateTrackEnabled);

         roadmap_screen_redraw ();
         printf ("Route found!\n%s\n\n", msg);

      }
   }

   return 0;
}
#endif

int navigate_main_calc_route () {

   int track_time;
   PluginLine from_line;
   int from_point;
   int flags;
   int from_direction;
   NavigateSegment *segments;
   int num_segments;
   int num_new_segments;
   const RoadMapPosition * depPos;

   const char *focus = roadmap_trip_get_focus_name ();

#ifdef TEST_ROUTE_CALC_STRESS
   navigate_bar_initialize ();
   navigate_main_test(0);
#endif

   NavigateTrackFollowGPS = focus && !strcmp (focus, "GPS");

   if (NavigateTrackFollowGPS) {
      if (roadmap_trip_get_position ("Departure")) {
         roadmap_trip_remove_point ("Departure");
      }
   }

   depPos = roadmap_trip_get_position ("Departure");
   if (!NavigateTrackFollowGPS && (!depPos ||(depPos->latitude == 0 && depPos->longitude==0))) {
      NavigateTrackFollowGPS = 1;
   }

   NavigateDestination.plugin_id = INVALID_PLUGIN_ID;
   navigate_main_suspend_navigation ();

   if (navigate_route_load_data () < 0) {

      roadmap_messagebox("Error", "Error loading navigation data.");
      return -1;
   }

   if (navigate_find_track_points_in_scale
         (&from_line, &from_point, &NavigateDestination, &NavigateDestPoint, &from_direction, 0, 0)) {
      return -1;
   }

	NavigateFromLinePending = from_line;
	NavigateFromPointPending = from_point;

   flags = NEW_ROUTE | ALLOW_DESTINATION_CHANGE | ALLOW_ALTERNATE_SOURCE;

   navigate_cost_reset ();

   NavigateNumOutlinePoints = 0;

   if (RealTimeLoginState ()) {
   	NavigateIsByServer = 1;
   	roadmap_log (ROADMAP_INFO, "Requesting new route..");
   	CalculatingRoute = TRUE;
   	NavigateRerouteTime = 0;
   	roadmap_main_set_periodic( 300, navigate_progress_message_delayed );
		roadmap_main_set_periodic( 30000, navigate_progress_message_hide_delayed );
   	navigate_route_request (&from_line, from_point, &NavigateDestination,
   									&NavigateSrcPos, &NavigateDestPos, NavigateDestStreet, flags,
   									-1, 1, NULL, navigate_main_on_segments, navigate_main_update_route);
   	return 0;

   } else {
   	NavigateIsByServer = 0;
   	if (NavigateDestination.plugin_id != ROADMAP_PLUGIN_ID) {

      	roadmap_messagebox ("Error", "Can't find a road near destination point.");
   	}

   	show_progress_dialog ();

   	roadmap_log (ROADMAP_INFO, "Calculating new route..");
	   track_time =
	      navigate_route_get_segments
	            (&from_line, from_point, &NavigateDestination, &NavigateDestPoint,
	             &segments, &num_segments, &num_new_segments,
	             &flags, NULL, 0);
   }

   if (track_time <= 0) {
#ifdef SSD
      ssd_dialog_hide ("Route calc", dec_close);
#else
      roadmap_dialog_hide ("Route calc");
#endif
      if (track_time < 0) {
         roadmap_messagebox("Error", "Error calculating route.");
      } else {
         roadmap_messagebox("Error", "Can't find a route.");
      }

      return -1;
   } else if (track_time > 0) {
      int i;
      int length = 0;

      navigate_bar_initialize ();
      NavigateSegments = segments;
      NavigateNumSegments = num_segments;
      NavigateDetourSize = 0;
      NavigateDetourEnd = 0;
      navigate_instr_prepare_segments (navigate_segment, num_segments, num_new_segments,
                                      &NavigateSrcPos, &NavigateDestPos);

      track_time = 0;
      for (i=0; i<num_segments; i++) {
         length += segments[i].distance;
         track_time += segments[i].cross_time;
      }

		navigate_main_on_route (flags, length, track_time,
									  	segments, num_segments, num_segments,
									  	NULL, 0);

   }

   return 0;
}


static void navigate_main_outline_iterator (int shape, RoadMapPosition *position) {

	if (NavigateOriginalRoutePoints != NULL) {
		*position = NavigateOriginalRoutePoints[shape];
	} else {
		*position = NavigateOutlinePoints[shape];
	}
}

void navigate_main_set_outline(RoadMapPosition *outline_points, int num_outline_points, int alt_id){
   navigate_free_points ();
   if (outline_points)
      NavigateDisplayALtRoute = 1;
   else
      NavigateDisplayALtRoute = 0;
   NavigateOutlinePoints = outline_points;
   NavigateNumOutlinePoints = num_outline_points;
   NavigateAltId = alt_id;


}

static void navigate_main_screen_outline (void) {

   RoadMapPen *pens;
   RoadMapPen previous_pen;
   RoadMapPosition *points;
   int num_points;

   if (NavigateOriginalRoutePoints) {

   	points = NavigateOriginalRoutePoints;
   	num_points = NavigateNumOriginalRoutePoints;
   }
   else {

   	points = NavigateOutlinePoints;
   	num_points = NavigateNumOutlinePoints;
   }

   if ((NavigateFlags & GRAPH_IGNORE_TURNS) ||
   	 (NavigateFlags & CHANGED_DESTINATION)) {
      pens = NavigatePenEst;
   } else {
      if (NavigateDisplayALtRoute){
         pens = NavigateAltPens[NavigateAltId];
      }
      else
         pens = NavigatePen;
   }

   previous_pen = roadmap_canvas_select_pen (pens[0]);
   roadmap_canvas_set_thickness (ROUTE_PEN_WIDTH);
   if (previous_pen) {
      roadmap_canvas_select_pen (previous_pen);
   }

  	roadmap_screen_draw_one_line (points,
                                 points + num_points - 1,
                                 0,
                                 points,
                                 1,
                                 num_points - 2,
                                 navigate_main_outline_iterator,
                                 pens,
                                 1,
                                 -1,
                                 NULL,
                                 NULL,
                                 NULL);

}

static RoadMapScreenSubscriber screen_prev_after_refresh = NULL;

void navigate_main_draw_route_number(void){
   RoadMapGuiPoint screen_point;
   RoadMapGuiPoint icon_screen_point;
   RoadMapImage image;
   char icon_name[20];

   if (!NavigateDisplayALtRoute){
      if (screen_prev_after_refresh)
         (*screen_prev_after_refresh)();
      return;
   }
   sprintf(icon_name,"%d_route",NavigateAltId+1 );
   roadmap_math_coordinate (&NavigateOutlinePoints[NavigateNumOutlinePoints/2], &screen_point);
   roadmap_math_rotate_coordinates (1, &screen_point);
   image = (RoadMapImage) roadmap_res_get(RES_BITMAP, RES_SKIN, icon_name);
   icon_screen_point.x = screen_point.x - roadmap_canvas_image_width(image)/2  ;
   icon_screen_point.y = screen_point.y - roadmap_canvas_image_height (image) ;
   roadmap_canvas_draw_image (image, &icon_screen_point,  255, IMAGE_NORMAL);

   if (screen_prev_after_refresh)
      (*screen_prev_after_refresh)();

}

void navigate_main_screen_repaint (int max_pen) {
   int i;
   int current_width = -1;
   int last_cfcc = -1;
   RoadMapPen *pens;
   int current_pen = 0;
   int pen_used = 0;
   int num_segments;

   if (NavigateDisplayALtRoute){
      navigate_main_screen_outline ();
      if (screen_prev_after_refresh == NULL)
         screen_prev_after_refresh = roadmap_screen_subscribe_after_refresh (navigate_main_draw_route_number);
      return;
   }


	if (roadmap_math_get_zoom () >= 100 &&
		 (NavigateOriginalRoutePoints != NULL ||
		  (NavigateTrackEnabled &&
		  	NavigateNumOutlinePoints > 0 && ((NavigateNumInstSegments == 0) ||
		   NavigateNumInstSegments < NavigateNumSegments))))
	{
		navigate_main_screen_outline ();
		return;
	}

   if (!NavigateTrackEnabled) return;

   if ((NavigateFlags & GRAPH_IGNORE_TURNS) ||
   	 (NavigateFlags & CHANGED_DESTINATION)) {
      pens = NavigatePenEst;
   } else {
      pens = NavigatePen;
   }

   if (!NavigateTrackFollowGPS && roadmap_trip_get_focus_name () &&
         !strcmp (roadmap_trip_get_focus_name (), "GPS")) {

      NavigateTrackFollowGPS = 1;

      roadmap_trip_stop ();

      if (roadmap_trip_get_position ("Departure")) {
         roadmap_trip_remove_point ("Departure");
      }
      roadmap_navigate_route (NavigateCallbacks);
   }

   num_segments = navigate_num_segments ();
   for (i=0; i<num_segments; i++) {

      NavigateSegment *segment = navigate_segment (i);
      RoadMapArea square_area;

		if (!(segment->is_instrumented && segment->distance)) continue;

		roadmap_tile_edges (segment->square,
								  &square_area.west,
								  &square_area.east,
								  &square_area.south,
								  &square_area.north);
		if (!roadmap_math_is_visible (&square_area)) {
			continue;
		}

      roadmap_square_set_current (segment->square);
      if (segment->cfcc != last_cfcc) {
         RoadMapPen layer_pen =
               roadmap_layer_get_pen (segment->cfcc, 0, 0);
         int width;

         if (layer_pen) width = roadmap_canvas_get_thickness (layer_pen) * 2 / 3;
         else width = ROUTE_PEN_WIDTH;

         if (width < ROUTE_PEN_WIDTH) {
            width = ROUTE_PEN_WIDTH;
         }

         if (width != current_width) {

            RoadMapPen previous_pen;
            if (pen_used) {
                current_pen = (current_pen+1)%2;
            }
            pen_used = 0;
            previous_pen = roadmap_canvas_select_pen (pens[current_pen]);
            roadmap_canvas_set_thickness (width);
            current_width = width;

            if (previous_pen) {
               roadmap_canvas_select_pen (previous_pen);
            }
         }

         last_cfcc = segment->cfcc;
      }

      pen_used |=
           roadmap_screen_draw_one_line (&segment->from_pos,
                                         &segment->to_pos,
                                         0,
                                         &segment->shape_initial_pos,
                                         segment->first_shape,
                                         segment->last_shape,
                                         NULL,
                                         pens + current_pen,
                                         1,
                                         -1,
                                         NULL,
                                         NULL,
                                         NULL);
   }
}


int navigate_main_reload_data (void) {

   navigate_traffic_refresh ();
   return navigate_route_reload_data ();
}

int navigate_is_auto_zoom (void) {
   return (roadmap_config_match(&NavigateConfigAutoZoom, "yes"));
}

///////////////////////////////////////////////////
// Navigation List
//////////////////////////////////////////////////
#define MAX_NAV_LIST_ENTRIES 	100
#define NAVIGATION_POP_UP_NAME  "Navigation list Pop Up"

typedef struct navigate_list_value_st{
	const char 		*str;
	const char		*icon;
	int 			inst_num;
	RoadMapPosition position;
}navigate_list_value;

static char *navigate_list_labels[MAX_NAV_LIST_ENTRIES] ;
static navigate_list_value *navigate_list_values[MAX_NAV_LIST_ENTRIES] ;
static const char *navigate_list_icons[MAX_NAV_LIST_ENTRIES];
static navigate_list_value *current_displayed_value;
static int navigate_list_count;
static void display_pop_up(navigate_list_value *list_value);

///////////////////////////////////////////////////
//
//////////////////////////////////////////////////
int navigate_main_list_state(void){
   int sign_active = roadmap_display_is_sign_active(NAVIGATION_POP_UP_NAME);

   if (sign_active)
        return 0;
   else
	   return -1;
}

///////////////////////////////////////////////////
//
//////////////////////////////////////////////////
int navigate_main_is_list_displaying(void){
	if (navigate_main_list_state() == 0)
		return TRUE;
	else
		return FALSE;
}

int navigate_main_state(void){
   if (NavigateTrackEnabled)
      return 0;
   else
      return -1;
}
///////////////////////////////////////////////////
//
//////////////////////////////////////////////////
void navigate_main_list_hide(void){
	roadmap_display_hide(NAVIGATION_POP_UP_NAME);
	roadmap_softkeys_remove_right_soft_key("Hide_Directions");
}

///////////////////////////////////////////////////
//
//////////////////////////////////////////////////
static void navigate_main_list_set_right_softkey(void){
	static Softkey s;
	strcpy(s.text, "Hide");
	s.callback = navigate_main_list_hide;
	roadmap_softkeys_set_right_soft_key("Hide_Directions", &s);
}

///////////////////////////////////////////////////
//
//////////////////////////////////////////////////
static int navigate_main_list_display_next (SsdWidget widget, const char *new_value){

   navigate_list_value *list_value = current_displayed_value;
   if (list_value->inst_num == (navigate_list_count-1))
      return 0;

   current_displayed_value = navigate_list_values[list_value->inst_num+1];
   list_value = current_displayed_value;
   display_pop_up(list_value);
   return 1;
}

///////////////////////////////////////////////////
//
//////////////////////////////////////////////////
static int navigate_main_list_display_previous(SsdWidget widget, const char *new_value){
   navigate_list_value *list_value = current_displayed_value;
   if (list_value->inst_num == 0)
      return 0;

   current_displayed_value = navigate_list_values[list_value->inst_num-1];
   list_value = current_displayed_value;
   display_pop_up(list_value);
   return 1;
}
///////////////////////////////////////////////////
//
//////////////////////////////////////////////////
static void display_pop_up(navigate_list_value *list_value){
	SsdWidget popup;
	SsdWidget button;
	SsdWidget text;
	SsdWidget image_con;
	char *icons[2];

	roadmap_screen_hold ();

   popup = ssd_popup_new("Direction", "", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE,&list_value->position, SSD_POINTER_LOCATION);
    /* Makes it possible to click in the bottom vicinity of the buttons  */
   ssd_widget_set_click_offsets_ext( popup, 0, 0, 0, 15 );

   image_con =
      ssd_container_new ("IMAGE_container", "", SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_ALIGN_RIGHT);
   ssd_widget_set_color(image_con, NULL, NULL);

   text = ssd_text_new("direction_txt",list_value->str,-1,SSD_END_ROW );
   ssd_widget_add(popup, text);
   icons[0] = "previous_e";
   icons[1] = NULL;
   button = ssd_button_new("prev", "prev_direction", (const char**) &icons[0], 1, SSD_ALIGN_VCENTER|SSD_ALIGN_RIGHT, navigate_main_list_display_previous);
   ssd_widget_add(popup, button);

   icons[0] = "next_e";
   icons[1] = NULL;
   button = ssd_button_new("next", "next_direction", (const char**) &icons[0], 1, SSD_ALIGN_VCENTER, navigate_main_list_display_next);
   ssd_widget_add(popup, button);

   roadmap_trip_set_point("Hold", &list_value->position);
	roadmap_screen_update_center(&list_value->position);
   roadmap_math_set_scale(300, roadmap_screen_height() / 3);
   roadmap_layer_adjust();
   roadmap_view_auto_zoom_suspend();
	ssd_dialog_hide_all(dec_ok);
	ssd_dialog_activate("Direction", NULL);
	navigate_main_list_set_right_softkey();
}

///////////////////////////////////////////////////
//
//////////////////////////////////////////////////
static int navigate_main_list_call_back (SsdWidget widget, const char *new_value, const void *value, void *context) {
	navigate_list_value *list_value = (navigate_list_value *)value;

	current_displayed_value = list_value;
	display_pop_up(list_value);
	return 0;
}


///////////////////////////////////////////////////
//
//////////////////////////////////////////////////
void navigate_main_list(void){
	int i;
	int count = 0;
	const char *inst_text = "";
	int roundabout_exit = 0;
	int distance_to_next =0;
	int group_id = 0;
	PluginLine segment_line;

	for (i=0; i<MAX_NAV_LIST_ENTRIES;i++){
		navigate_list_labels[i] = NULL;
		navigate_list_values[i] = NULL;
		navigate_list_icons[i] = NULL;
	}


	if (NavigateTrackEnabled){
		int num_segments = navigate_num_segments ();
		NavigateSegment *segment = navigate_segment (NavigateCurrentSegment);
		PluginStreetProperties properties;
		NavigateSegment *Nextsegment;
		int total_instructions = 0;
		int segment_idx = NavigateCurrentSegment;

		while (segment_idx < num_segments - 1) {
			segment = navigate_segment (segment_idx);
		   if (segment->context == SEG_ROUNDABOUT){
		   		segment_idx++;
		   		continue;
		   }
		   Nextsegment = navigate_segment (++segment_idx);
		   if (segment->group_id != Nextsegment->group_id)
		   		total_instructions++;
		}

		segment_idx = NavigateCurrentSegment;

		while (segment_idx < num_segments) {

			segment = navigate_segment (segment_idx);
		   distance_to_next += segment->distance;

		   if (segment->context == SEG_ROUNDABOUT){
		   		segment_idx++;
		   		continue;
		   }

		   switch (segment->instruction) {
		      case TURN_LEFT:
		         inst_text = "Turn left";
		         break;
		      case ROUNDABOUT_LEFT:
		         inst_text = "At the roundabout, turn left";
		         break;
		      case KEEP_LEFT:
		         inst_text = "Keep left";
		         break;
		      case TURN_RIGHT:
		         inst_text = "Turn right";
		         break;
		      case ROUNDABOUT_RIGHT:
		         inst_text = "At the roundabout, turn right";
		         break;
		      case KEEP_RIGHT:
		         inst_text = "Keep right";
		         break;
		      case APPROACHING_DESTINATION:
		         inst_text = "Approaching destination";
		         break;
		      case CONTINUE:
		         inst_text = "continue straight";
		         break;
		      case ROUNDABOUT_STRAIGHT:
		         inst_text = "At the roundabout, continue straight";
		         break;
		      case ROUNDABOUT_ENTER:
		         inst_text = "At the roundabout, exit number";
		         roundabout_exit = segment->exit_no;
		         break;
		      case ROUNDABOUT_EXIT:
		         inst_text = "At the roundabout, exit";
				 break;
		      case ROUNDABOUT_U:
      	         inst_text = "At the roundabout, make a u turn";
      	         break;
		      default:
		         break;
		   }

	   	navigate_get_plugin_line (&segment_line, segment);
		   roadmap_plugin_get_street_properties (&segment_line, &properties, 0);
		   Nextsegment = navigate_segment (segment_idx + 1);
		   if (segment_idx == num_segments - 1 ||
		   	 segment->group_id != Nextsegment->group_id){
		   	char str[100];
		   	char dist_str[100];
		   	char unit_str[20];
		   	int distance_far;
		   	int instr;
		   	if (count == 0 && (NavigateDistanceToNext != 0))
		   		distance_to_next = NavigateDistanceToTurn;
   			    distance_far = roadmap_math_to_trip_distance(distance_to_next);
				if (distance_far > 0)
		        {
		            int tenths = roadmap_math_to_trip_distance_tenths(distance_to_next);
		            snprintf(dist_str, sizeof(str), "%d.%d", distance_far, tenths % 10);
		            snprintf(unit_str, sizeof(unit_str), "%s", roadmap_lang_get(roadmap_math_trip_unit()));
		        }
		        else
		        {
		            snprintf(dist_str, sizeof(str), "%d", roadmap_math_distance_to_current(distance_to_next));
		            snprintf(unit_str, sizeof(unit_str), "%s", roadmap_lang_get(roadmap_math_distance_unit()));
		        }

		   		if (segment_idx < num_segments - 1){

		   			instr = segment->instruction;
		   			if ((segment->instruction >= ROUNDABOUT_ENTER) && (segment->instruction <= ROUNDABOUT_U)) {

		   				int j;
		   				int id = Nextsegment->group_id;
		   				for (j = segment_idx + 1; j < num_segments; j++) {
		   					Nextsegment = navigate_segment (j);
		   					if (Nextsegment->group_id != id) break;
		   				}
		   			}

	   				navigate_get_plugin_line (&segment_line, Nextsegment);
		   			roadmap_plugin_get_street_properties (&segment_line, &properties, 0);

		   			if (instr == ROUNDABOUT_ENTER )
		   				sprintf(str, " (%d/%d) %s %s %s%d",count+1,total_instructions+1, dist_str, unit_str, roadmap_lang_get(inst_text), roundabout_exit);
		   			else if (instr == ROUNDABOUT_U )
		   				sprintf(str, " (%d/%d) %s %s %s",count+1, total_instructions+1, dist_str, unit_str, roadmap_lang_get(inst_text));

		   			else
		   				if (properties.street[0] != 0)
		   					sprintf(str, " (%d/%d) %s %s %s %s%s",count+1, total_instructions+1, dist_str, unit_str, roadmap_lang_get(inst_text), roadmap_lang_get("to"), properties.street);
		   				else
		   					sprintf(str, " (%d/%d) %s %s %s",count+1, total_instructions+1, dist_str, unit_str, roadmap_lang_get(inst_text));

		   			navigate_list_labels[count] = strdup(str);
		   			navigate_list_values[count] = (navigate_list_value *) malloc(sizeof(navigate_list_value));
		   			navigate_list_values[count]->str = strdup(str);
		   			navigate_list_values[count]->inst_num = count;
		   			navigate_list_values[count]->icon = NAVIGATE_DIR_IMG[instr];
		   			if (Nextsegment->line_direction == ROUTE_DIRECTION_WITH_LINE){
		   				navigate_list_values[count]->position.longitude = Nextsegment->from_pos.longitude;
		   				navigate_list_values[count]->position.latitude = Nextsegment->from_pos.latitude;
		   			}
		   			else{
		   				navigate_list_values[count]->position.longitude = Nextsegment->to_pos.longitude;
		   				navigate_list_values[count]->position.latitude = Nextsegment->to_pos.latitude;
		   			}
		   			navigate_list_icons[count] = NAVIGATE_DIR_IMG[instr];
		   			count++;
		   		}
		   		else{
		   			instr = segment->instruction;
		   			sprintf(str, " (%d/%d) %s %s %s %s", count+1, total_instructions+1, roadmap_lang_get("Continue"), dist_str, unit_str, roadmap_lang_get(inst_text));
		   			navigate_list_labels[count] = strdup(str);
		   			navigate_list_values[count] = (navigate_list_value *) malloc(sizeof(navigate_list_value));
		   			navigate_list_values[count]->str = strdup(str);
		   			navigate_list_values[count]->inst_num = count;
		   			navigate_list_values[count]->icon = NAVIGATE_DIR_IMG[instr];
		   			if (Nextsegment->line_direction == ROUTE_DIRECTION_WITH_LINE){
		   				navigate_list_values[count]->position.longitude = segment->to_pos.longitude;
		   				navigate_list_values[count]->position.latitude = segment->to_pos.latitude;
		   			}
		   			else{
		   				navigate_list_values[count]->position.longitude = segment->from_pos.longitude;
		   				navigate_list_values[count]->position.latitude = segment->from_pos.latitude;
		   			}
		   			navigate_list_icons[count] = NAVIGATE_DIR_IMG[instr];
		   			count++;
		   		}

		   	}

		   	if (segment->group_id != Nextsegment->group_id){
		   		distance_to_next = 0;
		   	}

		   	group_id = segment->group_id;
	    		segment_idx++;
   		}
	}
	navigate_list_count = count;
	ssd_generic_icon_list_dialog_show (roadmap_lang_get ("Navigation list"),
                  count,
                  (const char **)navigate_list_labels,
                  (const void **)navigate_list_values,
                  (const char **)navigate_list_icons,
                  NULL,
                  navigate_main_list_call_back,
                  NULL,
                  NULL,
                  NULL,
                  NULL,
                  60,
                  0,
                  FALSE);


}

/* Allows other windows to be closed */
static void navigate_progress_message_delayed(void)
{
	roadmap_main_remove_periodic( navigate_progress_message_delayed );
	if( CalculatingRoute )
	{
		ssd_progress_msg_dialog_show( roadmap_lang_get( "Calculating route, please wait..." ) );
	}
	else if( ReCalculatingRoute )
	{
	   static RoadMapSoundList list;
	   static char twoLineMessage[256];

	    if (!list) {
	       list = roadmap_sound_list_create (SOUND_LIST_NO_FREE);
	       roadmap_sound_list_add (list, "TickerPoints");
	       roadmap_res_get (RES_SOUND, 0, "TickerPoints");
	    }

	   roadmap_sound_play_list (list);
	   snprintf (twoLineMessage, sizeof(twoLineMessage), "%s\n%s",
	   				roadmap_lang_get( "Driving off track:" ),
	   				roadmap_lang_get( "Recalculating route..." ));
		ssd_progress_msg_dialog_show( twoLineMessage );
	}
}

static void navigate_progress_message_hide_delayed(void)
{
	roadmap_main_remove_periodic( navigate_progress_message_hide_delayed );
	ssd_progress_msg_dialog_hide();
	if (CalculatingRoute)
	{
		roadmap_messagebox ("Error", "Routing service timed out");
		navigate_route_cancel_request();
		CalculatingRoute = FALSE;
	}
}
