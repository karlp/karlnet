/*
 * Karl Palsson, 2012 <karlp@tweak.net.au
 */

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <libopencm3/stm32/f1/rcc.h>
#include <libopencm3/stm32/f1/flash.h>
#include <libopencm3/stm32/f1/gpio.h>
#include <libopencm3/stm32/f1/dma.h>
#include <libopencm3/stm32/f1/desig.h>
#include <libopencm3/stm32/crc.h>
#include <libopencm3/stm32/nvic.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/systick.h>
#include <libopencm3/stm32/timer.h>

#include "syscfg.h"
#include "nastylog.h"
#include "ms_systick.h"

#define LOG_TAG __FILE__
#define DLOG(format, args...)         nastylog(UDEBUG, LOG_TAG, format, ## args)
#define ILOG(format, args...)         nastylog(UINFO, LOG_TAG, format, ## args)
#define WLOG(format, args...)         nastylog(UWARN, LOG_TAG, format, ## args)

static struct state_t state;

/**
 * To go back to libopencm3 for later...
 * This makes it "like" the 32vl discovery board, but without HSE fitted
 */
#if libopencm3_does_not_have_karls_patches_yet
void rcc_clock_setup_in_hsi_out_24mhz(void) {
    /* Enable internal high-speed oscillator. */
    rcc_osc_on(HSI);
    rcc_wait_for_osc_ready(HSI);

    /* Select HSI as SYSCLK source. */
    rcc_set_sysclk_source(RCC_CFGR_SW_SYSCLKSEL_HSICLK);

    /*
     * Set prescalers for AHB, ADC, ABP1, ABP2.
     * Do this before touching the PLL (TODO: why?).
     */
    rcc_set_hpre(RCC_CFGR_HPRE_SYSCLK_NODIV); /* Set. 24MHz Max. 24MHz */
    rcc_set_adcpre(RCC_CFGR_ADCPRE_PCLK2_DIV2); /* Set.  12MHz Max. 12MHz */
    rcc_set_ppre1(RCC_CFGR_PPRE1_HCLK_NODIV); /* Set. 24MHz Max. 24MHz */
    rcc_set_ppre2(RCC_CFGR_PPRE2_HCLK_NODIV); /* Set. 24MHz Max. 24MHz */

    /*
     * Sysclk is (will be) running with 24MHz -> 2 waitstates.
     * 0WS from 0-24MHz
     * 1WS from 24-48MHz
     * 2WS from 48-72MHz
     */
    flash_set_ws(FLASH_LATENCY_0WS);

    /*
     * Set the PLL multiplication factor to 6.
     * 8MHz (internal) * 6 (multiplier) / 2 (PLLSRC_HSI_CLK_DIV2) = 24MHz
     */
    rcc_set_pll_multiplication_factor(RCC_CFGR_PLLMUL_PLL_CLK_MUL6);

    /* Select HSI/2 as PLL source. */
    rcc_set_pll_source(RCC_CFGR_PLLSRC_HSI_CLK_DIV2);

    /* Enable PLL oscillator and wait for it to stabilize. */
    rcc_osc_on(PLL);
    rcc_wait_for_osc_ready(PLL);

    /* Select PLL as SYSCLK source. */
    rcc_set_sysclk_source(RCC_CFGR_SW_SYSCLKSEL_PLLCLK);

    /* Set the peripheral clock frequencies used */
    rcc_ppre1_frequency = 24000000;
    rcc_ppre2_frequency = 24000000;
}
#endif

void clock_setup(void) {
    //rcc_clock_setup_in_hse_8mhz_out_24mhz();
    rcc_clock_setup_in_hsi_out_24mhz();

    // oh, but because we want 300 baud for usart3 (kamstrup), 
    // 24mhz on apb1 is too fast,
    rcc_set_ppre1(RCC_CFGR_PPRE1_HCLK_DIV2);
    rcc_ppre1_frequency = 12000000; // Used internally by libopencm3

    /* uart1 and 2 pins are port A, also status LED */
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPAEN);
    /* usart 3 pins are on port B*/
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPBEN);
    //  Leds on port c
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPCEN);
    // all uarts please :)
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_USART1EN);
    rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_USART2EN);
    rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_USART3EN);
    // oh, and dma!
    rcc_peripheral_enable_clock(&RCC_AHBENR, RCC_AHBENR_DMA1EN);
    rcc_peripheral_enable_clock(&RCC_AHBENR, RCC_AHBENR_CRCEN);
    
    // and timers...
    rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_TIM6EN);
}

/**
 * We want all usarts, so just blindly configure all the _pins_ here.
 * We can set up bauds and parity elsewhere with logical names
 */
void usart_enable_all_pins(void) {
    /* Setup GPIO pin GPIO_USART1_TX/GPIO9 on GPIO port A for transmit. */
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
            GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART1_TX);
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO_USART1_RX);

    /* Setup GPIO pin GPIO_USART_MODBUS_TX/GPIO2 on GPIO port A for transmit. */
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
            GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART2_TX);
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO_USART2_RX);

    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
            GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART3_TX);
    gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO_USART3_RX);
}

