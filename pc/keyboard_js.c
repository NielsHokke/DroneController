#include <stdlib.h>   // for malloc
#include <stdio.h>
#include <errno.h>    // for checking joystick/serial connectivity
#include <string.h>
#include <stdbool.h> 

#include "joystick.h"
#include "packet.h"
#include "terminals.c"
#include "crc.c"

#define OFFSET 1       // trim offest on single keypress
#define PAYLOAD_LEN 30 // length of packet over serial (with crc)
#define P1 200         // velocity controller gain
#define P2 200         // position controller gain

// see all terminals print everything
#define DEBUG 0

bool terminate = false; //checks for ESC pattern

// joystick: current axis and button readings
int	axis[6];
int	button[12];

/* to do: keymaps
- Threads maybe?
- state machines
- check joystick connectivity
*/

int main (int argc, char **argv)
{
	packet_config_t packet;
	packet_config_t joystick;
	packet_config_t keyboard;

	// initialize final packet irrespective of kb and js
	packet.p1 = P1;
	packet.p2 = P2;
	packet.mode = 1;
	
	/*-----------------------------------------	
	Keyboard initialized in a different mode:
	https://www.raspberrypi.org/forums/viewtopic.php?t=177157&start=25
	-----------------------------------------*/
	// initialize terminals and IO devices
	init_keyboard();
	init_serial();
	init_js();

	// declare variables used in the while loop
	char keypress = 0;  // ascii decimal of the key pressed
	
	char *payload;		// normal payload
	int payload_len;
	payload = (char *) malloc(PAYLOAD_LEN-1);
	
	char *payload_crc;   // appended payload with crc
	int payload_crc_len;
	payload_crc = (char *) malloc(PAYLOAD_LEN);

	/* counter for received buffer */
	int n = 0; int spot = 0;
	char buf = '\0';

	/* Whole response*/
	char response[200];
	memset(response, '\0', 200 * sizeof(char));

	// initalize the crc lookup table
	crcInit();
	uint8_t crcByte;

	/*--------------------------------------------
		Parse i) keypress
			 ii) joystick
			iii) serial
	--------------------------------------------*/
	do {
		// i) Parse KEYBOARD

		// update variables only when read returns on keypress (len=1)
		int len = read(STDIN_FILENO, &keypress, 1); 

	    if (len == 1) {
	      switch (keypress) {
			/* keymaps
			• ESC: abort / exit
			• 0 mode 0, 1 mode 1, etc. (random access)
			• a/z: lift up/down
			• left/right arrow: roll up/down
			• up/down arrow: pitch down/up (cf. stick)
			• q/w: yaw down/up
			• u/j: yaw control P up/down
			• i/k: roll/pitch control P1 up/down
			• o/l: roll/pitch control P2 up/down
			*/

	        // numbers zero to nine
	        case 48 ... 57: packet.mode = keypress-48; break;
	        
	        case 'a'      : keyboard.lift = keyboard.lift + OFFSET; break;
	        case 'z'      : keyboard.lift = keyboard.lift - OFFSET; break;
	        
	        case 'q'      : keyboard.yaw = keyboard.yaw + OFFSET; break;
	        case 'w'      : keyboard.yaw = keyboard.yaw - OFFSET; break;

	        /*
	        case 'u'      : packet.p1 = packet.p1 + OFFSET; break;
	        case 'j'      : packet.p1 = packet.p1 - OFFSET; break;
			*/

	        case 'i'      : packet.p1 = packet.p1 + OFFSET; break;
	        case 'k'      : packet.p1 = packet.p1 - OFFSET; break;

	        case 'o'      : packet.p2 = packet.p2 + OFFSET; break;
	        case 'l'      : packet.p2 = packet.p2 - OFFSET; break;

	        case 27		  : read(STDIN_FILENO, &keypress, 1);  // ignore keypressaracter '['
	        				read(STDIN_FILENO, &keypress, 1);  // up=A, dn=B, rght=C, left=D 
	        				if (keypress == 'A') {//go fwd
	        					keyboard.pitch = keyboard.pitch - OFFSET;
	        				} 
	        				else if (keypress == 'B') {//bckwd
	        					keyboard.pitch = keyboard.pitch + OFFSET; 
	        				}
	        				else if (keypress == 'C') {//roll negative CW
	        					keyboard.roll = keyboard.roll - OFFSET;
	        				}
	        				else if (keypress == 'D') {//roll positive CCW
	        					keyboard.roll = keyboard.roll + OFFSET;
	        				}
	        				else {
	        					// ESC button pressed
	        					packet.mode = 0; // safe mode
	        					terminate = true;
	        				}
	        				break;

	        default       : printf("unmapped key 0x%02x : %c\n", keypress, 
	                        (keypress >= 32 && keypress < 127) ? keypress : ' '); //terminate = true;
	                        break;
	      }
	    }
		
		// ii) Parse joystick
		while (read(fd_js, &js, sizeof(struct js_event)) == sizeof(struct js_event)) {
			switch(js.type & ~JS_EVENT_INIT) {
				case JS_EVENT_BUTTON:
					button[js.number] = js.value;
					break;
				case JS_EVENT_AXIS:
					axis[js.number] = js.value;
					break;
			}
		}

		// downgrade joystick resolution
		joystick.yaw   = (axis[2] >> 8) + 128;
		joystick.pitch = (axis[1] >> 8) + 128;
		joystick.roll  = (axis[0] >> 8) + 128;
		joystick.lift  = 255 - ((axis[3] >> 8) + 128); 

		// set the trim from keyboard inputs
		packet.yaw   = joystick.yaw   + keyboard.yaw   ;
		packet.pitch = joystick.pitch + keyboard.pitch ;
		packet.roll  = joystick.roll  + keyboard.roll  ;
		packet.lift  = joystick.lift  + keyboard.lift  ;

		if (button[0]) //shoot or trigger button
			packet.mode = 0; // safe mode
			break;

		/*-----------------------------------------
	      compose packet 
	      int snprintf(char *str, size_t size, const char *format, ...);
	    ------------------------------------------*/
		#if DEBUG == 1
		printf("yprl: %03d %03d %03d %03d\n",
				      packet.yaw, 
	    			  packet.pitch, 
	    			  packet.roll, 
	    			  packet.lift);
		#endif
        payload_len = snprintf(payload, PAYLOAD_LEN, 
	    			  "%02x%02x%02x%02x%02x", 
	    			  0xAA,   			   
	    			  packet.yaw, 
	    			  packet.pitch, 
	    			  packet.roll, 
	    			  packet.lift);
        
        crcByte = crcFast(payload, payload_len);
        payload_crc_len = snprintf(payload_crc, PAYLOAD_LEN, 
        						"%s%02x", payload, crcByte);// crcByte);
	    
	   	#if DEBUG == 1
    	printf("\033[0;31m payload : %s len: %d\n\033[0m", 
    			payload_crc, payload_crc_len);
    	#endif

	    if(payload_crc_len > PAYLOAD_LEN) {
	    	printf("\033[0;31m check payload lengths\n\033[0m");
	    	terminate = true;
	    }

    	serial_putstring(payload_crc, payload_crc_len);

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
    

		// maybe for timeout: 
		if (errno != EAGAIN) {
			perror("\njs: error reading (EAGAIN)");
			deinit_keyboard();
			serial_close();
			exit(1);
		}

		unsigned int t, i;

		// set frequency
		mon_delay_ms(300);
		t = mon_time_ms();

  	} while(!terminate); //while (ch != 27);


	/*----------------------------------------
				safely exit
	-----------------------------------------*/

	/* Make sure no characters are left in the input stream as
	plenty of keys emit ESC sequences, otherwise they'll appear
	on the command-line after we exit.*/
	while(read(STDIN_FILENO, &keypress, 1)==1);
	printf("\033[1;31mexit\033[0m\n\n");

	free(payload);
	deinit_keyboard();
	serial_close();

	// todo: make sure state machine is changed

  	return 0;
}


//#include <fcntl.h>
//#include <unistd.h>
//#include <sys/ioctl.h>
//#include <sys/types.h>

/* 
// print millisec
printf("%5d ", t);
// print all axes
for (i = 0; i < 6; i++) {
	printf("%6d ", axis[i]);
}
// print all buttons 
printf(" |  ");
for (i = 0; i < 12; i++) {
	printf("%d ",button[i]);
}
*/