#!/usr/bin/env python
import threading
import time
import serial
import pygame

ser = serial.Serial('COM4', 19200, timeout=1)

Running = True

PITCH = 1
YAW = 3
ROLL = 0
LIFT = 2


class ControlThread(threading.Thread):
    def run(self):
        pitch_byte = b'\x00'
        yaw_byte = b'\x00'
        roll_byte = b'\x00'
        lift_byte = b'\x00'

        pygame.init()

        has_joystick = True
        if pygame.joystick.get_count() < 1:
            print("No joystick detected. Keyboard only mode")
            has_joystick = False
        if has_joystick:
            pygame.joystick.init()
            joystick = pygame.joystick.Joystick(0)
            joystick.init()

        while Running:
            ser.write(b'hello')

            if has_joystick:
                for event in pygame.event.get():  # User did something
                    # Possible joystick actions: JOYAXISMOTION JOYBALLMOTION JOYBUTTONDOWN JOYBUTTONUP JOYHATMOTION
                    if event.type == pygame.JOYBUTTONDOWN:
                        print("Joystick button pressed.")
                    if event.type == pygame.JOYBUTTONUP:
                        print("Joystick button released.")
                    if event.type == pygame.JOYAXISMOTION:
                        print("Joystick change")
                        pitch_byte = (round(joystick.get_axis(PITCH)*127.5)).to_bytes(1, byteorder='big', signed=True)
                        yaw_byte = (round(joystick.get_axis(YAW)*127.5)).to_bytes(1, byteorder='big', signed=True)
                        roll_byte = (round(joystick.get_axis(ROLL)*127.5)).to_bytes(1, byteorder='big', signed=True)
                        lift_byte = (255 - round((joystick.get_axis(LIFT)+1)*127.5)).to_bytes(1, byteorder='big', signed=False)

                # print("Pitch: {:>6.3f} yaw: {:>6.3f} Roll: {:>6.3f} lift: {:>6.3f}".format(joystick.get_axis(PITCH),joystick.get_axis(YAW),joystick.get_axis(ROLL),joystick.get_axis(LIFT)))

                print(b'\xAA' + yaw_byte + pitch_byte + roll_byte + lift_byte)
                #print(int.from_bytes(lift_byte, byteorder='big', signed=False))
            time.sleep(0.1)


if __name__ == '__main__':
        mythread = ControlThread(name="Control Thread")
        mythread.start()
        while True:
            pass
            time.sleep(0.1)
