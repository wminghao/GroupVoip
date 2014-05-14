#ifndef __LOG_H__
#define __LOG_H__

#include <fstream>
#include <assert.h>
#include <stdarg.h>


#define LOG Logger::log

typedef enum {
    kStderr,
    kSyslog
} LOG_MODE;

class Logger
{
 public:
    static void initLog( const char* programName, int m);
    static void log( const char * fmt, ... );

 private:
    Logger();
};

#endif //__LOG_H__
