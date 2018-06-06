
#include "in4073.h"

#define MAX_STR_LEN 50
#define MAX_INT_LEN 5


void downLink(uint8_t mode, int16_t m1, int16_t m2,int16_t m3,int16_t m4, int16_t pitch, int16_t yaw, int16_t roll){
	uart_put(0x02);
	uart_put(0x02);
	uart_put(mode);

	uart_put(m1 >> 8 & 0xFF);
	uart_put(m1 >> 0 & 0xFF);

	uart_put(m2 >> 8 & 0xFF);
	uart_put(m2 >> 0 & 0xFF);

	uart_put(m3 >> 8 & 0xFF);
	uart_put(m3 >> 0 & 0xFF);

	uart_put(m4 >> 8 & 0xFF);
	uart_put(m4 >> 0 & 0xFF);

	uart_put(pitch >> 8 & 0xFF);
	uart_put(pitch >> 0 & 0xFF);

	uart_put(yaw >> 8 & 0xFF);
	uart_put(yaw >> 0 & 0xFF);
	
	uart_put(roll >> 8 & 0xFF);
	uart_put(roll >> 0 & 0xFF);

	// Flush uart
	uart_put('\n');
}

void print(char *string){
	int i=0;
	while ((*string != '\f') && (i++ <= MAX_STR_LEN)){
		uart_put(*string++);
	}
}

void printeger(int x, uint8_t lenght){
	char string[lenght + 2];
	if (x<0){
		string[0] = '-';
	 	x = ~x + 1;
	}else{
		string[0] = ' ';
	}

	for (int i=0; i < lenght; i++){
		string[lenght-i] =  '0' + x % 10;
		x = x/10;
	}
	string[lenght+1] = '\f';
	print(string);
}

void uprinteger(uint x, uint8_t lenght){
	char string[lenght + 1];
	for (int i =0; i < lenght; i++){
		string[lenght-i-1] =  '0' + x % 10;
		x = x/10;
	}
	string[lenght] = '\f';
	print(string);
}

