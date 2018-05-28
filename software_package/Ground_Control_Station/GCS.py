#!/usr/bin/env python
import threading
import time
import serial
import pygame
import crcmod
from enum import Enum

#enumarate the different modes
class Mode(Enum):
    MODE_SAFE = 0
    MODE_PANIC = 1
    MODE_MANUAL = 2
    MODE_CALIBRATION = 3
    MODE_YAW_CONTROL = 4
    MODE_FULL = 5
    MODE_RAW = 6
    MODE_HEIGHT = 7
    MODE_WIRELESS = 8

#some functions to handle the communication 
def send_control_message():
    #print("{} {} {} {}".format(raw_pitch, raw_yaw, raw_lift, raw_roll))
    pitch_byte = raw_pitch.to_bytes(1, byteorder='big', signed=True)
    yaw_byte = raw_yaw.to_bytes(1, byteorder='big', signed=True)
    roll_byte = raw_roll.to_bytes(1, byteorder='big', signed=True)
    lift_byte = raw_lift.to_bytes(1, byteorder='big', signed=False)
    payload = b'\xAA' + yaw_byte + pitch_byte + roll_byte + lift_byte
    send_message(payload)
    send_control_message_flag = 100
    return

#per byte register mapping
def send_parameter_message(register,byteA,byteB,ByteC,ByteD):
    payload = b'\x55' + 
    byteA.to_bytes(1, byteorder='big', signed=True) + 
    byteB.to_bytes(1, byteorder='big', signed=True) + 
    ByteC.to_bytes(1, byteorder='big', signed=True) + 
    ByteD.to_bytes(1, byteorder='big', signed=True)
    send_message(payload)
    return 

#per 4 byte register 
def send_parameter_message(register,data):
    payload = b'\x55' + data.to_bytes(4, byteorder='big', signed=True)
    send_message(payload)
    return

#only used for finnishing the send operation   
def send_message(payload):
    message = payload + bytearray([crc8(payload)])
    ser.write(message + b'\n')
    ser.flush()
   return


#funtion to handel all the keyboard input
def handle_keypress(pressed_key):
    #escape -> close program
    if pressed_key == pygame.K_ESCAPE: Running = False #not functionall yet

    #number -> switch mode
    elif pressed_key == pygame.K_0:Switch_Mode(Mode.MODE_SAFE)
    elif pressed_key == pygame.K_1:Switch_Mode(Mode.MODE_PANIC)
    elif pressed_key == pygame.K_2:Switch_Mode(Mode.MODE_MANUAL)
    elif pressed_key == pygame.K_3:Switch_Mode(Mode.MODE_CALIBRATION)
    elif pressed_key == pygame.K_4:Switch_Mode(Mode.MODE_YAW_CONTROL)
    elif pressed_key == pygame.K_5:Switch_Mode(Mode.MODE_FULL)
    """
    elif pressed_key == pygame.K_6:Switch_Mode(Mode.MODE_RAW)
    elif pressed_key == pygame.K_7:Switch_Mode(Mode.MODE_HEIGHT)
    elif pressed_key == pygame.K_8:Switch_Mode(Mode.MODE_WIRELESS)

    elif MODE == MODE.MODE_MANUAL:
        # lift 
        if pressed_key == pygame.K_a #up
        elif pressed_key == pygame.K_z #down
        # roll
        elif pressed_key == pygame.K_LEFT #down
        elif pressed_key == pygame.K_RIGHT #up
        #pitch
        elif pressed_key == pygame.K_UP #up
        elif pressed_key == pygame.K_DOWN #down
        #yaw
        elif pressed_key == pygame.K_q
        elif pressed_key == pygame.K_w

    elif MODE == MODE.MODE_YAW_CONTROL:
        #yawcontroll
        if pressed_key == pygame.K_u
        elif pressed_key == pygame.K_j 

    elif MODE == MODE.MODE_FULL:
        #rollpitch control P1
        elif pressed_key == pygame.K_i
        elif pressed_key == pygame.K_k 
        #rollpitch control P2
        elif pressed_key == pygame.K_o
        elif pressed_key == pygame.K_l 
    """
    return

#funtion to test the various mode switches
def Switch_Mode(new_mode):
    # if the requested mode is the panic mode
    if new_mode == MODE_PANIC:
        #cant go to panic from normal states
        if MODE == MODE_SAFE or MODE == MODE_CALIBRATION:
            print("Can't go to panic mode from this state")
        else:
            MODE == MODE_PANIC
            send_parameter_message(REGMAP_NEWMODE, MODE_PANIC)

    #elif no motors are spinning
    if new_mode == MODE_SAFE:
        MODE = MODE_SAF
    elif new_mode == MODE_CALIBRATION and MODE == MODE_SAFE:
       MODE = 

    return

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

MODE = Mode.MODE_SAFE

Running = True
in_use = False

PITCH = 1
YAW = 2
ROLL = 0
LIFT = 3

if __name__ == '__main__':
    send_control_message_flag = 100 #timer and flag for sending controll messages
    send_parameter_message =0 #flag for sending parameters
    pitch_byte = b'\x00'
    yaw_byte = b'\x00'
    roll_byte = b'\x00'
    lift_byte = b'\x00'
    #ctrl_message = b'\xaa\x00\x00\x00\x00\x48'
    raw_pitch = 0
    raw_yaw = 0
    raw_roll = 0
    raw_lift = 0
    has_joystick = False

    # polynomial = 0xD8 this function requires a 1 at the start so sure
    crc8 = crcmod.mkCrcFun(0x1D8, initCrc=0, xorOut=0x00, rev=False)

    pygame.init()

    screen = pygame.display.set_mode([500, 700])

    pygame.display.set_caption("Ground Control Station")

    if pygame.joystick.get_count() > 0:
        print("Joystick detected")
        has_joystick = True
    else:
        print("No joystick detected. Keyboard only mode")

    if has_joystick:
        #pygame.joystick.init()
        GCS_joystick = pygame.joystick.Joystick(0)
        GCS_joystick.init()

    # --- Main loop ---
    while Running:
        # Line below should becommented out as soon as we're using the GUI
        # pygame.display.flip()
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                Running = False

            if event.type == pygame.KEYDOWN:
            	handle_keypress(event.key)

            if has_joystick:
                # Possible joystick actions: JOYAXISMOTION JOYBALLMOTION JOYBUTTONDOWN JOYBUTTONUP JOYHATMOTION
                if event.type == pygame.JOYBUTTONDOWN:
                    print("Joystick button pressed.")
                if event.type == pygame.JOYBUTTONUP:
                    print("Joystick button released.")
                if event.type == pygame.JOYAXISMOTION:
                    raw_pitch = round(GCS_joystick.get_axis(PITCH) * 127.5)
                    raw_yaw = round(GCS_joystick.get_axis(YAW) * 127.5)
                    raw_roll = round(GCS_joystick.get_axis(ROLL) * 127.5)
                    raw_lift = (255 - round((GCS_joystick.get_axis(LIFT) + 1) * 127.5))
                    send_control_message_flag = 0

        if send_control_message_flag == 0:
        	send_control_message()      
        else:
            # ser.write(b'\x00')
            if has_joystick: send_control_message_flag = send_control_message_flag - 1

        time.sleep(0.005)

    print("Shutting down")
    if has_joystick: joystick.quit()
    console.stop()
    ser.flush()
    ser.close()
    pygame.display.quit()
    pygame.quit()
    exit()