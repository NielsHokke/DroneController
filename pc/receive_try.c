#include <stdio.h>
#include <string.h>
#define MAX_MESSAGE_SIZE 100

struct rx
{
    unsigned long int  timestamp; //%10ld (:10?)
    
    unsigned int ae1;            //%03d  (:2 bits in hex?)
    unsigned int ae2; 
    unsigned int ae3;
    unsigned int ae4;

    int phi;
    int theta;
    int psi;

    int sp;
    int sq;
    int sr;

    int voltage;
    int temperature; //assuming an int is 32 bits
    int pressure;
}

struct drone
{
    union
    {
        char raw[MAX_MESSAGE_SIZE]; //sizeof(largest possible message)
        struct rx message;
    }
}

void main()
{
    drone msg;
    char *try = "483453850 |   0   0   0   0 |  -1063  -5952  16550 |     -3     -2     -1 |  567 | 3048 | 101542";
    memcpy(newmsg.raw, try, 100);
    
}