
#include <stdio.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/f1/adc.h>
#include "syscfg.h"
#include "ms_systick.h"
#include "jacks.h"

void jack_setup(const struct jack_t *jack, volatile struct jacks_machine_t *machine)
{
	// turn on gpios and modes for the pins...
	gpio_set_mode(jack->en_port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, jack->en_pin);
	// Explictly pull down.  This burns ~66uA per jack! // FIXME - make sure this all works properly!
	GPIO_ODR(jack->en_port) &= ~(jack->en_pin);
	gpio_set_mode(jack->power_port, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, jack->power_pin);
	gpio_set_mode(jack->val_port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, jack->val_pin);
	machine->step = jack_machine_step_off;
	machine->last_read_millis = 0;
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
void jack_run_task(volatile struct jacks_machine_t *machine, struct jacks_result_t *res)
{
	res->ready = false;
	if (!jack_connected(machine->jack)) {
		return;
	}
	switch (machine->step) {
	case jack_machine_step_off:
		// is it time to do a reading yet?
		if (millis() - 3000 > machine->last_read_millis) {
			printf("switching power on: channel %u\n", (unsigned int) machine->jack->val_channel);
			gpio_set(machine->jack->power_port, machine->jack->power_pin);
			machine->step = jack_machine_step_powered;
			machine->step_entry_millis = millis();
		}
		break;

	case jack_machine_step_powered:
		// have we been powered up long enough yet?
		if (millis() - machine->jack->power_on_time_millis > machine->step_entry_millis) {
			printf("power stable!\n");
			machine->step = jack_machine_step_ready;
			// not really necessary... machine->step_entry_millis = millis();
		} else {
			printf(".");
		}
		break;

	case jack_machine_step_ready:
		// TODO - this should actually start a dma sequence and go to a next step 
		// that decimates/averages and finally returns.
		// ok! do a few readings and call it good
		adc_disable_scan_mode(ADC1);
		adc_set_single_conversion_mode(ADC1);
		adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_28DOT5CYC);
		//adc_set_single_channel(ADC1, machine->jack->val_channel);
		adc_set_regular_sequence(ADC1, 1, (u8*)&(machine->jack->val_channel));

		adc_enable_external_trigger_regular(ADC1, ADC_CR2_EXTSEL_SWSTART);
		adc_start_conversion_regular(ADC1);
		printf("ok, doing reading on channel!\n");
		while(!adc_eoc(ADC1)) {
			;
		}
		res->ready = true;
		res->value = adc_read_regular(ADC1);
		machine->last_value = res->value;
		machine->last_read_millis = millis();
		gpio_clear(machine->jack->power_port, machine->jack->power_pin);
		machine->step = jack_machine_step_off;
		break;
	}
	return;
}
