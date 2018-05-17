#!/usr/bin/env python
import threading
import time
import serial
import pygame
import crcmod



# ser = serial.Serial('COM4', 115200, writeTimeout=0, dsrdtr=True)

Running = True
in_use = False

PITCH = 1
YAW = 3
ROLL = 0
LIFT = 2


class ControlThread(threading.Thread):
    send_update = 0
    pitch_byte = b'\x00'
    yaw_byte = b'\x00'
    roll_byte = b'\x00'
    lift_byte = b'\x00'
    ctrl_message = b'\xaa\x00\x00\x00\x00\x48'

    def run(self):
        # polynomial = 0xD8 this function requires a 1 at the start so sure
        crc8 = crcmod.mkCrcFun(0x1D8, initCrc=0, xorOut=0x00, rev=False)


        pygame.init()

        has_joystick = True
        if pygame.joystick.get_count() < 1:
            print("No joystick detected. Keyboard only mode")
            has_joystick = False
        if has_joystick:
            pygame.joystick.init()
            joystick = pygame.joystick.Joystick(0)
            joystick.init()
            start = time.time()

        start = time.time()
        while Running:

            if has_joystick:
                for event in pygame.event.get():  # User did something
                    # Possible joystick actions: JOYAXISMOTION JOYBALLMOTION JOYBUTTONDOWN JOYBUTTONUP JOYHATMOTION
                    if event.type == pygame.JOYBUTTONDOWN:
                        print("Joystick button pressed.")
                    if event.type == pygame.JOYBUTTONUP:
                        print("Joystick button released.")
                    if event.type == pygame.JOYAXISMOTION:
                        self.pitch_byte = (round(joystick.get_axis(PITCH)*127.5)).to_bytes(1, byteorder='big', signed=True)
                        self.yaw_byte = (round(joystick.get_axis(YAW)*127.5)).to_bytes(1, byteorder='big', signed=True)
                        self.roll_byte = (round(joystick.get_axis(ROLL)*127.5)).to_bytes(1, byteorder='big', signed=True)
                        self.lift_byte = (255 - round((joystick.get_axis(LIFT)+1)*127.5)).to_bytes(1, byteorder='big', signed=False)
                        payload = b'\xAA' + self.yaw_byte + self.pitch_byte + self.roll_byte + self.lift_byte
                        self.ctrl_message = payload + bytearray([crc8(payload)])
                        self.send_update = 0

            if self.send_update == 0:
                pass
                # ser.write(b'\n' + self.ctrl_message + b'\n')
                # ser.write(b'hoisdf gsdfg sdfgsdfgs dfg \n')
                # print(ctrl_message)
                end = time.time()
                print("{0:.12f} ".format(end - start))
                start = time.time()
                self.send_update = 100
            else:
                # ser.write(b'.')
                self.send_update = self.send_update - 1

            # print("Pitch: {:>6.3f} yaw: {:>6.3f} Roll: {:>6.3f} lift: {:>6.3f}".format(joystick.get_axis(PITCH),joystick.get_axis(YAW),joystick.get_axis(ROLL),joystick.get_axis(LIFT)))
            time.sleep(0.01)




class ConsoleThread(threading.Thread):
    def run(self):
        while ser.inWaiting() > 0:
            # print(ser.read(1).decode("utf-8", "backslashreplace"), end='', flush=True)

            # print("BYTEEES")
            time.sleep(0.01)


if __name__ == '__main__':
    # ser.close()
    # ser.open()
    # ser.read(10000)
    controller = ControlThread(name="Control Thread")
    controller.start()
    # console = ConsoleThread(name="Console thread")
    # console.start()

    while True:
        time.sleep(100)
