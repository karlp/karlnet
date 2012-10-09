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
    
    int nastylog(const char *tag, const char *format, ...);
    

#ifdef	__cplusplus
}
#endif

#endif	/* NASTYLOG_H */

