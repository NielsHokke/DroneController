#!/usr/bin/env python
import threading
import time
import serial
import pygame
import crcmod



ser = serial.Serial('/dev/ttyACM2', 115200, timeout=1, writeTimeout=0, dsrdtr=True)

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
                ser.write(self.ctrl_message + b'\n')             
                # ser.write("Pitch: {:>4} yaw: {:>4} Roll: {:>4} lift: {:>4} \n".format(int.from_bytes(self.pitch_byte, byteorder='little', signed='True'),int.from_bytes(self.yaw_byte, byteorder='little', signed='True'),int.from_bytes(self.roll_byte, byteorder='little', signed='True'),int.from_bytes(self.lift_byte, byteorder='little')).encode())
                self.send_update = 100
            else:
                # ser.write(b'\x00')
                self.send_update = self.send_update - 1

            time.sleep(0.005)


if __name__ == '__main__':
    ser.close()
    ser.open()
    ser.flush()
    controller = ControlThread(name="Control Thread")
    controller.start()

    while True:
        print(ser.readline().decode("utf-8", "backslashreplace"), end='', flush=True)
        # time.sleep(100)
