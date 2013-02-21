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

#define PORT_RHT_POWER GPIOA
#define PIN_RHT_POWER GPIO10
#define PORT_RHT_IO GPIOB
#define PIN_RHT_IO GPIO6
#define RHT_EXTI EXTI6
#define RHT_isr exti9_5_isr
#define RHT_NVIC NVIC_EXTI9_5_IRQ


#define MRF_SPI SPI1
#define MRF_SELECT_PORT GPIOB
#define MRF_SELECT_PIN GPIO7
	//#define MRF_RESET_PORT GPIOC
	//#define MRF_RESET_PIN GPIO1
#define MRF_INTERRUPT_PORT GPIOB
#define MRF_INTERRUPT_PIN GPIO0
#define MRF_INTERRUPT_NVIC NVIC_EXTI0_IRQ
#define MRF_INTERRUPT_EXTI EXTI0
#define MRF_isr exti0_isr


#define RHT_INTER_BIT_TIMEOUT_USEC 500
	// Falling edge-falling edge times less than this are 0, else 1
#define RHT_LOW_HIGH_THRESHOLD 100

	enum jack_machine_steps {
		jack_machine_step_off,
		jack_machine_step_powered,
		jack_machine_step_ready
	};

	struct jacks_result_t {
		bool ready;
		int value;
	};

	struct jacks_machine_t {
		int last_read_millis;
		int last_value;
		enum jack_machine_steps step;
		int step_entry_millis;
		const struct jack_t *jack;
	};

	struct state_t {
		int seconds;
		int last_start;
		long last_blink_time;
		bool seen_startbit;
		uint8_t rht_bytes[5];
		bool rht_timeout;
		int bitcount;
		int milliticks;
		uint16_t rf_dest_id;

		float last_temperature;
		float last_relative_humidity;
		long last_send_time;

		struct jacks_machine_t jack_machine1;
		struct jacks_machine_t jack_machine2;
	};


#ifdef	__cplusplus
}
#endif

#endif	/* SYSCFG_H */

