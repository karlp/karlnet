/* 
 * Sets up a millisecond systick timer, 
 * and a millis() style timer to make things more compatible with arduino 
 * style code
 * 
 * Karl Palsson <karlp@tweak.net.au> 2012
 */


#include <stdint.h>
#include "ms_systick.h"
#include "syscfg.h"

extern struct state_t state;

static volatile int64_t ksystick;

int64_t millis(void)
{
	return ksystick;
}

/**
 * Busy loop for X ms USES INTERRUPTS
 * @param ms
 */
void delay_ms(int ms)
{
	int64_t now = millis();
	while (millis() - ms < now) {
		;
	}
}

void sys_tick_handler(void)
{
	ksystick++;
	state.milliticks++;
	if (state.milliticks >= 1000) {
		state.milliticks = 0;
		printf("Tick: %d\n", state.seconds);
		state.seconds++;
		gpio_toggle(PORT_STATUS_LED, PIN_STATUS_LED);
	}
}