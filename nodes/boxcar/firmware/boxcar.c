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
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/timer.h>

#include <simrf.h>
#include "simrf_plat.h"

#include "syscfg.h"
#include "ms_systick.h"
#include "jacks.h"

const struct jack_t jack1 = {
	.en_pin = GPIO15,
	.en_port = GPIOC,
	.power_pin = GPIO1,
	.power_port = GPIOA,
	.val_pin = GPIO0,
	.val_port = GPIOA,
	.val_channel = ADC_CHANNEL0,
	.power_on_time = 5 // just a guess
};

const struct jack_t jack2 = {
	.en_pin = GPIO8,
	.en_port = GPIOA,
	.power_pin = GPIO9,
	.power_port = GPIOA,
	.val_pin = GPIO1,
	.val_port = GPIOB,
	.val_channel = ADC_CHANNEL9,
	.power_on_time = 5 // just a guess
};

__attribute__((always_inline)) static inline void __WFI(void)
{
	__asm volatile ("wfi");
}

struct state_t volatile state;

void clock_setup(void)
{
	rcc_clock_setup_in_hsi_out_24mhz();
	/* Lots of things on all ports... */
	rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPAEN);
	rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPBEN);
	rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPCEN);

	rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_USART2EN);
	// oh, and dma!
	rcc_peripheral_enable_clock(&RCC_AHBENR, RCC_AHBENR_DMA1EN);
	// and timers...
	rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_TIM7EN);
	// and spi for the radio
	rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_SPI1EN);
	/* Enable AFIO clock. */
	rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_AFIOEN);
}

void usart_enable_all_pins(void)
{

	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART2_TX);
	gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO_USART2_RX);
}

void usart_console_setup(void)
{

	usart_set_baudrate(USART_CONSOLE, 115200);
	usart_set_databits(USART_CONSOLE, 8);
	usart_set_stopbits(USART_CONSOLE, USART_STOPBITS_1);
	usart_set_mode(USART_CONSOLE, USART_MODE_TX);
	usart_set_parity(USART_CONSOLE, USART_PARITY_NONE);
	usart_set_flow_control(USART_CONSOLE, USART_FLOWCONTROL_NONE);
	usart_enable(USART_CONSOLE);
}

void gpio_setup(void)
{
	gpio_set_mode(PORT_RHT_POWER, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, PIN_RHT_POWER);

	// FIXME - we aren't using this yet...
	gpio_set_mode(PORT_STATUS_LED, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, PIN_STATUS_LED);
}

