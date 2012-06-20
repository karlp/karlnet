/* 
 * Like uglylogging, but no syslog, no timestamps, and only on or off.
 * (So it can be optimized out hopefully)
 * 
 * Karl Palsson <karlp@tweak.net.au> 2012
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "nastylog.h"

// Using fiprintf instead of fprintf will make this somewhat smaller

static int nastylog_stderr(int level, const char *tag, const char *format, va_list args) {
    switch (level) {
#if (NASTYLOG_LEVEL >= UDEBUG)
    case UDEBUG:
        fprintf(stderr, "DEBUG %s: ", tag);
        vfprintf(stderr, format, args);
        break;
#endif
#if (NASTYLOG_LEVEL >= UINFO)
    case UINFO:
        fprintf(stderr, "INFO %s: ", tag);
        vfprintf(stderr, format, args);
        break;
#endif
#if (NASTYLOG_LEVEL >= UWARN)
    case UWARN:
        fprintf(stderr, "WARN %s: ", tag);
        vfprintf(stderr, format, args);
        break;
#endif
#if (NASTYLOG_LEVEL >= UERROR)
    case UERROR:
        fprintf(stderr, "ERROR %s: ", tag);
        vfprintf(stderr, format, args);
        break;
#endif
#if (NASTYLOG_LEVEL >= UFATAL)
    case UFATAL:
        fprintf(stderr, "FATAL %s: ", tag);
        vfprintf(stderr, format, args);
        exit(EXIT_FAILURE);
        // NEVER GETS HERE!!!
        break;
#endif
    default:
        break;
    }
    return 1;
}


int nastylog(int level, const char *tag, const char *format, ...) {
#if defined USE_NASTYLOG
    va_list args;
    va_start(args, format);
    nastylog_stderr(level, tag, format, args);
    va_end(args);
#endif
    return 0;
}
