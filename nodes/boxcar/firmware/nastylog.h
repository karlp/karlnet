/* 
 * headers and docs for nastylog
 * 
 * Karl Palsson <karlp@remake.is>, ReMake Electric ehf. 2012
 */

#ifndef NASTYLOG_H
#define	NASTYLOG_H

#ifdef	__cplusplus
extern "C" {
#endif


#define UDEBUG 90
#define UINFO  50
#define UWARN  30
#define UERROR 20
#define UFATAL 10

#define USE_NASTYLOG
#define NASTYLOG_LEVEL UDEBUG
    
    int nastylog(int level, const char *tag, const char *format, ...);
    

#ifdef	__cplusplus
}
#endif

#endif	/* NASTYLOG_H */

