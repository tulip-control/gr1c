/* Unit tests for the logger.
 *
 * SCL; August 2012.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "tests_common.h"
#include "logging.h"


#define STRING_MAXLEN 1024
int main( int argc, char **argv )
{
	char *result;
	FILE *fp;
	char filename[STRING_MAXLEN];
	char data[STRING_MAXLEN];

	strcpy( filename, "temp_logging_dumpXXXXXX" );
	result = mktemp( filename );
	if (result == NULL) {
		perror( "test_logging, mktemp" );
		abort();
	}
	fp = fopen( filename, "w+" );
	if (fp == NULL) {
		perror( "test_logging, fopen" );
		abort();
	}

	setlogstream( fp );

	logprint( "Hello, world!" );
	logprint( "%d + %d = %f", 2, 3, 5. );
	logprint( "%s %d", "ok thx bai", -1 );

	if (fseek( fp, 0, SEEK_SET )) {
		perror( "test_logging, fseek" );
		abort();
	}
	result = fgets( data, STRING_MAXLEN, fp );
	if (result == NULL) {
		perror( "test_logging, fgets" );
		abort();
	}
	result = strstr( data, "Hello, world!" );
	if (result == NULL) {
		ERRPRINT( "unexpected log output while testing." );
		abort();
	}

	result = fgets( data, STRING_MAXLEN, fp );
	if (result == NULL) {
		perror( "test_logging, fgets" );
		abort();
	}
	result = strstr( data, "2 + 3 = 5.00" );
	if (result == NULL) {
		ERRPRINT( "unexpected log output while testing." );
		abort();
	}

	result = fgets( data, STRING_MAXLEN, fp );
	if (result == NULL) {
		perror( "test_logging, fgets" );
		abort();
	}
	result = strstr( data, "ok thx bai -1" );
	if (result == NULL) {
		ERRPRINT( "unexpected log output while testing." );
		abort();
	}

	if (closelogfile()) {
		ERRPRINT( "failed to close log file for testing." );
		abort();
	}
	if (remove( filename )) {
		perror( "test_logging, remove" );
		abort();
	}
	return 0;
}
