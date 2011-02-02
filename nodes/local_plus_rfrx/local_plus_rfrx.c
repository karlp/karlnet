/*
             LUFA Library
     Copyright (C) Dean Camera, 2010.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org


    Based on DualVirtualSerial demo project,
Guts ripped out and replaced with my own,
Karl Palsson, 2010

*/

/*
  Copyright 2010  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaim all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

#include "local_plus_rfrx.h"
#include "karlnet.h"
#include "FreqCounter2.h"


/** LUFA CDC Class driver interface configuration and state information. This structure is
 *  passed to all CDC Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another. This is for the first CDC interface,
 *  which sends strings to the host for each joystick movement.
 */
USB_ClassInfo_CDC_Device_t VirtualSerial1_CDC_Interface =
	{
		.Config =
			{
				.ControlInterfaceNumber           = 0,

				.DataINEndpointNumber             = CDC1_TX_EPNUM,
				.DataINEndpointSize               = CDC_TXRX_EPSIZE,
				.DataINEndpointDoubleBank         = false,

				.DataOUTEndpointNumber            = CDC1_RX_EPNUM,
				.DataOUTEndpointSize              = CDC_TXRX_EPSIZE,
				.DataOUTEndpointDoubleBank        = false,

				.NotificationEndpointNumber       = CDC1_NOTIFICATION_EPNUM,
				.NotificationEndpointSize         = CDC_NOTIFICATION_EPSIZE,
				.NotificationEndpointDoubleBank   = false,
			},
	};

/** LUFA CDC Class driver interface configuration and state information. This structure is
 *  passed to all CDC Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another. This is for the second CDC interface,
 *  which echos back all received data from the host.
 */
USB_ClassInfo_CDC_Device_t VirtualSerial2_CDC_Interface =
	{
		.Config =
			{
				.ControlInterfaceNumber           = 2,

				.DataINEndpointNumber             = CDC2_TX_EPNUM,
				.DataINEndpointSize               = CDC_TXRX_EPSIZE,
				.DataINEndpointDoubleBank         = false,

				.DataOUTEndpointNumber            = CDC2_RX_EPNUM,
				.DataOUTEndpointSize              = CDC_TXRX_EPSIZE,
				.DataOUTEndpointDoubleBank        = false,

				.NotificationEndpointNumber       = CDC2_NOTIFICATION_EPNUM,
				.NotificationEndpointSize         = CDC_NOTIFICATION_EPSIZE,
				.NotificationEndpointDoubleBank   = false,
			},
	};

/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
int main(void)
{
	SetupHardware();

	sei();

        kpacket packet;
        packet.header = 'x';
        packet.version = 1;
        packet.nsensors = 3;

        char mychar;
        char waitingForFreq = 0;
        unsigned long frq = 0;

	for (;;)
	{
		/* Discard all received data on both CDC interfaces */
		CDC_Device_ReceiveByte(&VirtualSerial1_CDC_Interface);
		CDC_Device_ReceiveByte(&VirtualSerial2_CDC_Interface);

                // Serial1 listens for rf data from the serially connected xbee
                if (Serial_IsCharReceived()) {
                    do {
                        mychar = Serial_RxByte();
                        CDC_Device_SendByte(&VirtualSerial1_CDC_Interface, mychar);
                    } while (Serial_IsCharReceived());
                } 

                // if appropriate timing wise, also create and send our own local packet
                uint16_t adc = ADC_GetChannelReading(ADC_REFERENCE_AVCC | ADC_CHANNEL0);
                adc = ADC_GetChannelReading(ADC_REFERENCE_AVCC | ADC_CHANNEL0);
                adc = ADC_GetChannelReading(ADC_REFERENCE_AVCC | ADC_CHANNEL0);
                
                uint16_t itemp = ADC_GetChannelReading(ADC_REFERENCE_INT2560MV | ADC_INT_TEMP_SENS);

                // primitive state machine
                if (!waitingForFreq) {
                    FreqCounter__start(1, 1000);            // Start counting with gatetime of 100ms
                    waitingForFreq = 1;
                    frq = 0;
                }
                if (waitingForFreq && f_ready) {
                    frq = f_freq;
                    waitingForFreq = 0;
                }

                if (frq > 0) {
                    // Remember, this sort of raw string will need a special parser,
                    // it won't have any escaping, rf frame type, or node id
                    ksensor s1 = {36, adc};
                    ksensor s2 = {'I', itemp};
                    ksensor s3 = {'f', frq};
                    packet.ksensors[0] = s1;
                    packet.ksensors[1] = s2;
                    packet.ksensors[2] = s3;

                    CDC_Device_SendString(&VirtualSerial2_CDC_Interface, (char *)&packet, sizeof(kpacket));
                    frq = 0;
                }

		CDC_Device_USBTask(&VirtualSerial1_CDC_Interface);
		CDC_Device_USBTask(&VirtualSerial2_CDC_Interface);
		USB_USBTask();
	}
}

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware(void)
{
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);

	/* Hardware Initialization */
        Serial_Init(19200, false);
        ADC_Init(ADC_PRESCALE_128);
	USB_Init();
}


/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	CDC_Device_ConfigureEndpoints(&VirtualSerial1_CDC_Interface);
	CDC_Device_ConfigureEndpoints(&VirtualSerial2_CDC_Interface);
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
	CDC_Device_ProcessControlRequest(&VirtualSerial1_CDC_Interface);
	CDC_Device_ProcessControlRequest(&VirtualSerial2_CDC_Interface);
}

