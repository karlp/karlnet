
#include <stdio.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/f1/adc.h>
#include "syscfg.h"
#include "ms_systick.h"
#include "jacks.h"

void jack_setup(const struct jack_t *jack, struct jacks_machine_t *machine)
{
	// turn on gpios and modes for the pins...
	gpio_set_mode(jack->en_port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, jack->en_pin);
	// Explictly pull down.  // FIXME - make sure this all works properly!
	gpio_set(jack->en_port, jack->en_pin);
	gpio_set_mode(jack->power_port, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, jack->power_pin);
	gpio_set_mode(jack->val_port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, jack->val_pin);
	machine->step = jack_machine_step_off;
	machine->last_read = 0;
	machine->step_entry_millis = 0;
	machine->jack = jack;
}

bool jack_connected(const struct jack_t *jack)
{
	return (gpio_get(jack->en_port, jack->en_pin));
}

/**
 * NOTE this is a state machine, but it expects to run often enough for millis()
 * @param machine
 * @param res
 */
void jack_run_task(struct jacks_machine_t *machine, struct jacks_result_t *res)
{
	res->ready = false;
	if (!jack_connected(machine->jack)) {
		return;
	}
	switch (machine->step) {
	case jack_machine_step_off:
		// is it time to do a reading yet?
		if (millis() - 3000 > machine->last_read) {
			printf("switching power on\n");
			gpio_set(machine->jack->power_port, machine->jack->power_pin);
			machine->step = jack_machine_step_powered;
			machine->step_entry_millis = millis();
		}
		break;

	case jack_machine_step_powered:
		// have we been powered up long enough yet?
		if (millis() - machine->jack->power_on_time > machine->step_entry_millis) {
			printf("power stable!\n");
			machine->step = jack_machine_step_ready;
			// not really necessary... machine->step_entry_millis = millis();
		} else {
			printf(".");
		}
		break;

	case jack_machine_step_ready:
		// ok! do a few readings and call it good
		printf("ok, doing reading on channel!\n");
		res->ready = true;
		res->value = 1234;
		machine->last_read = millis();
		gpio_clear(machine->jack->power_port, machine->jack->power_pin);
		machine->step = jack_machine_step_off;
		break;
	}
	return;
}
