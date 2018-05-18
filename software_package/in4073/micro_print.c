
#include "in4073.h"

#define MAX_STR_LEN 50
#define MAX_INT_LEN 5

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

