/* 
 * General configuration of the device
 * 
 * Karl Palsson <karlp@tweak.net.au> 2012
 */

#ifndef SYSCFG_H
#define	SYSCFG_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <libopencm3/stm32/f1/gpio.h>
#include <libopencm3/stm32/nvic.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/usart.h>


#define USART_CONSOLE USART1

    // Discovery board LED is PC8 (blue led)
#define PIN_STATUS_LED GPIO8
#define PORT_STATUS_LED GPIOC


    struct state_t {
        unsigned long last_blink_time;
    };


#ifdef	__cplusplus
}
#endif

#endif	/* SYSCFG_H */

