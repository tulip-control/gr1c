/** \file logging.h
 * \brief A simple logging mechanism for gr1c.
 *
 *
 * SCL; 2012, 2013.
 */


#ifndef LOGGING_H
#define LOGGING_H


#include <stdio.h>
#include "common.h"


/** Use given stream for logging.  If NULL, then use stdout. */
void setlogstream( FILE *fp );

FILE *getlogstream();


/** Configure format of log entries.

   Combine non-conflicting options with or.  Options can be

       LOGOPT_TIME : Show timestamp (default).

       LOGOPT_NOTIME : Do not show timestamp. */
void setlogopt( int options );
#define LOGOPT_TIME 0
#define LOGOPT_NOTIME 1

/** Get current log output configuration */
int getlogopt();


/** Open a new log file, automatically name it with the current date and given
   prefix, and return the resulting stream, or NULL if error.  If the file
   already exists, then it is appended to.  If prefix=NULL, then use "gr1c-" and
   the PID of the calling process. Name ends with ".log" */
FILE *openlogfile( char *prefix );

/** Manually close the current log stream, unless it is stdout.
   Return 0 on success, -1 on error. */
int closelogfile();

/** Rudimentary printing routine. Supports %d, %f, and %s flags. */
void logprint( char *fmt, ... );

/** Write to log stream.  Do not automatically end line, do not flush, etc. */
void logprint_raw( char *fmt, ... );

/** Start a new line.  Intended for use with logprint_raw() and
   logprint_endline(). */
void logprint_startline();

/** End line and flush log stream.  Intended for use with
   logprint_raw() and logprint_startline(). */
void logprint_endline();


#endif
