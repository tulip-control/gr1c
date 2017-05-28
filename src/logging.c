/* logging.c -- Definitions for signatures appearing in logging.h.
 *
 *
 * SCL; 2012-2015
 */


#define _POSIX_C_SOURCE 200809L
#include "logging.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <sys/types.h>
#include <unistd.h>


#define TIMESTAMP_LEN 32
#define FILENAME_LEN 256
char logfilename[FILENAME_LEN];
FILE *logfp = NULL;
int logopt = 0;


void setlogstream( FILE *fp )
{
    *logfilename = '\0';
    if (fp == NULL) {
        logfp = stdout;
    } else {
        logfp = fp;
    }
}

FILE *getlogstream()
{
    return logfp;
}


void setlogopt( int options )
{
    logopt = options;
}

int getlogopt()
{
    return logopt;
}


FILE *openlogfile( char *prefix )
{
    struct tm *timeptr;
    time_t clock;

    if (prefix != NULL) {
        strncpy( logfilename, prefix, FILENAME_LEN );
    } else {
        strncpy( logfilename, "gr1c", FILENAME_LEN );
        snprintf( logfilename+4, FILENAME_LEN-4, "-%d", getpid() );
    }
    logfilename[FILENAME_LEN-1] = '\0';

    clock = time( NULL );
    timeptr = gmtime( &clock );  /* UTC */
    if (strftime( logfilename+strlen(logfilename),
                  FILENAME_LEN-strlen(logfilename),
                  "-%Y%m%d.log", timeptr ) == 0) {
        fprintf( stderr, "ERROR: strftime() failed to create timestamp." );
        return NULL;
    }
    logfilename[FILENAME_LEN-1] = '\0';

    if (closelogfile())
        return NULL;

    logfp = fopen( logfilename, "a" );
    if (logfp == NULL) {
        perror( __FILE__ ",  fopen" );
        exit(-1);
    }

    return logfp;
}

int closelogfile()
{
    if (logfp != NULL && logfp != stdout) {
        if (fclose( logfp ) == EOF) {
            perror( __FILE__ ",  fclose" );
            exit(-1);
        }
    }
    return 0;
}

void logprint( char *fmt, ... )
{
    va_list ap;
    struct tm *timeptr;
    time_t clock;
    char timestamp[TIMESTAMP_LEN];

    if (logfp == NULL) {
        fprintf( stderr, "WARNING: Attempted to write to non-existent log." );
        return;
    }

    if ((logopt & 0x1) == LOGOPT_TIME) {
        clock = time( NULL );
        timeptr = gmtime( &clock );  /* UTC */
        if (strftime( timestamp, TIMESTAMP_LEN,
                      "%Y-%m-%d %H:%M:%S", timeptr ) == 0) {
            fprintf( stderr, "ERROR: strftime() failed to create timestamp." );
            return;
        }
        fprintf( logfp, "%s ", timestamp );
    }

    va_start( ap, fmt );

    while (*fmt != '\0') {
        if (*fmt == '%') {
            fmt++;
            switch(*fmt) {
            case 'f':
                fprintf( logfp, "%f", va_arg( ap, double ) );
                break;
            case 'd':
                fprintf( logfp, "%d", va_arg( ap, int ) );
                break;
            case 's':
                fprintf( logfp, "%s", va_arg( ap, char * ) );
                break;
            default:
                fputc( *fmt, logfp );
            }
        } else {
            fputc( *fmt, logfp );
        }
        fmt++;
    }
    fputc( '\n', logfp );

    va_end( ap );
    fflush( logfp );
}


void logprint_startline()
{
    struct tm *timeptr;
    time_t clock;
    char timestamp[TIMESTAMP_LEN];

    if (logfp == NULL) {
        fprintf( stderr, "WARNING: Attempted to write to non-existent log." );
        return;
    }

    if ((logopt & 0x1) == LOGOPT_TIME) {
        clock = time( NULL );
        timeptr = gmtime( &clock );  /* UTC */
        if (strftime( timestamp, TIMESTAMP_LEN,
                      "%Y-%m-%d %H:%M:%S", timeptr ) == 0) {
            fprintf( stderr, "ERROR: strftime() failed to create timestamp." );
            return;
        }
        fprintf( logfp, "%s ", timestamp );
    }
}

void logprint_endline()
{
    fputc( '\n', logfp );
    fflush( logfp );
}

void logprint_raw( char *fmt, ... )
{
    va_list ap;

    if (logfp == NULL) {
        fprintf( stderr, "WARNING: Attempted to write to non-existent log." );
        return;
    }

    va_start( ap, fmt );
    while (*fmt != '\0') {
        if (*fmt == '%') {
            fmt++;
            switch(*fmt) {
            case 'f':
                fprintf( logfp, "%f", va_arg( ap, double ) );
                break;
            case 'd':
                fprintf( logfp, "%d", va_arg( ap, int ) );
                break;
            case 's':
                fprintf( logfp, "%s", va_arg( ap, char * ) );
                break;
            default:
                fputc( *fmt, logfp );
            }
        } else {
            fputc( *fmt, logfp );
        }
        fmt++;
    }

    va_end( ap );
}
