#define _GNU_SOURCE
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h> 
#include "packet.h"
#include "terminals.c"
#include <string.h>

#define OFFSET 1       // trim offest on single keypress
#define PAYLOAD_LEN 30 // length of packet over serial
#define P1 200         // velocity controller gain
#define P2 200         // position controller gain

bool terminate = false; //checks for ESC pattern

int main(void)
{
  packet_config_t packet;
  packet.p1 = P1;
  packet.p2 = P2;

  // open both the terminal sessions (keyboard and serial port)
  init_keyboard();
  init_serial();
  /*
  • 0 mode 0, 1 mode 1, etc. (random access) 
  • a/z: lift up/down 
  • left/right arrow: roll up/down 
  • up/down arrow: pitch down/up (cf. stick)
  • q/w: yaw down/up
  • u/j: yaw control P up/down
  • i/k: roll/pitch control P1 up/down
  • o/l: roll/pitch control P2 up/down
  */

  char ch = 0;
  int payload_len;
  char *payload;
  payload = (char *) malloc(PAYLOAD_LEN * sizeof(char));

  char *rx_c;
  rx_c = (char *) malloc(200 * sizeof(char));
  int result = 0; 

 // char buffer[200];


	int n = 0; int spot = 0;
	char buf = '\0';

	/* Whole response*/
	char response[200];
	memset(response, '\0', 200 * sizeof(char));

  do {
    // update variables only when read returns on ch.
    int len = read(STDIN_FILENO, &ch, 1); 

    if (len == 1) {
      switch (ch) {
        // numbers zero to nine
        case 48 ... 57: packet.mode = ch-48; break;
        
        case 'a'      : packet.lift = packet.lift + OFFSET; break;
        case 'z'      : packet.lift = packet.lift - OFFSET; break;
        
        case 'q'      : packet.yaw = packet.yaw + OFFSET; break;
        case 'w'      : packet.yaw = packet.yaw - OFFSET; break;

        /*
        case 'u'      : packet.p1 = packet.p1 + OFFSET; break;
        case 'j'      : packet.p1 = packet.p1 - OFFSET; break;
		*/

        case 'i'      : packet.p1 = packet.p1 + OFFSET; break;
        case 'k'      : packet.p1 = packet.p1 - OFFSET; break;

        case 'o'      : packet.p2 = packet.p2 + OFFSET; break;
        case 'l'      : packet.p2 = packet.p2 - OFFSET; break;

        case 27		    : read(STDIN_FILENO, &ch, 1);  // ignore character '['
                				read(STDIN_FILENO, &ch, 1);  // up=A, dn=B, rght=C, left=D 
                				if (ch == 'A') {//go fwd
                					packet.pitch = packet.pitch - OFFSET;
                				} 
                				else if (ch == 'B') {//bckwd
                					packet.pitch = packet.pitch + OFFSET; 
                				}
                				else if (ch == 'C') {//roll negative CW
                					packet.roll = packet.roll - OFFSET;
                				}
                				else if (ch == 'D') {//roll positive CCW
                					packet.roll = packet.roll + OFFSET;
                				}
                				else {
                					// ESC button pressed
                					terminate = true;
                				}
                				break;

        default       : printf("unmapped key 0x%02x : %c\n", ch, 
                        (ch >= 32 && ch < 127) ? ch : ' '); //terminate = true;
                        break;
      }
    }

    /*-----------------------------------------
     	read from drone //put read packet size
    ------------------------------------------*/
	spot = 0;
	do {
		n = read(fd_serial, &buf, 1 );
		sprintf( &response[spot], "%c", buf );
		spot += n;
	} while( buf != '\n' && n > 0);

	if (n < 0) {
		printf("\033[1;31mError in reading, errno: d\033[0m\n");
		// switch mode 
		break;

	}
	else if (n == 0) {
		printf("\033[1;31mTimeout: read from Serial - maybe flash chip again\033[0m\n");
		// switch mode 
		break;
	}
	else {
		printf("\033[0;33mrx: %s, len %d\033[0m\n", response, spot);
	}

	/* to do"
	1) impl delay?
    3) parse recieved
    4) port makefile
    5) without js code?
	*/

    /*----------------------------------------
    			compose packet 
    ------------------------------------------
    int snprintf(char *str, size_t size, const char *format, ...);
    // remember to fix field length
    */

    payload_len = snprintf(payload, PAYLOAD_LEN, 
    			  "%03d %03d %03d %03d %03d %03d %03d", 
    			  packet.mode, packet.lift, 
    			  packet.yaw, packet.pitch, packet.roll, 
    			  packet.p1, packet.p2);
    if(payload_len <= PAYLOAD_LEN) {
    	printf("\033[0;32mpayload: %s len: %d\033[0m\n", payload, payload_len);
    	serial_putstring(payload, payload_len);
    }
    else {
     	printf("increase payload len\n"); terminate = true;
    }
    // usleep(100000); // Cut update rate to 10Hz
  } while(!terminate); //while (ch != 27);

  /*----------------------------------------
  				safely exit
  -----------------------------------------*/
  // Make sure no characters are left in the input stream as
  // plenty of keys emit ESC sequences, otherwise they'll appear
  // on the command-line after we exit.
  while(read(STDIN_FILENO, &ch, 1)==1);
  printf("\033[1;31mexit\033[0m\n\n");

  free(payload);
  free(rx_c);
  deinit_keyboard();
  serial_close();

  return 0;
}