
#ifndef _MICROP_PRINT
#define _MICROP_PRINT


void downLink(uint8_t mode, int16_t m1, int16_t m2,int16_t m3,int16_t m4, int16_t pitch, int16_t yaw, int16_t roll, uint16_t bat_volt, uint32_t time);
void print(char *string);
void printeger(int x, uint8_t length);
void uprinteger(uint x, uint8_t length);


#endif