/* logging.c -- Definitions for signatures appearing in logging.h.
 *
 *
 * SCL; August 2012.
 */


#include "logging.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>


#define TIMESTAMP_LEN 32
#define FILENAME_LEN 256
char logfilename[FILENAME_LEN];
FILE *logfp = NULL;
int logopt = 0;


void setlogstream( FILE *fp )
{
	if (closelogfile())
		return;
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


FILE *openlogfile()
{
	struct tm *timeptr;
	time_t clock;

	clock = time( NULL );
	timeptr = gmtime( &clock );  /* UTC */
	if (strftime( logfilename, FILENAME_LEN, "gr1clog-%Y%m%d.txt", timeptr ) == 0) {
		fprintf( stderr, "ERROR: strftime() failed to create timestamp." );
		return NULL;
	}
	logfilename[FILENAME_LEN-1] = '\0';
	
	if (closelogfile())
		return NULL;
	
	logfp = fopen( logfilename, "a" );
	if (logfp == NULL) {
		perror( "openlogfile, fopen" );
		return NULL;
	}

	return logfp;
}

int closelogfile()
{
	if (logfp != NULL && logfp != stdout) {
		if (fclose( logfp ) == EOF) {
			perror( "closelogfile, fclose" );
			return -1;
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
		if (strftime( timestamp, TIMESTAMP_LEN, "%Y-%m-%d %H:%M:%S", timeptr ) == 0) {
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
		if (strftime( timestamp, TIMESTAMP_LEN, "%Y-%m-%d %H:%M:%S", timeptr ) == 0) {
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
