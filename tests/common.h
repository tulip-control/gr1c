/* SCL; April 2012. */

#ifndef _TESTS_COMMON_H_
#define _TESTS_COMMON_H_

#include <stdio.h>


#define ERRPRINT(msg) fprintf( stderr, "============================================================\nERROR: " msg "\n\n" )
#define ERRPRINT1(msg, arg) fprintf( stderr, "============================================================\nERROR: " msg "\n\n", arg )
#define ERRPRINT2(msg, arg1, arg2) fprintf( stderr, "============================================================\nERROR: " msg "\n\n", arg1, arg2 )


#endif
