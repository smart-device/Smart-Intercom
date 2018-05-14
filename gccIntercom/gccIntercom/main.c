#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <string.h>

#define	rxEnable()	UCSR0B|=(1<<RXCIE0)|(1<<RXEN0)
#define	rxDisable()	UCSR0B&=~((1<<RXCIE0)|(1<<RXEN0))

const char AT[] PROGMEM = "AT\r\n";
const char ATE0[] PROGMEM = "ATE0\r\n";
const char CREG[] PROGMEM = "AT+CREG?\r\n";
const char CNMI[] PROGMEM = "AT+CNMI=0,0,0,0,0\r\n";
const char CAMCFG[] PROGMEM = "AT+CAMCFG=0,1\r\n";
const char CAMSTART[] PROGMEM = "AT+CAMSTART=1\r\n";
const char CRLF[] PROGMEM = "\r\n";
const char OK[] PROGMEM = "\r\nOK\r\n";
const char ERROR[] PROGMEM = "ERROR";
const char CGDCONT[] PROGMEM = "AT+CGDCONT=1,\"IP\",\"mtnirancell\"\r\n";
const char CSTT[] PROGMEM = "AT+CSTT=\"mtnirancell\",\"\",\"\"\r\n";
const char CIICR[] PROGMEM = "AT+CIICR\r\n";
const char CIFSR[] PROGMEM = "AT+CIFSR\r\n";
const char CONNECT[] PROGMEM = "CONNECT ";
const char RST[] PROGMEM = "AT+RST=1\r\n";
const char WEB[] PROGMEM = "AT+CIPSTART=\"TCP\",\"smart-device.ir\",80\r\n";
const char CLOSE[] PROGMEM = "AT+CIPCLOSE\r\n";
const char SEND[] PROGMEM = "AT+CIPSEND\r\n";
const char GET[] PROGMEM = "GET /ic/mhsn.php?i=";
const char HTTP[] PROGMEM = " HTTP/1.1\r\n";
const char HOST[] PROGMEM = "Host: smart-device.ir\r\n\r\n";
const char CODE1[] PROGMEM = "UUU1";
const char CODE2[] PROGMEM = "UUU2";
const char CODE3[] PROGMEM = "UUU3";
const char ATCAMCAP[] PROGMEM = "AT+CAMCAP\r\n";
const char CAMCAP[] PROGMEM = "CAMCAP:";
const char ATCAMPOST[] PROGMEM = "AT+CAMPOST=\"smart-device.ir/ic/up.php?n=0.jpg\",80\r\n";
const char CAMPOST[] PROGMEM = "+CAMPOST:";
const char CAMPOST0[] PROGMEM = "+CAMPOST:0";
const char CAMPOST1[] PROGMEM = "+CAMPOST:1";
const char NORESPONSE[] PROGMEM = "COMMAND NO RESPONSE!\r\n";

volatile unsigned char rxBuff[256] = { 0 }, rxIndx = 0, pb2 = 1;

void(*rstMicro) (void) = 0; // Reset uC by jumping to address 0

void init(void) {
	DDRB |= 0x01;					// GREEN LED
	PORTB |= 0x04;					// Switch
	DDRD |= 0x80;					// RED LED

	PCICR  = (1<<PCIE0);	PCMSK0 = (1<<PCINT2);	PCIFR  = (1<<PCIF0);

	TCCR1B = (0 << ICNC1) | (0 << ICES1) | (0 << WGM13) | (0 << WGM12) | (0 << CS12) | (1 << CS11) | (1 << CS10);	// ~1Hz@ 3.686400MHz
	TCNT1H = 0x00; TCNT1L = 0x00; ICR1H = 0x00; ICR1L = 0x00; OCR1AH = 0x00; OCR1AL = 0x00; OCR1BH = 0x00; OCR1BL = 0x00;

	UCSR0B = (1 << TXEN0);	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
	UBRR0H = 0x00;	UBRR0L = 0x17;	// BaudRate=9600bps  @ 3.686400MHz

	WDTCSR = (0 << WDIF) | (0 << WDIE) | (1 << WDP3) | (1 << WDCE) | (1 << WDE) | (0 << WDP2) | (0 << WDP1) | (1 << WDP0);
	WDTCSR = (0 << WDIF) | (0 << WDIE) | (1 << WDP3) | (0 << WDCE) | (1 << WDE) | (0 << WDP2) | (0 << WDP1) | (1 << WDP0);
	
	sei();
}

void USART_Tx(unsigned char data) {
	while (!(UCSR0A & (1 << UDRE0)));	// Wait for empty transmit buffer
	UDR0 = data;						// Put data into buffer, sends the data
}

void USART_TxStr(unsigned char *data) {
	while (*data != '\0')
		USART_Tx(*data++);
}

void USART_TxStr_P(const char *data) {
	while (pgm_read_byte(data) != 0x00)
		USART_Tx(pgm_read_byte(data++));
}

void flushBuff(void) {
	rxIndx = 0;
	for (unsigned char i = 0; i<255; i++)
		rxBuff[i] = 0;
}

void rstA6C(void) {
	DDRC |= 0x02;	// A6C's rst pin
	USART_TxStr_P(RST);	_delay_ms(100);
	USART_TxStr_P(RST);	_delay_ms(100);
	DDRC &= 0xFD;
	rstMicro();
}

unsigned char a6c(const char *cmd, const char *res, unsigned char timeout) {
	unsigned char r = 1;
	flushBuff();
	rxEnable();
	for (unsigned char retryCntr = 0; retryCntr < timeout;) {
		asm volatile("wdr");
		if(TIFR1&0x01) {
			USART_TxStr_P(cmd);
			TIFR1 |= 0x01;
			retryCntr++;
		}
		if (strstr_P(rxBuff, res))	r = 0;
	}
	rxDisable();
	return r;
}

