/*
 * attiny406.c
 *
 * Created: 9/22/2020 5:06:26 PM
 * Author : jtperrotta
 */ 

#define F_CPU 20000000UL 
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
	TCA0.SINGLE.CMP0 = 128;
	TCA0.SINGLE.CMP1 = 128;
	TCA0.SINGLE.CMP2 = 128;
	TCA0.SINGLE.PER = 0xFF; // set top to 255
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
		ret |= (PORTA.IN & (1<<5))?128:0;
		_delay_loop_2(_tx_delay);
	}
	sei();
	return ret;
}
/*void Send (uint8_t b) {
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
}*/

uint8_t recv[3] = {0};
volatile bool dos = true;
int main(void)
{
	CPU_CCP = 0xD8;
	CLKCTRL_MCLKCTRLA = CLKCTRL_CLKSEL_OSC20M_gc;
	CPU_CCP = 0xD8;
	CLKCTRL_MCLKCTRLB = 0;
	PORTB.DIR = 0b111;
	init_timer();
	initSerial();
	PORTA.INTFLAGS = (1<<5);
	PORTA.PIN5CTRL = 11;

	sei();
    while (1) 
    {
		while(1){
			while(dos);
			cli();
			uint8_t tmp = readSerial();
			sei();
			dos = true;
			if(tmp == 0xff)break;
			
		}
		for(uint8_t b = 0; b < 3; b++){
			while(dos);
			cli();
			recv[b] = readSerial();
			sei();
			dos = true;
		}
		TCA0.SINGLE.CMP0 = recv[0];
		TCA0.SINGLE.CMP1 = recv[1];
		TCA0.SINGLE.CMP2 = recv[2];
		TCA0.SINGLE.CTRLESET = (1 << 2); //reset timmer
		/*char chr[30] = {0};
		sprintf(chr, "%u", recv[0]);
		for(uint8_t j = 0; j < sizeof(chr);j++){
			Send(chr[j]);
		}
		Send(10);*/
    }
}
ISR(PORTA_PORT_vect){
	dos = PORTA.IN & (1<<5); // fuster clucking on both edges even when set to neg
}

