/*------------------------------------------------------------------
 *  gpio.c -- gpio configuration (leds, GPIOTE ppi links)
 *
 *  I. Protonotarios
 *  Embedded Software Lab
 *
 *  July 2016
 *------------------------------------------------------------------
 */

#include "in4073.h"

#define MOTOR_0_PIN	21
#define MOTOR_1_PIN	23
#define MOTOR_2_PIN	25
#define MOTOR_3_PIN	29

void gpio_init(void)
{
	//motors
	NRF_GPIOTE->CONFIG[0] = GPIOTE_CONFIG_MODE_Msk | (MOTOR_0_PIN<<GPIOTE_CONFIG_PSEL_Pos) | GPIOTE_CONFIG_POLARITY_Msk | GPIOTE_CONFIG_OUTINIT_Msk;
	NRF_GPIOTE->CONFIG[1] = GPIOTE_CONFIG_MODE_Msk | (MOTOR_1_PIN<<GPIOTE_CONFIG_PSEL_Pos) | GPIOTE_CONFIG_POLARITY_Msk | GPIOTE_CONFIG_OUTINIT_Msk;
	NRF_GPIOTE->CONFIG[2] = GPIOTE_CONFIG_MODE_Msk | (MOTOR_2_PIN<<GPIOTE_CONFIG_PSEL_Pos) | GPIOTE_CONFIG_POLARITY_Msk | GPIOTE_CONFIG_OUTINIT_Msk;
	NRF_GPIOTE->CONFIG[3] = GPIOTE_CONFIG_MODE_Msk | (MOTOR_3_PIN<<GPIOTE_CONFIG_PSEL_Pos) | GPIOTE_CONFIG_POLARITY_Msk | GPIOTE_CONFIG_OUTINIT_Msk;

	// LEDS
	nrf_gpio_cfg_output(RED);
	nrf_gpio_cfg_output(YELLOW);
	nrf_gpio_cfg_output(GREEN);
	nrf_gpio_cfg_output(BLUE);
	nrf_gpio_pin_set(RED);
	nrf_gpio_pin_set(YELLOW);
	nrf_gpio_pin_set(GREEN);
	nrf_gpio_pin_set(BLUE);
    	
	nrf_gpio_cfg_output(19);
	nrf_gpio_cfg_output(20);
	nrf_gpio_cfg_output(10);
	nrf_gpio_cfg_output(8);
	
	// dmp interrupt - h/w limitation, we cannot setup an interrupt since all gpiote channels are used for the motors
	nrf_gpio_cfg_input(INT_PIN, 0);
}

