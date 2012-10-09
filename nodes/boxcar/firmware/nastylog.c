/* 
 * Like uglylogging, but no syslog, no timestamps, and only on or off.
 * (So it can be optimized out hopefully)
 * 
 * Karl Palsson <karlp@tweak.net.au> 2012
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "syscfg.h"
#include "nastylog.h"

// Using fiprintf instead of fprintf will make this somewhat smaller

#if (USE_NASTYLOG == 1)
static int nastylog_stderr(const char *tag, const char *format, va_list args) {
    fiprintf(stderr, "%s: ", tag);
    vfiprintf(stderr, format, args);
    return 0;
}
#endif


int nastylog(const char *tag, const char *format, ...) {
#if (USE_NASTYLOG == 1)
    va_list args;
    va_start(args, format);
    nastylog_stderr(tag, format, args);
    va_end(args);
#else
    (void)tag;
    (void)format;
#endif
    return 0;
}
