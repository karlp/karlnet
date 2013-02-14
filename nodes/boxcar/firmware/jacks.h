/* 
 * File:   jacks.h
 * Author: karlp
 *
 * Created on February 1, 2013, 12:36 AM
 */

#ifndef JACKS_H
#define	JACKS_H

#ifdef	__cplusplus
extern "C" {
#endif


#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/f1/adc.h>
#include "syscfg.h"

	struct jack_t {
		u32 power_port;
		u32 power_pin;
		u32 en_port;
		u32 en_pin;
		u32 val_port;
		u32 val_pin;
		u32 val_channel;
		int power_on_time_millis;
		int sensor_type;
	};

	bool jack_connected(const struct jack_t *jack);
	void jack_setup(const struct jack_t *jack, volatile struct jacks_machine_t *machine);
	void jack_run_task(volatile struct jacks_machine_t *machine, struct jacks_result_t *res);




#ifdef	__cplusplus
}
#endif

#endif	/* JACKS_H */

