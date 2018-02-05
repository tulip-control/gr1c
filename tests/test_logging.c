/* Unit tests for the logger.
 *
 * SCL; 2012, 2015
 */

#define _POSIX_C_SOURCE 200809L
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
    int fd;
    FILE *fp;
    char filename[STRING_MAXLEN];
    char data[STRING_MAXLEN];

    strcpy( filename, "temp_logging_dumpXXXXXX" );
    fd = mkstemp( filename );
    if (fd == -1) {
        perror( __FILE__ ",  mkstemp" );
        abort();
    }
    fp = fdopen( fd, "w+" );
    if (fp == NULL) {
        perror( __FILE__ ",  fdopen" );
        abort();
    }

    setlogstream( NULL );
    if (getlogstream() != stdout) {
        ERRPRINT( "setlogstream( NULL ) failed to select stdout." );
        abort();
    }

    setlogstream( fp );
    if (getlogstream() != fp) {
        ERRPRINT1( "setlogstream() failed to select file pointer to %s.", filename );
        abort();
    }

    logprint( "Hello, world!" );
    logprint( "%d + %d = %f", 2, 3, 5. );
    logprint( "%s %d", "ok thx bai", -1 );

    logprint_startline();
    logprint_raw( "this is a multi-" );
    logprint_raw( "part entry, on one line" );
    logprint_endline();
    logprint_startline();
    logprint_raw( "%d this is another multi-part en", 5 );
    logprint_raw( "try,\nbut it spans two lines and has data: %s %f",
                  "like",
                  0.25 );
    logprint_endline();

    if (fseek( fp, 0, SEEK_SET )) {
        perror( __FILE__ ",  fseek" );
        abort();
    }
    result = fgets( data, STRING_MAXLEN, fp );
    if (result == NULL) {
        perror( __FILE__ ",  fgets" );
        abort();
    }
    result = strstr( data, "Hello, world!" );
    if (result == NULL) {
        ERRPRINT( "unexpected log output while testing." );
        abort();
    }

    result = fgets( data, STRING_MAXLEN, fp );
    if (result == NULL) {
        perror( __FILE__ ",  fgets" );
        abort();
    }
    result = strstr( data, "2 + 3 = 5.00" );
    if (result == NULL) {
        ERRPRINT( "unexpected log output while testing." );
        abort();
    }

    result = fgets( data, STRING_MAXLEN, fp );
    if (result == NULL) {
        perror( __FILE__ ",  fgets" );
        abort();
    }
    result = strstr( data, "ok thx bai -1" );
    if (result == NULL) {
        ERRPRINT( "unexpected log output while testing." );
        abort();
    }

    result = fgets( data, STRING_MAXLEN, fp );
    if (result == NULL) {
        perror( __FILE__ ",  fgets" );
        abort();
    }
    result = strstr( data, "this is a multi-part entry, on one line" );
    if (result == NULL) {
        ERRPRINT( "unexpected log output while testing." );
        abort();
    }

    result = fgets( data, STRING_MAXLEN, fp );
    if (result == NULL) {
        perror( __FILE__ ",  fgets" );
        abort();
    }
    result = strstr( data, "5 this is another multi-part entry," );
    if (result == NULL) {
        ERRPRINT( "unexpected log output while testing." );
        abort();
    }
    result = fgets( data, STRING_MAXLEN, fp );
    if (result == NULL) {
        perror( __FILE__ ",  fgets" );
        abort();
    }
    result = strstr( data, "but it spans two lines and has data: like 0.25" );
    if (result == NULL) {
        ERRPRINT( "unexpected log output while testing." );
        abort();
    }

    if (closelogfile()) {
        ERRPRINT( "failed to close log file for testing." );
        abort();
    }
    if (remove( filename )) {
        perror( __FILE__ ",  remove" );
        abort();
    }
    return 0;
}
