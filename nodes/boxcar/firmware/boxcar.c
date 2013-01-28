/*
 * Karl Palsson, 2012 <karlp@tweak.net.au
 */

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/f1/rcc.h>
#include <libopencm3/stm32/f1/flash.h>
#include <libopencm3/stm32/f1/dma.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/timer.h>

#include "syscfg.h"
#include "ms_systick.h"

static struct state_t state;


void clock_setup(void) {
    rcc_clock_setup_in_hsi_out_24mhz();
    /* Lots of things on all ports... */
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPAEN);
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPBEN);
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPCEN);

    rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_USART2EN);
    // oh, and dma!
    rcc_peripheral_enable_clock(&RCC_AHBENR, RCC_AHBENR_DMA1EN);
    // and timers...
    rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_TIM6EN);
}

void usart_enable_all_pins(void) {
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART2_TX);
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO_USART2_RX);
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
    gpio_set_mode(PORT_DHT_POWER, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, PIN_DHT_POWER);

    // FIXME - we aren't using this yet...
    //gpio_set_mode(PORT_STATUS_LED, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, PIN_STATUS_LED);
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

void dht_power(bool enable) {
    if (enable) {
        gpio_set(PORT_DHT_POWER, PIN_DHT_POWER);
    } else {
        gpio_clear(PORT_DHT_POWER, PIN_DHT_POWER);
    }
}

/**
 * Ugly hack, just drag some pins up and down and try and record timings...
 * @return 
 */
int read_dht(void) {
    // drag the pins up and down, 
    // then turn on EXTI, and have it just print out that it detected transitions.
    // Then, we can turn on a timer to have the interrupt grab the times instead.
    
    
    // This is for the IO pin...
    gpio_set_mode(PORT_DHT_IO, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, PIN_DHT_IO);
    gpio_set(PORT_DHT_IO, PIN_DHT_IO);
    delay_ms(250);

    
    gpio_clear(PORT_DHT_IO, PIN_DHT_IO);
    delay_ms(20);
    gpio_set(PORT_DHT_IO, PIN_DHT_IO);
    // want to wait for 40us here, but we're ok with letting some code delay us..

    gpio_set_mode(PORT_DHT_IO, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, PIN_DHT_IO);
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
        if (timer_get_counter(TIM6) > 0xdfff) {
            printf("timeout...\n");
            break;
        }
        int nowstate = gpio_get(PORT_DHT_IO, PIN_DHT_IO);
        if (nowstate != state) {
            timings[bitcount++] = timer_get_counter(TIM6);
            state = nowstate;
        }
    }
    int i = 0;
    for (i =  0; i < bitcount; i++) {
        printf("tim[%d] = %d\n", i, timings[i]);
    }
    
#endif
    
    return 0;
    
}

#if 0
void exti0_isr(void) {
    exti_reset_request(EXTI_DHT);
    state.bitcount++;
    if (gpio_get(PORT_DHT, PIN_DHT_IO)) {
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
    dht_power(true);

    while (1) {
        if (millis() - state.last_blink_time > 1000) {
            printf("still alive: %c\n", c + '0');
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
