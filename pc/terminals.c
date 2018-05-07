#include <termios.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>

int fd_serial;

void serial_open(void)
{
  	char *name;
  	int result;
  	struct termios tty;

    fd_serial = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY);  // Hardcode your serial port here, or request it as an argument at runtime

	assert(fd_serial>=0);
	printf("\033[1;32mSerial Device Connected!\033[0m\n");
  	result = isatty(fd_serial);
  	assert(result == 1);

  	name = ttyname(fd_serial);
  	assert(name != 0);

  	result = tcgetattr(fd_serial, &tty);
	assert(result == 0);

	tty.c_iflag = IGNBRK; /* ignore break condition */
	tty.c_oflag = 0;
	tty.c_lflag = 0;

	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; /* 8 bits-per-character */
	tty.c_cflag |= CLOCAL | CREAD; /* Ignore model status + read input */

	cfsetospeed(&tty, B115200);
	cfsetispeed(&tty, B115200);

	tty.c_cc[VMIN]  = 0;
	tty.c_cc[VTIME] = 1; // added timeout

	tty.c_iflag &= ~(IXON|IXOFF|IXANY);

	result = tcsetattr(fd_serial, TCSANOW, &tty); /* non-canonical */

	tcflush(fd_serial, TCIOFLUSH); /* flush I/O buffer */
}

void serial_close(void)
{
  	int result;
  	result = close(fd_serial);
  	assert (result==0);
}

int	serial_getchar_nb()
{
	int result;
	unsigned char c;
	result = read(fd_serial, &c, 1);
	if (result == 0)
		return -1;
	else {
		assert(result == 1);
		return (int) c;
	}
}

int serial_getchar()
{
	int c;
	while ((c = serial_getchar_nb()) == -1);
	return c;
}

int serial_putchar(char c)
{
	int result;

	do {
		result = (int) write(fd_serial, &c, 1);
	} while (result == 0);

	assert(result == 1);
	return result;
}

/*pointer to string and
 length of string*/
int serial_putstring(char *s, int len)
{
	int result;
	result = (int) write(fd_serial, s, len);
	// assert(result == 1);
	return result;
}

/* Keyboard */
struct termios orig_term, raw_term;

void init_keyboard(void)
{
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
}

void deinit_keyboard(void)
{
	// Restore original terminal settings
	tcsetattr(STDIN_FILENO, TCSANOW, &orig_term);
}