void systick_setup(void)
{

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
int _write(int file, char *ptr, int len)
{
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

void dht_power(bool enable)
{
	if (enable) {
		gpio_set(PORT_RHT_POWER, PIN_RHT_POWER);
	} else {
		gpio_clear(PORT_RHT_POWER, PIN_RHT_POWER);
	}
}

void stuff_bit(int bitnumber, int timing, volatile uint8_t * bytes)
{
	int byte_offset = bitnumber / 8;
	int bit = 7 - (bitnumber % 8); // Stuff MSB first.
	if (timing < RHT_LOW_HIGH_THRESHOLD) {
		bytes[byte_offset] &= ~(1 << bit);
	} else {

		bytes[byte_offset] |= (1 << bit);
	}
}

void RHT_isr(void)
{
	exti_reset_request(RHT_EXTI);
	int cnt = TIM7_CNT;
	TIM7_CNT = 0;
	// Skip catching ourself pulsing the start line until the 150uS start.
	if (!state.seen_startbit) {
		if (cnt < RHT_LOW_HIGH_THRESHOLD) {
			return;
		} else {
			state.seen_startbit = true;
		}
	}
	if (state.bitcount > 0) { // but skip that start bit...
		stuff_bit(state.bitcount - 1, cnt, state.rht_bytes);
	}
	state.bitcount++;
}

/**
 * We set this timer to count uSecs.
 * The interrupt is only to indicate that it timed out and to shut itself off.
 */
void setup_tim7(void)
{

	timer_clear_flag(TIM7, TIM_SR_UIF);
	TIM7_CNT = 1;
	timer_set_prescaler(TIM7, 23); // 24Mhz/1Mhz - 1
	timer_set_period(TIM7, RHT_INTER_BIT_TIMEOUT_USEC);
	timer_enable_irq(TIM7, TIM_DIER_UIE);
	nvic_enable_irq(NVIC_TIM7_IRQ);
	timer_enable_counter(TIM7);
}

void start_rht_read(void)
{
	// First, move the pins up and down to get it going...
	gpio_set_mode(PORT_RHT_IO, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, PIN_RHT_IO);
	gpio_clear(PORT_RHT_IO, PIN_RHT_IO);
	delay_ms(20); // docs say 1-10ms is enough....
	gpio_set(PORT_RHT_IO, PIN_RHT_IO);
	// want to wait for 40us here, but we're ok with letting some code delay us..
	state.bitcount = 0;
	state.seen_startbit = false;
	state.rht_timeout = false;
	nvic_enable_irq(RHT_NVIC);
	// pull up will finish the job here for us.
	gpio_set_mode(PORT_RHT_IO, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, PIN_RHT_IO);
	exti_select_source(RHT_EXTI, PORT_RHT_IO);
	exti_set_trigger(RHT_EXTI, EXTI_TRIGGER_FALLING);
	exti_enable_request(RHT_EXTI);
	setup_tim7();
}

void tim7_isr(void)
{
	timer_clear_flag(TIM7, TIM_SR_UIF);
	state.rht_timeout = true;
	nvic_disable_irq(NVIC_TIM7_IRQ);
	timer_disable_irq(TIM7, TIM_DIER_UIE);
	timer_disable_counter(TIM7);
}

void wait_for_shit(void)
{
	while (1) {
		if (state.rht_timeout) {
			return;
		}
		if (state.bitcount >= 40) {
			return;
		}
		//__WFI();
	}
}

void handle_tx(simrf_tx_info_t *txinfo)
{
	if (txinfo->tx_ok) {
		printf("TX went ok, got ack\n");
	} else {
		printf("TX failed after %d retries\n", txinfo->retries);
	}
}

void loop_forever(void)
{
	if (state.seconds - state.last_start > 3) {
		state.last_start = state.seconds;
		printf("Start!\n");
		start_rht_read();
		wait_for_shit();
		if (state.rht_timeout) {
			printf("timeout, toggling power\n");
			dht_power(false);
			delay_ms(1000);
			dht_power(true);
			delay_ms(3000);
			return;
		}
		printf("All bits found!\n");
		unsigned chksum = state.rht_bytes[0] + state.rht_bytes[1] + state.rht_bytes[2] + state.rht_bytes[3];
		chksum &= 0xff;
		printf("%x %x %x %x sum: %x == %x\n",
			state.rht_bytes[0], state.rht_bytes[1], state.rht_bytes[2], state.rht_bytes[3],
			chksum, state.rht_bytes[4]);
		if (chksum != state.rht_bytes[4]) {
			printf("CHKSUM failed, ignoring: \n");
			return;
		}

		int rh = (state.rht_bytes[0] << 8 | state.rht_bytes[1]);
		int temp = (state.rht_bytes[2] << 8 | state.rht_bytes[3]);
		printf("orig: temp = %d, rh = %d\n", temp, rh);
		printf("Temp: %d.%d C, RH = %d.%d %%\n", temp / 10, temp % 10, rh / 10, rh % 10);
		// Send something real here!
		simrf_send16(0x1, 4, "abcd");

	}
	// texane/stlink will have problems debugging through this.
	//__WFI();
}

int main(void)
{
	clock_setup();
	gpio_setup();
	systick_setup();
	usart_enable_all_pins();
	usart_console_setup();
	printf("hello!\n");
	// power up the RHT chip...
	dht_power(true);
	delay_ms(2000);
	setup_tim7();
	platform_simrf_init();
	// interrupt pin from mrf
	platform_mrf_interrupt_enable();

	simrf_soft_reset();
	simrf_init();

	simrf_pan_write(0xcafe);
	uint16_t pan_sanity_check = simrf_pan_read();
	printf("pan read back in as %#x\n", pan_sanity_check);
	simrf_address16_write(0x1111);

	jack_setup(&jack1, &state.jack_machine1);
	jack_setup(&jack2, &state.jack_machine2);


	while (1) {
		struct jacks_result_t jr1, jr2;
		simrf_check_flags(NULL, &handle_tx);
		loop_forever();
		jack_run_task(&state.jack_machine1, &jr1);
		jack_run_task(&state.jack_machine2, &jr2);
	}

	return 0;
}
