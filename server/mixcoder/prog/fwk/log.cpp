#include <string>
#include <sstream>
#include <syslog.h>
#include <iomanip>
#include <fstream>
#include "log.h"

static int mode;

void Logger::initLog( const char* syslogName,
                          int m)
{
    mode = m;
    if( mode == kSyslog ) {
        openlog( syslogName, LOG_PID, LOG_USER );
    }
}

void Logger::log( const char * fmt, ... ) 
{        
    va_list valist;

    /* initialize valist for num number of arguments */
    va_start(valist, fmt);

    if ( mode == kSyslog ) {
        syslog( LOG_DEBUG, fmt, valist);
    } else {
        fprintf(stderr, fmt, valist);
    }
    va_end(valist);
}

