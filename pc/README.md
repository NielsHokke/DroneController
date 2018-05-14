# PC side 

- compiled with standard preinstalled gcc with Ubuntu 16.04.
- commands like ctrl+D to exit the terminal don't work anymore (keyboard inputs flow in a non-blocking mode now)
- text marked with (grep "todo" *.c) is yet to be implemented (serial comm) 
- sanity check for formatting yet to be checked
- Timeout partially/indirectly works
- Timeout implementation pending: https://lnguin.wordpress.com/2013/06/06/linux-serial-programming-with-read-timeout/
- Yet to parse the received packets to struct. 

1. `keyboard_js.c`: Both keyboard and joystick to control the drone (supposed to be the final code)
2. `keyboard.c`   : We don't have the joystick always, use only keyboard to send packets.
3. `js.c`         : Pre-implemented (unmodified) code from course website
4. `js_test.c`    : library of js alongwith `joystick.h`
5. `packet.h`     : new header with intention to keep the source code in `keyboard_js.c` clean

