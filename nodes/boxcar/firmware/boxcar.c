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
    }

    return 0;
}
