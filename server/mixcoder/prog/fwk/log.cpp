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
    va_list args;

    /* initialize valist for num number of arguments */
    va_start(args, fmt);

    if ( mode == kSyslog ) {
        vsyslog( LOG_DEBUG, fmt, args );
    } 
    vfprintf(stderr, fmt, args);
    
    va_end(args);
}

