/**
 * Karl Palsson, 2012
 * Demo AVR platform driver for simrf
 * 
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/f1/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/spi.h>

#include <simrf.h>
#include "syscfg.h"
#include "simrf_plat.h"

void platform_mrf_interrupt_disable(void) {
    exti_disable_request(MRF_INTERRUPT_EXTI);
    nvic_disable_irq(MRF_INTERRUPT_NVIC);
}

void platform_mrf_interrupt_enable(void) {
    // Enable EXTI0 interrupt.
    nvic_enable_irq(MRF_INTERRUPT_NVIC);
    /* Configure the EXTI subsystem. */
    exti_select_source(MRF_INTERRUPT_EXTI, MRF_INTERRUPT_PORT);
    exti_set_trigger(MRF_INTERRUPT_EXTI, EXTI_TRIGGER_FALLING);
    exti_enable_request(MRF_INTERRUPT_EXTI);
}

void MRF_isr(void) {
    exti_reset_request(MRF_INTERRUPT_EXTI);
    simrf_interrupt_handler();
}

static void plat_select(bool value) {
    // active low
    if (value) {
        gpio_clear(MRF_SELECT_PORT, MRF_SELECT_PIN);
    } else {
        gpio_set(MRF_SELECT_PORT, MRF_SELECT_PIN);
    }
}

extern void delay_ms(int ms);

static uint8_t plat_spi_tx(uint8_t cData) {
    return spi_xfer(MRF_SPI, cData);
}

void spi_setup(void) {
    // SPI1 SCK
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_SPI1_SCK);
    // SPI1 MOSI
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_SPI1_MOSI);
    // SPI ChipSelect
    gpio_set_mode(MRF_SELECT_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, MRF_SELECT_PIN);
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO_SPI1_MISO);

    /* Setup SPI parameters. */
    spi_init_master(MRF_SPI, SPI_CR1_BAUDRATE_FPCLK_DIV_4, SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
        SPI_CR1_CPHA_CLK_TRANSITION_1, SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);
    /* Ignore the stupid NSS pin. */
    spi_enable_software_slave_management(MRF_SPI);
    spi_set_nss_high(MRF_SPI);

    /* Finally enable the SPI. */
    spi_enable(MRF_SPI);
}

void mrf_gpio_setup(void) {
    // MRF reset pin
    //gpio_set_mode(MRF_RESET_PORT, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, MRF_RESET_PIN);
    // MRF interrupt pin
    gpio_set_mode(MRF_INTERRUPT_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, MRF_INTERRUPT_PIN);
}


void platform_simrf_init(void) {
    mrf_gpio_setup();
    spi_setup();
    
    plat_select(false);
    
    struct simrf_platform plat;
    memset(&plat, 0, sizeof(plat));
    plat.select = &plat_select;
    plat.reset = NULL; // Not connected on this board
    plat.spi_xfr = &plat_spi_tx;
    plat.delay_ms = &delay_ms;
    // TODO more here!
    simrf_setup(&plat);
}
