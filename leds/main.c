/*
 * attiny406.c
 *
 * Created: 9/22/2020 5:06:26 PM
 * Author : jtperrotta
 */ 

#define F_CPU 20000000UL // ????
#include <avr/io.h>
#include <stdbool.h>
#include <stdio.h>

#include <util/delay.h>
#include <avr/interrupt.h>

volatile uint16_t _tx_delay = 0;

void init_timer(){
	TCA0.SINGLE.CTRLA = TCA_SINGLE_ENABLE_bm;
	TCA0.SINGLE.CTRLB = TCA_SINGLE_CMP2EN_bm | TCA_SINGLE_CMP1EN_bm | TCA_SINGLE_CMP0EN_bm | TCA_SINGLE_WGMODE1_bm | TCA_SINGLE_WGMODE0_bm;
	TCA0.SINGLE.CTRLC = TCA_SINGLE_CMP2OV_bm | TCA_SINGLE_CMP1OV_bm | TCA_SINGLE_CMP0OV_bm;
	TCA0.SINGLE.CMP0 = 0;
	TCA0.SINGLE.CMP1 = 0;
	TCA0.SINGLE.CMP2 = 0;
	TCA0.SINGLE.PER = 0xFF; // set top to 255
}

void init_AC(){
	AC0.CTRLA = AC_ENABLE_bm |AC_HYSMODE_50mV_gc |AC_INTMODE_NEGEDGE_gc ;
	AC0.MUXCTRLA = AC_MUXPOS_PIN1_gc | AC_MUXNEG_PIN1_gc;
	AC0.INTCTRL = 1;
}
uint8_t state_AC(){
	return (AC0.STATUS>>4)&1;
}
void initSerial(){
	_tx_delay = 0;
	uint16_t bit_delay = (F_CPU / 9600) / 4;
	if (bit_delay > 15 / 4)
		_tx_delay = bit_delay - (15 / 4);
	else
		_tx_delay = 1;
}
uint8_t readSerial(){
	cli();
	uint8_t ret = 0;
	_delay_loop_2(_tx_delay>>1);  
	_delay_loop_2(_tx_delay);
	for (uint8_t w = 0; w < 8;w++) {
		ret >>= 1;
		ret |= state_AC()?128:0;
		_delay_loop_2(_tx_delay);
	}
	sei();
	return ret;
}
void Send (uint8_t b) {
	uint8_t mask = 1<<3;
	uint8_t imask = ~mask;
	cli();
	PORTB.OUT &= imask;
	_delay_loop_2(_tx_delay);
	for (uint8_t i = 8; i > 0; --i) {
		if (b & 1)
		PORTB.OUT |= mask;
		else
		PORTB.OUT &= imask;
		_delay_loop_2(_tx_delay);
		b >>= 1;
	}
	PORTB.OUT |= mask;
	_delay_loop_2(_tx_delay);
	sei();
}

uint8_t recv[3] = {0};
volatile bool dos = true;
int main(void)
{
	CPU_CCP = 0xD8;
	CLKCTRL_MCLKCTRLA = CLKCTRL_CLKSEL_OSC20M_gc;
	CPU_CCP = 0xD8;
	CLKCTRL_MCLKCTRLB = 0;
	PORTB.DIR = 0b1111;
	PORTB.OUT |= (1<<3);
	PORTB.PIN4CTRL =  1 << 3;
	PORTB.PIN5CTRL =  1 << 3;

	init_AC();
	init_timer();
	initSerial();
	PORTC.PIN0CTRL = 1 << 3;
	PORTC.PIN1CTRL = 1 << 3;
	PORTC.PIN2CTRL = 1 << 3;
	PORTC.PIN3CTRL = 1 << 3;
	PORTA.PIN4CTRL = 1 << 3;
	PORTA.PIN5CTRL = 1 << 3;
	PORTA.PIN6CTRL = 1 << 3;
	PORTA.PIN7CTRL = 1 << 3;
	sei();
    while (1) 
    {
		while(1){
			while(dos);
			cli();
			AC0.INTCTRL = 0;
			uint8_t tmp = readSerial();
			AC0.INTCTRL = 1;
			sei();
			dos = true;
				TCA0.SINGLE.CTRLESET = (1 << 2); //reset timmer
			if(tmp == ((PORTC.IN & 0x0F) | (PORTA.IN & 0xF0)))break;
			
		}
		for(uint8_t b = 0; b < 3; b++){
			while(dos);
			cli();
			AC0.INTCTRL = 0;
			recv[b] = readSerial();
			AC0.INTCTRL = 1;
			sei();
			dos = true;
		}
		
		TCA0.SINGLE.CMP0 = recv[0];
		TCA0.SINGLE.CMP1 = recv[1];
		TCA0.SINGLE.CMP2 = recv[2];
		TCA0.SINGLE.CTRLESET = (1 << 2); //reset timmer
		/*char chr[30] = {0};
		sprintf(chr, "%d", recv[1]);
		for(uint8_t j = 0; j < sizeof(chr);j++){
			Send(chr[j]);
		}
		Send('\n');
		Send('\r');
*/
    }
}
ISR(AC0_AC_vect){
		dos = state_AC(); // fuster clucking on both edges even when set to neg
		AC0.STATUS = AC_CMP_bm; //clear interup flag
}

