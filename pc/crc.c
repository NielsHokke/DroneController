// for unsigned 8 bit integer
#include <stdint.h>

uint8_t  crcTable[256];

#define WIDTH  (8 * sizeof(uint8_t))
#define TOPBIT (1 << (WIDTH - 1))
#define POLYNOMIAL 0xD8  /* 11011 followed by 0's */

void crcInit(void){
    uint8_t  remainder;
    for (int dividend = 0; dividend < 256; ++dividend)
    {
        remainder = dividend << (WIDTH - 8);
        for (uint8_t bit = 8; bit > 0; --bit){
            if (remainder & TOPBIT){
                remainder = (remainder << 1) ^ POLYNOMIAL;
            } else{
                remainder = (remainder << 1);
            }
        }
        crcTable[dividend] = remainder;
    }
}

uint8_t crcFast(char message[], int nBytes){
    uint8_t data;
    uint8_t remainder = 0;

    for (int byte = 0; byte < nBytes; ++byte){
        data = message[byte] ^ (remainder >> (WIDTH - 8));
        remainder = crcTable[data] ^ (remainder << 8);
    }

    return (remainder);
}