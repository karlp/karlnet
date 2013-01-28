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

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/usart.h>


#define USART_CONSOLE USART2
#define USE_NASTYLOG 1

    // Discovery board LED is PC8 (blue led)
#define PIN_STATUS_LED GPIO14
#define PORT_STATUS_LED GPIOB

#define PORT_DHT_POWER GPIOA
#define PIN_DHT_POWER GPIO10
#define PORT_DHT_IO GPIOB
#define PIN_DHT_IO GPIO6
#define EXTI_DHT EXTI6
    


    struct state_t {
        unsigned long last_blink_time;
        unsigned long last_dht_time;
        int bitcount;
    };


#ifdef	__cplusplus
}
#endif

#endif	/* SYSCFG_H */

