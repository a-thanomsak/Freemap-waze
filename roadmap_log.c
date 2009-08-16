/* roadmap_log.c - a module for managing uniform error & info messages.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
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
 *   #include "roadmap.h"
 *
 *   void roadmap_log (int level, char *format, ...);
 *
 * This module is used to control and manage the appearance of messages
 * printed by the roadmap program. The goals are (1) to produce a uniform
 * look, (2) have a central point of control for error management and
 * (3) have a centralized control for routing messages.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <assert.h>

#include "roadmap.h"
#include "roadmap_path.h"
#include "roadmap_file.h"
#include "roadmap_messagebox.h"

#if defined(IPHONE) || defined(unix)
#include <sys/timeb.h>
#endif

#ifdef   FREEZE_ON_FATAL_ERROR
   #pragma message("    In case of fatal error process will freeze and wait for debugger")
#endif   // FREEZE_ON_FATAL_ERROR

#define ROADMAP_LOG_STACK_SIZE 256

static const char *RoadMapLogStack[ROADMAP_LOG_STACK_SIZE];
static int         RoadMapLogStackCursor = 0;

static struct roadmap_message_descriptor {
   int   level;
   int   show_stack;
   int   save_to_file;
   int   do_exit;
   char *prefix;
} RoadMapMessageHead [] = {
   {ROADMAP_MESSAGE_DEBUG,   0, 1, 0, "..debug.."},
   {ROADMAP_MESSAGE_INFO,    0, 1, 0, "Info --->"},
   {ROADMAP_MESSAGE_WARNING, 0, 1, 0, ":WARNING:"},
   {ROADMAP_MESSAGE_ERROR,   1, 1, 0, "ERROR !!!"},
   {ROADMAP_MESSAGE_FATAL,   1, 1, 1, "[???????]"},
   {0,                       1, 1, 1, "??"}
};


static roadmap_log_msgbox_handler RoadmapLogMsgBox;

void roadmap_log_register_msgbox (roadmap_log_msgbox_handler handler) {
   RoadmapLogMsgBox = handler;
}


void roadmap_log_push (const char *description) {

   if (RoadMapLogStackCursor < ROADMAP_LOG_STACK_SIZE) {
      RoadMapLogStack[RoadMapLogStackCursor++] = description;
   }
}

void roadmap_log_pop (void) {

   if (RoadMapLogStackCursor > 0) {
      RoadMapLogStackCursor -= 1;
   }
}

void roadmap_log_reset_stack (void) {

   RoadMapLogStackCursor = 0;
}


void roadmap_log_save_all (void) {

    int i;

    for (i = 0; RoadMapMessageHead[i].level > 0; ++i) {
        RoadMapMessageHead[i].save_to_file = 1;
    }
}


void roadmap_log_save_none (void) {

    int i;

    for (i = 0; RoadMapMessageHead[i].level > 0; ++i) {
        RoadMapMessageHead[i].save_to_file = 0;
    }
    roadmap_log_purge();
}


int  roadmap_log_enabled (int level, char *source, int line) {
   return (level >= roadmap_verbosity());
}


static void roadmap_log_one (struct roadmap_message_descriptor *category,
                             FILE *file,
                             char  saved,
                             char *source,
                             int line,
                             char *format,
                             va_list ap) {

   int i;

#ifdef J2ME
   fprintf (file, "%d %c%s %s, line %d ",
         time(NULL), saved, category->prefix, source, line);

#elif defined (__SYMBIAN32__)
   time_t now;
   struct tm *tms;

   time (&now);
   tms = localtime (&now);

   fprintf (file, "%02d:%02d:%02d %c%s %s, line %d ",
         tms->tm_hour, tms->tm_min, tms->tm_sec,
         saved, category->prefix, source, line);

#elif defined (_WIN32)

   SYSTEMTIME st;
   GetSystemTime(&st);

   fprintf (file, "%02d/%02d %02d:%02d:%02d %s\t",
         st.wDay, st.wMonth, st.wHour, st.wMinute, st.wSecond,
         category->prefix);

#else
   struct tm *tms;
   struct timeb tp;

   ftime(&tp);
   tms = localtime (&tp.time);

   fprintf (file, "%02d:%02d:%02d.%03d %c%s %s, line %d ",
         tms->tm_hour, tms->tm_min, tms->tm_sec, tp.millitm,
         saved, category->prefix, source, line);
#endif
   if (!category->show_stack && (RoadMapLogStackCursor > 0)) {
      fprintf (file, "(%s): ", RoadMapLogStack[RoadMapLogStackCursor-1]);
   }

#ifdef _DEBUG
   if( format && (*format) && ('\n' == format[strlen(format)-1]))
      assert(0);  // Please remove '\n' from logged message end...
#endif   // _DEBUG

   vfprintf(file, format, ap);
   fprintf( file, " \t[File: '%s'; Line: %d]\n", source, line);

   if (category->show_stack && RoadMapLogStackCursor > 0) {

      int indent = 8;

      fprintf (file, "   Call stack:\n");

      for (i = 0; i < RoadMapLogStackCursor; ++i) {
          fprintf (file, "%*.*s %s\n", indent, indent, "", RoadMapLogStack[i]);
          indent += 3;
      }
   }

   if (RoadmapLogMsgBox && category->do_exit) {
      char str[256];
      char msg[256];
#ifdef   FREEZE_ON_FATAL_ERROR
      const char* title = "Fatal Error - Process awaits debugger";

#else
      const char* title = "Fatal Error";

#endif   // FREEZE_ON_FATAL_ERROR

      vsprintf(msg, format, ap);
      sprintf (str, "%c%s %s, line %d %s",
         saved, category->prefix, source, line, msg);
      RoadmapLogMsgBox(title, str);
   }
}

void roadmap_log (int level, char *source, int line, char *format, ...) {

   static FILE *file;
   va_list ap;
   char saved = ' ';
   struct roadmap_message_descriptor *category;
   char *debug;

   if (level < roadmap_verbosity()) return;

#if(defined DEBUG && defined SKIP_DEBUG_LOGS)
   return;
#endif   // SKIP_DEBUG_LOGS

   debug = roadmap_debug();

   if ((debug[0] != 0) && (strcmp (debug, source) != 0)) return;

   for (category = RoadMapMessageHead; category->level != 0; ++category) {
      if (category->level == level) break;
   }

   va_start(ap, format);

   if (category->save_to_file) {
      static int open_file_attemped = 0;

      if ((file == NULL) && (!open_file_attemped)) {
         open_file_attemped = 1;

#if defined (__SYMBIAN32__)
#if defined (WINSCW)
         file = roadmap_file_fopen ("C:\\", "waze_log.txt", "w");
#else
         file = roadmap_file_fopen ("E:\\", "waze_log.txt", "w");
#endif
#elif !defined (J2ME)
         file = roadmap_file_fopen (roadmap_path_user(), "postmortem", "sa");
#else
         //file = roadmap_file_fopen ("file:///e:/FreeMap", "logger.txt", "w");
#endif
         if (file) fprintf (file, "*** Starting log file %d ***", (int)time(NULL));
      }

      if (file != NULL) {

         roadmap_log_one (category, file, ' ', source, line, format, ap);
         fflush (file);
         //fclose (file);

         va_end(ap);
         va_start(ap, format);

         saved = 's';
      }
   }

#ifdef __SYMBIAN32__
   //roadmap_log_one (category, __stderr(), saved, source, line, format, ap);
#else
   roadmap_log_one (category, stderr, saved, source, line, format, ap);
#endif

   va_end(ap);

   if( category->do_exit)
#ifdef FREEZE_ON_FATAL_ERROR
   {
      int beep_times =   20;
      int sleep_time = 1000;

      do
      {
         Sleep( sleep_time);

         if( beep_times)
         {
            fprintf( file, ">>> FATAL ERROR - WAITING FOR PROCESS TO BE ATTACHED BY A DEBUGGER...\r\n");
            MessageBeep(MB_OK);
            beep_times--;

            if(!beep_times)
               sleep_time = 5000;
         }

      }  while(1);
   }

#else
      exit(1);

#endif   // FREEZE_ON_FATAL_ERROR
}


void roadmap_log_purge (void) {

    roadmap_file_remove (roadmap_path_user(), "postmortem");
}


void roadmap_check_allocated_with_source_line
                (char *source, int line, const void *allocated) {

    if (allocated == NULL) {
        roadmap_log (ROADMAP_MESSAGE_FATAL, source, line, "no more memory");
    }
}
