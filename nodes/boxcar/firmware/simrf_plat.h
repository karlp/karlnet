/* 
 * File:   simrf_plat.h
 * Author: karlp
 *
 * Created on August 9, 2012, 10:41 PM
 */

#ifndef SIMRF_PLAT_H
#define	SIMRF_PLAT_H

#ifdef	__cplusplus
extern "C" {
#endif

    /**
     * You can also just set up the function pointers yourself.
     */
void platform_simrf_init(void);
void platform_mrf_interrupt_disable(void);
void platform_mrf_interrupt_enable(void);

#ifdef	__cplusplus
}
#endif

#endif	/* SIMRF_PLAT_H */