void closeConnections(void){
	unsigned char retryCntr = 0;
	flushBuff();
	USART_TxStr_P(CLOSE);
	rxEnable();
	while (!(strstr_P(rxBuff, OK) || strstr_P(rxBuff, ERROR)) && retryCntr < 10) {
		asm volatile("wdr");
		if(TIFR1&0x01) {
			TIFR1 |= 0x01;
			retryCntr++;
		}
	}
	rxDisable();
}

int main(void) {
	unsigned char takePic = 1, retryCntr = 0;
	
	init();
	
	if (a6c(AT, OK, 30))
		rstA6C();
	
	if (a6c(ATE0, OK, 5))
		rstA6C();
	
	if (a6c(CREG, PSTR("+CREG: 1"), 30))
		rstMicro();
	
	if (a6c(CNMI, OK, 5))
		rstA6C();
	
	if (a6c(CAMCFG, OK, 5))
		rstMicro();
	
	if (a6c(CAMSTART, CRLF, 10))
		rstMicro();
	_delay_ms(10);
	
	if (a6c(CGDCONT, OK, 5))
		rstMicro();
	
	if (a6c(CSTT, OK, 5))
		rstMicro();
	
	if (a6c(CIICR, CRLF, 60))
		rstMicro();
	_delay_ms(100);
	
	USART_TxStr_P(CIFSR);
	flushBuff();
	rxEnable();
	while (!strstr(rxBuff, ".") && retryCntr < 10) {
		asm volatile("wdr");
		if(TIFR1&0x01) {
			TIFR1 |= 0x01;
			retryCntr++;
		}
	}
	rxDisable();
	if(retryCntr >= 10)
		rstA6C();
	_delay_ms(10);
	
	for(;;){
		if(takePic) {
			USART_TxStr_P(ATCAMCAP);
			flushBuff();
			rxEnable();
			while (!strstr_P(rxBuff, CAMCAP) && retryCntr < 5) {
				asm volatile("wdr");
				if(TIFR1&0x01) {
					TIFR1 |= 0x01;
					retryCntr++;
				}
			}
			rxDisable();
			if(strstr_P(rxBuff, ERROR) || (retryCntr >= 5) )
				rstA6C();
			_delay_ms(100);
			
			closeConnections();
			
			USART_TxStr_P(ATCAMPOST);
			flushBuff();
			rxEnable();
			retryCntr = 0;
			while (!(strstr_P(rxBuff, NORESPONSE) || strstr_P(rxBuff, ERROR) || strstr_P(rxBuff, CAMPOST)) && retryCntr < 60) {
				asm volatile("wdr");
				if(TIFR1&0x01) {
					TIFR1 |= 0x01;
					retryCntr++;
				}
			}
			_delay_ms(100);
			rxDisable();
			if(strstr_P(rxBuff, CAMPOST0) || (retryCntr >= 60) )
				rstA6C();
			if(strstr_P(rxBuff, CAMPOST1))	takePic = 0;
		}
		
		closeConnections();
		
		retryCntr = 0;
		flushBuff();
		USART_TxStr_P(WEB);
		rxEnable();
		while (!(strstr_P(rxBuff, OK) || strstr_P(rxBuff, ERROR)) && retryCntr < 10) {
			asm volatile("wdr");
			if(TIFR1&0x01) {
				TIFR1 |= 0x01;
				retryCntr++;
			}
		}
		if (strstr_P(rxBuff, OK)) {
			retryCntr = 0;
		}
		rxDisable();
		if (retryCntr >= 10) {
			rstA6C();
		} else {
			flushBuff();
			rxEnable();
			USART_TxStr_P(SEND);
			for (retryCntr = 0; retryCntr < 10;) {
				if (strstr(rxBuff, ">")) {
					USART_TxStr_P(GET);
					if(pb2) {
						USART_Tx('3');
					} else {
						USART_Tx('1');
						takePic = 1;
						pb2 = 1;
					}
					USART_TxStr_P(HTTP);
					USART_TxStr_P(HOST);
					USART_Tx(0x1A);
					USART_TxStr_P(CRLF);
					
					flushBuff();
					rxEnable();
					for (retryCntr = 0; retryCntr < 10;) {
						if (strstr_P(rxBuff, CODE1)) {
							PORTD &= 0x7F;	_delay_ms(50);	PORTB |= 0x01;
							break;
						} else if (strstr_P(rxBuff, CODE2)) {
							PORTB &= 0xFE;	_delay_ms(50);	PORTD |= 0x80;
							break;
						} else if (strstr_P(rxBuff, CODE3)) {
							takePic = 1;
							break;
						}
						asm volatile("wdr");
						if(TIFR1&0x01) {
							TIFR1 |= 0x01;
							retryCntr++;
						}
					}
					rxDisable();
				}
				asm volatile("wdr");
				if(TIFR1&0x01) {
					TIFR1 |= 0x01;
					retryCntr++;
				}
			}
			rxDisable();
			
			closeConnections();
		}
	}
}

ISR(USART_RX_vect) {
	unsigned char udr = UDR0;
	if (!(UCSR0A & ((1<<FE0) | (1<<UPE0) | (1<<DOR0))))
		rxBuff[rxIndx++] = udr;
}

ISR(PCINT0_vect) {
	if(!(PINB & (1 << 2))) {
		pb2 = 0;
	}
}
