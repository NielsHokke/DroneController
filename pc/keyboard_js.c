
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "joystick.h"
#include "packet.h"

//INCLUDE FOR NON BLOCKING INPUTS from KEYBOARD
#include <termios.h>

/* time */
#include <time.h>
#include <assert.h>

#define JS_DEV	"/dev/input/js0"

#define OFFSET 10 //trim offest on single keypress
#define PAYLOAD_LEN 20 //length of packet over serial

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
	/*-----------------------------------------	
	Keyboard initialized in a different mode:
	https://www.raspberrypi.org/forums/viewtopic.php?t=177157&start=25
	-----------------------------------------*/
	struct termios orig_term, raw_term;

	// Get terminal settings and save a copy for later
	tcgetattr(STDIN_FILENO, &orig_term);
	raw_term = orig_term;

	// Turn off echoing and canonical mode
	raw_term.c_lflag &= ~(ECHO | ICANON);

	// Set min character limit and timeout to 0 so read() returns immediately
	// whether there is a character available or not
	raw_term.c_cc[VMIN] = 0;
	raw_term.c_cc[VTIME] = 0;

	// Apply new terminal settings
	tcsetattr(STDIN_FILENO, TCSANOW, &raw_term);

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
		    case 'a'      : packet.lift = packet.lift + OFFSET; break;
		    case 'z'      : packet.lift = packet.lift - OFFSET; break;  
		    case 'i'      : packet.p1 = packet.p1 + OFFSET; break;
		    case 'k'      : packet.p1 = packet.p1 - OFFSET; break;
		    case 'o'      : packet.p2 = packet.p2 + OFFSET; break;
		    case 'l'      : packet.p2 = packet.p2 - OFFSET; break;
		    default       : printf("unknown key 0x%02x : %c\n", keypress, 
		                    (keypress >= 32 && keypress < 127) ? keypress : ' '); 
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
		printf("%5d   ", t);
		for (i = 0; i < 6; i++) {
			printf("%6d ",axis[i]);
		}
		printf(" |  ");
		for (i = 0; i < 12; i++) {
			printf("%d ",button[i]);
		}
		if (button[0])
			break;

	/*-----------------------------------------
      compose packet 
      int snprintf(char *str, size_t size, const char *format, ...);
    ------------------------------------------*/

    payload_len = snprintf(payload, PAYLOAD_LEN, "%03d %03d %03d %03d", packet.mode, packet.lift, packet.p1, packet.p2);
    if(payload_len <= PAYLOAD_LEN) {
     printf("payload: %s len: %d\n", payload, payload_len);
    }
    else {
      printf("increase payload len\n");
    }
    sleep(0.4);
    //serialSend(payload);

	} while (keypress != 27); // except ESC and DEL

	/* ----------------------------------------
				safely exit
	------------------------------------------*/
	free(payload);

	printf("\n<exit>\n");
	// Make sure no characters are left in the input stream as
	// plenty of keys emit ESC sequences, otherwise they'll appear
	// on the command-line after we exit.
	while(read(STDIN_FILENO, &keypress, 1)==1);

	// Restore original terminal settings
	tcsetattr(STDIN_FILENO, TCSANOW, &orig_term);

	// todo: make sure state machine is changed

  	return 0;
}
