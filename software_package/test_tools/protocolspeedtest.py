import serial
import time

with serial.Serial('/dev/ttyUSB4', 115200, timeout=1) as ser:
	while(True):
		ser.write("\xAA\x01\x02\x03\x04\xe0")
		time.sleep(.005)