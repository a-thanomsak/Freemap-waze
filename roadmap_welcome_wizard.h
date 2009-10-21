/* roadmap_welcome_wizard.h - First time settup wizard
 *
 * LICENSE:
 *
 *   Copyright 2008 Avi B.S
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

#include "roadmap.h"

#define  WELCOME_WIZ_CONFIG_TYPE	   ("user")
#define  WELCOME_WIZ_ENABLE_CONFIG_TYPE  ("preferences")
#define  WELCOME_WIZ_TAB            ("Welcome Wizard")

//   First Time
#define  WELOCME_WIZ_ENABLED_Name       ("Enabled")
#define  WELCOME_WIZ_FIRST_TIME_Name   ("First time")
#define  WELCOME_WIZ_TERMS_OF_USE_Name   ("Terms of Use accepted")
#define  WELCOME_WIZ_MINUTES_FOR_ACTIVATION_Name ("Minutes for activation")
#define  WELCOME_WIZ_FIRST_TIME_Yes    ("yes")
#define  WELCOME_WIZ_FIRST_TIME_No     ("no")
#define  WELCOME_WIZ_MINUTES_FOR_ACTIVATION_Defaut ("1440")

void roadmap_welcome_wizard(void);
BOOL roadmap_welcome_on_preferences( void );
void welcome_wizard_twitter_dialog(void);
void roadmap_welcome_personalize_dialog();
void roadmap_term_of_use(RoadMapCallback callback);
