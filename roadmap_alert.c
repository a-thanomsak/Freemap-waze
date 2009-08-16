/* roadmap_alert.c - Manage the alert points in DB.
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
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "roadmap.h"
#include "roadmap_dbread.h"
#include "roadmap_tile_model.h"
#include "roadmap_db_alert.h"
#include "roadmap_layer.h"
#include "roadmap_config.h"
#include "roadmap_sound.h"
#include "editor/static/add_alert.h"

#include "roadmap_alert.h"
#include "roadmap_alerter.h"

#define ALERT_AUDIO "ApproachSpeedCam"
#define ALERT_ICON      "speed_cam_alert.png"
#define WARN_ICON       "speed_cam_warn.png"


static void *roadmap_alert_map (const roadmap_db_data_file *file);
static void roadmap_alert_activate (void *context);
static void roadmap_alert_unmap (void *context);
static int roadmap_alert_is_cancelable(int alertId);
static int roadmap_alert_cancel(int alertId);
static RoadMapConfigDescriptor AlertDistanceCfg =
            ROADMAP_CONFIG_ITEM("Alerts", "Alert Distance");


static char *RoadMapAlertType = "RoadMapAlertContext";
typedef struct {
   char *type;
   RoadMapAlert *Alert;
   int           AlertCount;
} RoadMapAlertContext;

static RoadMapAlertContext *RoadMapAlertActive = NULL;

roadmap_db_handler RoadMapAlertHandler = {
   "alert",
   roadmap_alert_map,
   roadmap_alert_activate,
   roadmap_alert_unmap
};

roadmap_alert_providor RoadmapAlertProvidor = {
   "alertDb",
   roadmap_alert_count,
   roadmap_alert_get_id,
   roadmap_alert_get_position,
   roadmap_alert_get_speed,
   roadmap_alert_get_map_icon,
   roadmap_alert_get_alert_icon,
   roadmap_alert_get_warning_icon,
   roadmap_alert_get_distance,
   roadmap_alert_get_alert_sound,
   roadmap_alert_alertable,
   roadmap_alert_get_string,
   roadmap_alert_is_cancelable,
   roadmap_alert_cancel
};
int roadmap_alert_is_cancelable(int alertId){
    return TRUE;
}

int roadmap_alert_cancel(int alertId){
    request_speed_cam_delete();
   return TRUE;
}



static void *roadmap_alert_map (const roadmap_db_data_file *file) {

   RoadMapAlertContext *context;

   context = malloc(sizeof(RoadMapAlertContext));
   roadmap_check_allocated(context);

   context->type = RoadMapAlertType;

    if (!roadmap_db_get_data (file,
                                      model__tile_alert_data,
                                      sizeof(RoadMapAlert),
                                      (void **)&(context->Alert),
                                      &(context->AlertCount))) {

      roadmap_log (ROADMAP_FATAL, "invalid alert/data structure");
   }

    //Distance to alert
    roadmap_config_declare
    ("preferences", &AlertDistanceCfg, "400", NULL);

   return context;
}

static void roadmap_alert_activate (void *context) {

   RoadMapAlertContext *alert_context = (RoadMapAlertContext *) context;

   if (alert_context != NULL) {

      if (alert_context->type != RoadMapAlertType) {
         roadmap_log (ROADMAP_FATAL, "cannot activate alert (bad type)");
      }
   }

   RoadMapAlertActive = alert_context;
}

static void roadmap_alert_unmap (void *context) {

   RoadMapAlertContext *alert_context = (RoadMapAlertContext *) context;

   if (alert_context->type != RoadMapAlertType) {
      roadmap_log (ROADMAP_FATAL, "cannot activate alert (bad type)");
   }
   if (RoadMapAlertActive == alert_context) {
      RoadMapAlertActive = NULL;
   }
   free(alert_context);
}


int roadmap_alert_count (void) {

   if (RoadMapAlertActive == NULL) {
      return 0;
   }

   return RoadMapAlertActive->AlertCount;
}

RoadMapAlert * roadmap_alert_get_alert(int record){
   return RoadMapAlertActive->Alert + record;

}

RoadMapAlert *roadmap_alert_get_alert_by_id( int id)
{
    int i;
    RoadMapAlert *alert;

    //  Find alert:
    for( i=0; i< RoadMapAlertActive->AlertCount; i++){
        alert = roadmap_alert_get_alert(i);
        if( alert->id == id)
            return alert;
    }
    return NULL;
}


void roadmap_alert_get_position(int alert, RoadMapPosition *position, int *steering) {

   RoadMapAlert *alert_st = roadmap_alert_get_alert (alert);
   assert(alert_st != NULL);

   position->longitude = alert_st->pos.longitude;
   position->latitude = alert_st->pos.latitude;

   *steering = alert_st->steering;
}


unsigned int roadmap_alert_get_speed(int alert){
   RoadMapAlert *alert_st = roadmap_alert_get_alert (alert);
   assert(alert_st != NULL);
   return (int) alert_st->speed;
}

int roadmap_alert_get_category(int alert) {

   RoadMapAlert *alert_st = roadmap_alert_get_alert (alert);
   assert(alert_st != NULL);

   return (int) alert_st->category;
}


int roadmap_alert_get_id(int alert){
   RoadMapAlert *alert_st = roadmap_alert_get_alert (alert);
   assert(alert_st != NULL);

   return (int) alert_st->id;
}

// check if an alert should be generated for a specific category
int roadmap_alert_alertable(int record){

    int alert_category = roadmap_alert_get_category(record);

    switch (alert_category) {
    case ALERT_CATEGORY_SPEED_CAM:
        return 1;
    default:
        return 0;
    }
}

const char *  roadmap_alert_get_string(int id){
    return "Speed trap" ;
}

const char * roadmap_alert_get_map_icon(int id){

    RoadMapAlert *alert_st  = roadmap_alert_get_alert_by_id(id);

    switch (alert_st->category) {
    case ALERT_CATEGORY_SPEED_CAM:
        return "rm_speed_cam.png" ;
    case ALERT_CATEGORY_DUMMY_SPEED_CAM:
        return "rm_dummy_speed_cam.png";
    default:
        return  NULL;
    }
}

int roadmap_alert_get_distance(int record){
    return roadmap_config_get_integer(&AlertDistanceCfg);
}

const char *roadmap_alert_get_alert_icon(int Id){
    return ALERT_ICON;
}

const char *roadmap_alert_get_warning_icon(int Id){
    return WARN_ICON;
}

RoadMapSoundList  roadmap_alert_get_alert_sound(int Id){

    RoadMapSoundList sound_list;
        sound_list = roadmap_sound_list_create (0);
        roadmap_sound_list_add (sound_list,  ALERT_AUDIO);
        return sound_list;
}
