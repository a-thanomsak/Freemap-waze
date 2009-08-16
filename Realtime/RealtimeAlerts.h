/* RealtimeAlerts.c - Manage real time alerts
 *
 * LICENSE:
 *
 *   Copyright 2008 Avi B.S
 *   Copyright 2008 Ehud Shabtai
 *
 *   This file is part of RoadMap.
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License V2 as published by
 *   the Free Software Foundation.
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
 *
 */

#ifndef __REALTIME_ALERT_H__
#define __REALTIME_ALERT_H__

#include "../roadmap_sound.h"

// Alerts types
#define RT_ALERT_TYPE_CHIT_CHAT             0
#define RT_ALERT_TYPE_POLICE                1
#define RT_ALERT_TYPE_ACCIDENT              2
#define RT_ALERT_TYPE_TRAFFIC_JAM           3
#define RT_ALERT_TYPE_TRAFFIC_INFO          4
#define RT_ALERT_TYPE_HAZARD                5
#define RT_ALERT_TYPE_OTHER                 6
#define RT_ALERT_TYPE_CONSTRUCTION          7
#define RT_ALERTS_LAST_KNOWN_STATE          7

//Alerts direction
#define RT_ALERT_BOTH_DIRECTIONS            0
#define RT_ALERT_MY_DIRECTION               1
#define RT_ALERT_OPPSOITE_DIRECTION         2

#define RT_MAXIMUM_ALERT_COUNT              500
#define RT_ALERT_LOCATION_MAX_SIZE          150
#define RT_ALERT_DESCRIPTION_MAXSIZE        150
#define RT_ALERT_USERNM_MAXSIZE             50

#define REROUTE_MIN_DISTANCE 30000

#define STATE_NONE         -1
#define STATE_OLD          1
#define STATE_NEW          2
#define STATE_NEW_COMMENT  3
#define STATE_SCROLLING    4
#ifdef _WIN32
#define NEW_LINE "\r\n"
#else
#define NEW_LINE "\n"
#endif


//////////////////////////////////////////////////////////////////////////////////////////////////
// Sort Method
typedef enum alert_sort_method
{
   sort_proximity,
   sort_recency
}  alert_sort_method;


//////////////////////////////////////////////////////////////////////////////////////////////////
//  Comment
typedef struct
{
    int iID; // Comment ID (within the server)
    int iAlertId; //  Alert Type
    long long i64ReportTime; // The time the comment was posted
    char sPostedBy [RT_ALERT_USERNM_MAXSIZE+1]; // The User who posted the comment
    char sDescription [RT_ALERT_DESCRIPTION_MAXSIZE+1]; // The comment
    BOOL bCommentByMe;
} RTAlertComment;

//////////////////////////////////////////////////////////////////////////////////////////////////
//  Comments List
typedef struct RTAlertCommentsEntry_s
{
    struct RTAlertCommentsEntry_s *next;
    struct RTAlertCommentsEntry_s *previous;
    RTAlertComment comment;
} RTAlertCommentsEntry;

//////////////////////////////////////////////////////////////////////////////////////////////////
//  Alert
typedef struct
{
    int iID; // Alert ID (within the server)
    int iType; //  Alert Type
    int iDirection; //  0 – node's direction, 1 – segment direction, 2 – opposite direction
    int iLongitude; //  Alert location: Longtitue
    int iLatitude; //   Alert location: Latitude
    int iAzymuth; //    Alert Azimuth
    int iSpeed; // Alowed speed to alert
    char sReportedBy [RT_ALERT_USERNM_MAXSIZE+1]; // The User who reported the alert
    long long i64ReportTime; // The time the alert was reported
    int iNode1; // Alert's segment
    int iNode2; // Alert's segment
    char sDescription [RT_ALERT_DESCRIPTION_MAXSIZE+1]; // Alert's description
    char sLocationStr[RT_ALERT_LOCATION_MAX_SIZE+1]; //alert location
    int iDistance;
    int iLineId;
    int  iSquare;
    int iNumComments;
    BOOL bAlertByMe;
    RTAlertCommentsEntry *Comment;

} RTAlert;

