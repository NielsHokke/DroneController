
#include "in4073.h"

#define MAX_STR_LEN 20
#define MAX_INT_LEN 5

void print(char *string){
	int i=0;
	while ((*string != '\f') && (i++ <= MAX_STR_LEN)){
		uart_put(*string++);
	}
}

void printeger(int x){
	char string[MAX_INT_LEN + 2] = {0};
	if (x<0){
		string[0] = '-';
	 	x = ~x + 1;
	}else{
		string[0] = ' ';
	}

	for (int i=0; i < MAX_INT_LEN; i++){
		string[MAX_INT_LEN-i] =  '0' + x % 10;
		x = x/10;
	}
	string[MAX_INT_LEN+1] = '\f';
	print(string);
}

void uprinteger(uint x){
	char string[MAX_INT_LEN + 1] = {0};
	for (int i =0; i < MAX_INT_LEN; i++){
		string[MAX_INT_LEN-i] =  '0' + x % 10;
		x = x/10;
	}
	string[MAX_INT_LEN] = '\f';
	print(string);
}

