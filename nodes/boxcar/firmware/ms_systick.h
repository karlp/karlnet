/* 
 * Karl Palsson <karlp@tweak.net.au> 2012
 */

#ifndef MS_SYSTICK_H
#define	MS_SYSTICK_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdio.h>
    
uint64_t millis(void);
void delay_ms(unsigned int ms);



#ifdef	__cplusplus
}
#endif

#endif	/* MS_SYSTICK_H */