//////////////////////////////////////////////////////////////////////////////////////////////////
//  Runtime Alert Table
typedef struct
{
    RTAlert *alert[RT_MAXIMUM_ALERT_COUNT];
    int iCount;
} RTAlerts;

void RTAlerts_Alert_Init(RTAlert *alert);
void RTAlerts_Clear_All(void);
void RTAlerts_Init(void);
void RTAlerts_Term(void);
BOOL RTAlerts_Add(RTAlert *alert);
BOOL RTAlerts_Remove(int iID);
int RTAlerts_Count(void);
const char * RTAlerts_Count_Str(void);
RTAlert *RTAlerts_Get(int record);
RTAlert *RTAlerts_Get_By_ID(int iID);
BOOL RTAlerts_Is_Empty();
BOOL RTAlerts_Exists(int iID);
void RTAlerts_Get_Position(int alert, RoadMapPosition *position, int *steering);
int RTAlerts_Get_Type(int record);
int RTAlerts_Get_Id(int record);
char *RTAlerts_Get_LocationStr(int record);
unsigned int RTAlerts_Get_Speed(int record);
int RTAlerts_Get_Distance(int record);
const char * RTAlerts_Get_Map_Icon(int alert);
const char * RTAlerts_Get_Icon(int alertId);
const char * RTAlerts_Get_Alert_Icon(int alertId);
const char * RTAlerts_Get_Warn_Icon(int alertId);
const char * RTAlerts_Get_String(int alertId);
RoadMapSoundList RTAlerts_Get_Sound(int alertId);
int RTAlerts_Is_Alertable(int record);
void RTAlerts_Sort_List(alert_sort_method sort_method);
void RTAlerts_Get_City_Street(RoadMapPosition AlertPosition,
        const char **city_name, const char **street_name,  int *square, int*line_id,
        int direction);
void RTAlerts_Popup_By_Id(int alertId);
void RTAlerts_Popup_By_Id_No_Center(int iID);
void RTAlerts_Scroll_All(void);
void RTAlerts_Stop_Scrolling(void);
void RTAlerts_Scroll_Next(void);
void RTAlerts_Scroll_Prev(void);
void RTAlerts_Cancel_Scrolling(void);
BOOL RTAlerts_Zoomin_To_Aler(void);
int RTAlerts_State(void);
int RTAlerts_Get_Current_Alert_Id(void);
int RTAlerts_Pernalty(int line_id, int against_dir);
void RTAlerts_Delete_All_Comments(RTAlert *alert);

void RTAlerts_Comment_Init(RTAlertComment *comment);
BOOL RTAlerts_Comment_Add(RTAlertComment *comment);
int RTAlerts_Get_Number_of_Comments(int iAlertId);

void RealtimeAlertsMenu(void);
int RTAlerts_Alert_near_position( RoadMapPosition position, int distance);

void add_real_time_alert_for_police(void);
void add_real_time_alert_for_accident(void);
void add_real_time_alert_for_traffic_jam(void);
void add_real_time_alert_for_hazard(void);
void add_real_time_alert_for_other(void);
void add_real_time_alert_for_construction(void);
void add_real_time_alert_for_police_my_direction(void);
void add_real_time_alert_for_accident_my_direction(void);
void add_real_time_alert_for_traffic_jam_my_direction(void);
void add_real_time_alert_for_hazard_my_direction(void);
void add_real_time_alert_for_other_my_direction(void);
void add_real_time_alert_for_construction_my_direction(void);
void add_real_time_alert_for_police_opposite_direction(void);
void add_real_time_alert_for_accident_opposite_direction(void);
void add_real_time_alert_for_traffic_jam_opposite_direction(void);
void add_real_time_alert_for_hazard_opposite_direction(void);
void add_real_time_alert_for_other_opposite_direction(void);
void add_real_time_alert_for_construction_opposite_direction(void);
void add_real_time_chit_chat(void);
void real_time_post_alert_comment(void);
void real_time_post_alert_comment_by_id(int iAlertId);
BOOL real_time_remove_alert(int iAlertId);
int RTAlerts_ScrollState(void);
int RTAlerts_Is_Cancelable(int alertId);
int Rtalerts_Delete(int alertId);

#endif  //  __REALTIME_ALERT_H__