void usart_console_setup(void) {
    usart_set_baudrate(USART_CONSOLE, 115200);
    usart_set_databits(USART_CONSOLE, 8);
    usart_set_stopbits(USART_CONSOLE, USART_STOPBITS_1);
    usart_set_mode(USART_CONSOLE, USART_MODE_TX);
    usart_set_parity(USART_CONSOLE, USART_PARITY_NONE);
    usart_set_flow_control(USART_CONSOLE, USART_FLOWCONTROL_NONE);
    usart_enable(USART_CONSOLE);
}


void gpio_setup(void) {
    gpio_set_mode(PORT_STATUS_LED, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, PIN_STATUS_LED);
}


void systick_setup(void) {
    /* 24MHz / 8 => 3000000 counts per second. */
    systick_set_clocksource(STK_CTRL_CLKSOURCE_AHB_DIV8);

    /* 3000000/3000 = 1000 overflows per second - every 1ms one interrupt */
    systick_set_reload(3000);

    systick_interrupt_enable();

    /* Start counting. */
    systick_counter_enable();
}

/**
 * Use USART_CONSOLE as a console.
 * @param file
 * @param ptr
 * @param len
 * @return 
 */
int _write(int file, char *ptr, int len) {
    int i;

    if (file == STDOUT_FILENO || file == STDERR_FILENO) {
        for (i = 0; i < len; i++) {
            if (ptr[i] == '\n') {
                usart_send_blocking(USART_CONSOLE, '\r');
            }
            usart_send_blocking(USART_CONSOLE, ptr[i]);
        }
        return i;
    }
    errno = EIO;
    return -1;
}

/**
 * Ugly hack, just drag some pins up and down and try and record timings...
 * @return 
 */
int read_dht(void) {
    // drag the pins up and down, 
    // then turn on EXTI, and have it just print out that it detected transitions.
    // Then, we can turn on a timer to have the interrupt grab the times instead.
    
    gpio_set_mode(PORT_DHT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, PIN_DHT);
    gpio_set(PORT_DHT, PIN_DHT);
    delay_ms(250);
    gpio_clear(PORT_DHT, PIN_DHT);
    delay_ms(20);

    gpio_set_mode(PORT_DHT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, PIN_DHT);
#if USE_INTERRUPTS
     //Enable EXTI0 interrupt. 
    nvic_enable_irq(NVIC_EXTI0_IRQ);
    
    exti_select_source(EXTI_DHT, PORT_DHT);
    exti_set_trigger(EXTI_DHT, EXTI_TRIGGER_BOTH);
    exti_enable_request(EXTI_DHT);
#else
    timer_reset(TIM6);
    timer_set_prescaler(TIM6, 23);  // 24Mhz, freq is x + 1, so 1uS ticks...
    timer_set_period(TIM6, 0xffff);  // max, we should use the overflow here to "timeout"
    timer_enable_counter(TIM6);
    
    int state = 0;
    int bitcount = 0;
    int timings[40];
    while (bitcount < 40) {
        if (timer_get_counter(TIM6) > 0xbfff) {
            DLOG("timeout...\n");
            break;
        }
        int nowstate = gpio_get(PORT_DHT, PIN_DHT);
        if (nowstate != state) {
            timings[bitcount++] = timer_get_counter(TIM6);
            //DLOG("pin changed at %d: s!=ns %x != %x\n", timer_get_counter(TIM6), state, nowstate);
            //bitcount++;
            state = nowstate;
        }
    }
    int i = 0;
    for (i =  0; i < bitcount; i++) {
        DLOG("tim[%d] = %d\n", i, timings[i]);
    }
    
#endif
    
    return 0;
    
}

#if 0
void exti0_isr(void) {
    exti_reset_request(EXTI_DHT);
    state.bitcount++;
    if (gpio_get(PORT_DHT, PIN_DHT)) {
        putchar('I');
    } else {
        putchar('i');
    }
    if (state.bitcount > 40) {
        exti_disable_request(EXTI_DHT);
        nvic_disable_irq(NVIC_EXTI0_IRQ);
    }
}
#endif

int main(void) {
    int c = 0;

    memset(&state, 0, sizeof (state));

    clock_setup();
    gpio_setup();
    systick_setup();
    usart_enable_all_pins();
    usart_console_setup();

    while (1) {
        if (millis() - state.last_blink_time > 1000) {
            gpio_toggle(PORT_STATUS_LED, PIN_STATUS_LED); /* LED on/off */
            DLOG("still alive: %c\n", c + '0');
            c = (c == 9) ? 0 : c + 1; /* Increment c. */
            state.last_blink_time = millis();
        }
        if (millis() - state.last_dht_time > 2500) {
            state.last_dht_time = millis();
            read_dht();
        }
    }

    return 0;
}
