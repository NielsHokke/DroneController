#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include "packet.h"

#define OFFSET 10      // trim offest on single keypress
#define PAYLOAD_LEN 20 // length of packet over serial
#define P1 200         // velocity controller gain
#define P2 200         // position controller gain

int main(void)
{
  packet_config_t packet;
  packet.p1 = P1;
  packet.p2 = P2;

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

  char ch = 0;
  int payload_len;
  char *payload;
  payload = (char *) malloc(PAYLOAD_LEN);

  do {
    // update variables only when read returns on ch.
    int len = read(STDIN_FILENO, &ch, 1); 

    if (len == 1) {
      switch (ch) {
        // numbers zero to nine
        case 48 ... 57: packet.mode = ch-48; break;
        case 'a'      : packet.lift = packet.lift + OFFSET; break;
        case 'z'      : packet.lift = packet.lift - OFFSET; break;  
        case 'i'      : packet.p1 = packet.p1 + OFFSET; break;
        case 'k'      : packet.p1 = packet.p1 - OFFSET; break;
        case 'o'      : packet.p2 = packet.p2 + OFFSET; break;
        case 'l'      : packet.p2 = packet.p2 - OFFSET; break;
        default       : printf("unknown key 0x%02x : %c\n", ch, 
                        (ch >= 32 && ch < 127) ? ch : ' '); 
                        break;
      }
    }

    /*-----------------------------------------
      compose packet 
    ------------------------------------------*/
    /*
    int snprintf(char *str, size_t size, const char *format, ...);
    */
    // remember to fix field length
    payload_len = snprintf(payload, PAYLOAD_LEN, "%03d %03d %03d %03d", packet.mode, packet.lift, packet.p1, packet.p2);
    if(payload_len <= PAYLOAD_LEN) {
     printf("payload: %s len: %d\n", payload, payload_len);
    }
    else {
      printf("increase payload len\n");
    }
    sleep(0.4);
  } while (ch != 27);



  // todo: send over serial
  // Make sure no characters are left in the input stream as
  // plenty of keys emit ESC sequences, otherwise they'll appear
  // on the command-line after we exit.
  while(read(STDIN_FILENO, &ch, 1)==1);
  
  free(payload);
  printf("exit\n");

  // Restore original terminal settings
  tcsetattr(STDIN_FILENO, TCSANOW, &orig_term);
  return 0;
}