
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h> 

#include "joystick.h"
#include "packet.h"
#include "terminals.c"

//INCLUDE FOR NON BLOCKING INPUTS from KEYBOARD
#include <termios.h>

/* time */
#include <time.h>
#include <assert.h>

#define JS_DEV	"/dev/input/js0"

#define OFFSET 1       // trim offest on single keypress
#define PAYLOAD_LEN 30 // length of packet over serial
#define P1 200         // velocity controller gain
#define P2 200         // position controller gain

bool terminate = false; //checks for ESC pattern

/* to do: keymaps
- Threads maybe?
- state machines
- check joystick connectivity
*/

/* current axis and button readings */
int	axis[6];
int	button[12];


unsigned int mon_time_ms(void)
{
    unsigned int    ms;
    struct timeval  tv;
    struct timezone tz;

    gettimeofday(&tv, &tz);
    ms = 1000 * (tv.tv_sec % 65); // 65 sec wrap around
    ms = ms + tv.tv_usec / 1000;
    return ms;
}

void mon_delay_ms(unsigned int ms)
{
    struct timespec req, rem;

    req.tv_sec = ms / 1000;
    req.tv_nsec = 1000000 * (ms % 1000);
    assert(nanosleep(&req,&rem) == 0);
}


int main (int argc, char **argv)
{
	packet_config_t packet;
	packet_config_t joystick;
	packet.p1 = P1;
	packet.p2 = P2;
	
	/*-----------------------------------------	
	Keyboard initialized in a different mode:
	https://www.raspberrypi.org/forums/viewtopic.php?t=177157&start=25
	-----------------------------------------*/
	// open both the terminal sessions (keyboard and serial port)
	init_keyboard();
	serial_open();

	char keypress = 0;  // ascii decimal of the key pressed
	int payload_len;
	char *payload;
	payload = (char *) malloc(PAYLOAD_LEN);
	/*------------------------------------------*/


	/*-------------------------------------------
				Initialize joystick 
	--------------------------------------------*/
	int fd;
	struct js_event js;
	unsigned int t, i;

	if ((fd = open(JS_DEV, O_RDONLY)) < 0) {
		perror("jstest");
		exit(1);
	}

	/* non-blocking mode joystick */
	fcntl(fd, F_SETFL, O_NONBLOCK);
	/*------------------------------------------*/


	/*--------------------------------------------
		Parse joystick and Keypress
	--------------------------------------------*/

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

	do {
		/* Parse KEYBOARD */

		// update variables only when read returns on keypress.
		int len = read(STDIN_FILENO, &keypress, 1); 

	    if (len == 1) {
	      switch (keypress) {
	        // numbers zero to nine
	        case 48 ... 57: packet.mode = keypress-48; break;
	        
	        case 'a'      : packet.lift = joystick.lift + OFFSET; break;
	        case 'z'      : packet.lift = joystick.lift - OFFSET; break;
	        
	        case 'q'      : packet.yaw = joystick.yaw + OFFSET; break;
	        case 'w'      : packet.yaw = joystick.yaw - OFFSET; break;

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
	        					packet.pitch = joystick.pitch - OFFSET;
	        				} 
	        				else if (keypress == 'B') {//bckwd
	        					packet.pitch = joystick.pitch + OFFSET; 
	        				}
	        				else if (keypress == 'C') {//roll negative CW
	        					packet.roll = joystick.roll - OFFSET;
	        				}
	        				else if (keypress == 'D') {//roll positive CCW
	        					packet.roll = joystick.roll + OFFSET;
	        				}
	        				else {
	        					// ESC button pressed
	        					terminate = true;
	        				}
	        				break;

	        default       : printf("unmapped key 0x%02x : %c\n", keypress, 
	                        (keypress >= 32 && keypress < 127) ? keypress : ' '); //terminate = true;
	                        break;
	      }
	    }

		/* Parse JOYSTICK */
		/* simulate work */
		mon_delay_ms(300);
		t = mon_time_ms();
		
		/* check up on JS */
		while (read(fd, &js, sizeof(struct js_event)) == sizeof(struct js_event)) {
			/* register data */
			// fprintf(stderr,".");
			switch(js.type & ~JS_EVENT_INIT) {
				case JS_EVENT_BUTTON:
					button[js.number] = js.value;
					break;
				case JS_EVENT_AXIS:
					axis[js.number] = js.value;
					break;
			}
		}
		if (errno != EAGAIN) {
			perror("\njs: error reading (EAGAIN)");
			exit (1);
		}

		printf("\n");
		
		// print millisec
		// printf("%5d ", t);
		/*
		for (i = 0; i < 6; i++) {
			printf("%6d ", axis[i]);
		}
		
		printf(" |  ");
		for (i = 0; i < 12; i++) {
			printf("%d ",button[i]);
		}
		*/
		joystick.yaw   = axis[1] >> 8;
		joystick.pitch = axis[2] >> 8;
		joystick.roll  = axis[3] >> 8;
		joystick.lift  = axis[4] >> 8; 
		printf("\033[0;34m joystick: %3d %3d %3d %3d\n\033[0m", 
				joystick.yaw,
				joystick.pitch,
				joystick.roll,
				joystick.lift);
		if (button[0]) //shoot or trigger button
			break;

		/*-----------------------------------------
	      compose packet 
	      int snprintf(char *str, size_t size, const char *format, ...);
	    ------------------------------------------*/
		joystick.yaw   = joystick.yaw + keyboard.yaw;
		joystick.pitch = joystick.pitch + keyboard.pitch;
		joystick.roll  = joystick.roll + keyboard.roll;
		joystick.lift  = joystick.lift + keyboard.lift;
        payload_len = snprintf(payload, PAYLOAD_LEN, 
    			  "%03d %03d %03d %03d %03d %03d %03d", 
    			  packet.mode, packet.lift, 
    			  packet.yaw, packet.pitch, packet.roll, 
    			  packet.p1, packet.p2);
	    if(payload_len <= PAYLOAD_LEN) {
	    	printf("\033[0;31m payload:  %s len: %d\n\033[0m", payload, payload_len);
	    	serial_putstring(payload, payload_len);
	    }
	    else {
	     	printf("increase payload len\n"); terminate = true;
	    }
    //usleep(100000); // Cut update rate to 10Hz
  	} while(!terminate); //while (ch != 27);


	/*----------------------------------------
				safely exit
	-----------------------------------------*/
	// Make sure no characters are left in the input stream as
	// plenty of keys emit ESC sequences, otherwise they'll appear
	// on the command-line after we exit.
	while(read(STDIN_FILENO, &keypress, 1)==1);
	printf("\033[1;31mexit\033[0m\n\n");

	free(payload);

	deinit_keyboard();
	serial_close();

	// todo: make sure state machine is changed

  	return 0;
}
