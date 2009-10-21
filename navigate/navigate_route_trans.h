/* navigate_route_trans.h - handle routing transactions 
 *
 * LICENSE:
 *
 *   Copyright 2009 Israel Disatnik
 *
 *   This file is part of Waze.
 *
 *   Waze is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   Waze is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Waze; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef __NAVIGATE_ROUTE_TRANS_H
#define __NAVIGATE_ROUTE_TRANS_H

#include "websvc_trans/string_parser.h"
#include "navigate/navigate_main.h"
#include "roadmap_plugin.h"

#define	ROUTE_ORIGINAL		1
#define	ROUTE_UPDATE		2
#define	ROUTE_ALTERNATIVE	3

typedef struct {
	
	int						num_points;
	int						valid_points;
	RoadMapPosition		*points;
} NavigateRouteGeometry;

typedef struct {
	
	int						num_segments;
	int						num_received;
	int						num_valid;
	int						num_instrumented;
	int						next_to_instrument;
	int						num_first;
	int						num_last;
	NavigateSegment		*segments;
} NavigateRouteSegments;

typedef struct {
	
	int							flags;
	int							total_length;
	int							total_time;
	int							num_segments;
	int							alt_id;
	char							*description;
	int							route_status;
	NavigateRouteGeometry	geometry;
} NavigateRouteResult;

typedef enum {
	route_succeeded,
	route_server_error,
	route_inconsistent	
}	NavigateRouteRC;

typedef void (*NavigateOnRouteResults) (NavigateRouteRC rc, int num_res, const NavigateRouteResult *res);
typedef void (*NavigateOnRouteSegments) (NavigateRouteRC rc, const NavigateRouteResult *res, const NavigateRouteSegments *segments); 
typedef void (*NavigateOnRouteInstrumented) (int num_instrumented); 

const char *on_routing_response_code (/* IN  */   const char*       data, 
                          		   	  /* IN  */   void*             context,
                                 	  /* OUT */   BOOL*             more_data_needed,
                                 	  /* OUT */   roadmap_result*   rc);
                                 	  
const char *on_routing_response (/* IN  */   const char*       data, 
                          		   /* IN  */   void*             context,
                                 /* OUT */   BOOL*             more_data_needed,
                                 /* OUT */   roadmap_result*   rc);

const char *on_route_points (/* IN  */   const char*       data, 
                             /* IN  */   void*             context,
                             /* OUT */   BOOL*             more_data_needed,
                             /* OUT */   roadmap_result*   rc);

const char *on_route_segments (/* IN  */   const char*       data, 
                               /* IN  */   void*             context,
                               /* OUT */   BOOL*             more_data_needed,
                               /* OUT */   roadmap_result*   rc);

void navigate_route_request (const PluginLine *from_line,
                             int from_point,
                             const PluginLine *to_line,
                             const RoadMapPosition *from_pos,
                             const RoadMapPosition *to_pos,
                             const char *to_street,
                             int flags,
                             int trip_id,
                             int max_routes,
                             NavigateOnRouteResults cb_results,
                             NavigateOnRouteSegments cb_segments,
                             NavigateOnRouteInstrumented cb_instrumented);

void navigate_route_cancel_request (void);

void navigate_route_select (int alt_id);

#endif //__NAVIGATE_ROUTE_TRANS_H
