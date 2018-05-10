// terminals io library (non blocking)
#include <termios.h>
#include <fcntl.h>      // macros for open()
#include <unistd.h>
#include <assert.h>
#include <time.h>

#define JS_DEV	"/dev/input/js0"

// if plans to make header, use extern
int fd_serial;
int fd_js; struct js_event js;

/*-------------------------------------------
		Initialize joystick 
--------------------------------------------*/
void init_js(void)
{
	fd_js = open(JS_DEV, O_RDONLY);

	assert(fd_js>=0);
	printf("\033[1;32mJoystick Connected!\033[0m\n");

	/* non-blocking mode joystick */
	fcntl(fd_js, F_SETFL, O_NONBLOCK);
}

/*---------------------------------------------
	timer functions for setting update frequency
-----------------------------------------------*/
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

/*------------------------------------------------
		serial for drone board
-------------------------------------------------*/
void init_serial(void)
{
  	char *name;
  	int result;
  	struct termios tty;

    fd_serial = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY); 

	assert(fd_serial>=0);
	printf("\033[1;32mSerial Device (drone board) Connected!\033[0m\n");
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

/*-------------------------------------------------
 initialize special non blocking non echoing Keyboard 
 ------------------------------------------------*/
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