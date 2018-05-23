#!/usr/bin/env python
import threading
import time
import serial
import pygame
import crcmod


class ConsoleThread(threading.Thread):
    Running = True

    def run(self):
        while self.Running:
            print(ser.readline().decode("utf-8", "backslashreplace"), end='', flush=True)

    def stop(self):
        self.Running = False


ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=1, writeTimeout=0, dsrdtr=True)

console = ConsoleThread(name="Console Thread")
console.start()


Running = True
in_use = False

PITCH = 1
YAW = 2
ROLL = 0
LIFT = 3

if __name__ == '__main__':
    send_update = 0
    pitch_byte = b'\x00'
    yaw_byte = b'\x00'
    roll_byte = b'\x00'
    lift_byte = b'\x00'
    ctrl_message = b'\xaa\x00\x00\x00\x00\x48'
    raw_pitch = 0
    raw_yaw = 0
    raw_roll = 0
    raw_lift = 0
    has_joystick = True

    # polynomial = 0xD8 this function requires a 1 at the start so sure
    crc8 = crcmod.mkCrcFun(0x1D8, initCrc=0, xorOut=0x00, rev=False)

    pygame.init()

    screen = pygame.display.set_mode([500, 700])

    pygame.display.set_caption("Ground Control Station")


    if pygame.joystick.get_count() < 0:
        print("No joystick detected. Keyboard only mode")
        has_joystick = False

    if has_joystick:
        pygame.joystick.init()
        joystick = pygame.joystick.Joystick(0)
        joystick.init()

    # --- Main loop ---
    while Running:
        # Line below should becommented out as soon as we're using the GUI
        # pygame.display.flip()
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                Running = False

            if event.type == pygame.KEYDOWN:
                if event.key == pygame.K_w:
                    send_update = 0
                elif event.key == pygame.K_s:
                    send_update = 0

            if has_joystick:
                # Possible joystick actions: JOYAXISMOTION JOYBALLMOTION JOYBUTTONDOWN JOYBUTTONUP JOYHATMOTION
                if event.type == pygame.JOYBUTTONDOWN:
                    print("Joystick button pressed.")
                if event.type == pygame.JOYBUTTONUP:
                    print("Joystick button released.")
                if event.type == pygame.JOYAXISMOTION:
                    raw_pitch = round(joystick.get_axis(PITCH) * 127.5)
                    raw_yaw = round(joystick.get_axis(YAW) * 127.5)
                    raw_roll = round(joystick.get_axis(ROLL) * 127.5)
                    raw_lift = (255 - round((joystick.get_axis(LIFT) + 1) * 127.5))
                    send_update = 0

        if send_update == 0:

            #print("{} {} {} {}".format(raw_pitch, raw_yaw, raw_lift, raw_roll))
            pitch_byte = raw_pitch.to_bytes(1, byteorder='big', signed=True)
            yaw_byte = raw_yaw.to_bytes(1, byteorder='big', signed=True)
            roll_byte = raw_roll.to_bytes(1, byteorder='big', signed=True)
            lift_byte = raw_lift.to_bytes(1, byteorder='big', signed=False)
            payload = b'\xAA' + yaw_byte + pitch_byte + roll_byte + lift_byte
            ctrl_message = payload + bytearray([crc8(payload)])

            ser.write(ctrl_message + b'\n')
            ser.flush()
            send_update = 100
        else:
            # ser.write(b'\x00')
            send_update = send_update - 1

        time.sleep(0.005)

    print("Shutting down")
    joystick.quit()
    console.stop()
    ser.flush()
    ser.close()
    pygame.quit()
