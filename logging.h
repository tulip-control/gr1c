/** \file logging.h
 * \brief A simple logging mechanism for gr1c.
 *
 * 
 * SCL; August 2012.
 */


#ifndef LOGGING_H
#define LOGGING_H


#include <stdio.h>
#include "common.h"


/** Use given stream for logging.  If NULL, then use stdout. */
void setlogstream( FILE *fp );

/** Open a new log file, automatically name it with the current
   timestamp, and return the resulting stream, or NULL if error.
   If the file already exists, then it is appended to. */
FILE *openlogfile();

/** Manually close the current log stream, unless it is stdout.
   Return 0 on success, -1 on error. */
int closelogfile();

/** Rudimentary printing routine. Supports %d, %f, and %s flags. */
void logprint( char *fmt, ... );


#endif
