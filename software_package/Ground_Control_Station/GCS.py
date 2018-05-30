#!/usr/bin/env python
import threading
import time
import serial
import pygame
import crcmod
import Registermapping #homebrew registermpping file
import GUI #homebrew gui file
from enum import IntEnum

#enumarate the different modes
class Mode(IntEnum):
    MODE_SAFE = 0
    MODE_PANIC = 1
    MODE_MANUAL = 2
    MODE_CALIBRATION = 3
    MODE_YAW_CONTROL = 4
    MODE_FULL = 5
    MODE_RAW = 6
    MODE_HEIGHT = 7
    MODE_WIRELESS = 8

class Trimdirection(IntEnum):
    UP = 1
    DOWN = 0

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
    A = byteA.to_bytes(1, byteorder='big', signed=True)
    B = byteB.to_bytes(1, byteorder='big', signed=True)
    C = ByteC.to_bytes(1, byteorder='big', signed=True)
    D = ByteD.to_bytes(1, byteorder='big', signed=True)
    payload = b'\x55'+register+A+B+C+D
    send_message(payload)
    return 

#per 4 byte register 
def send_parameter_message(register,data):
    payload = b'\x55' + register + data.to_bytes(4, byteorder='big', signed=True)
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
    global Running #use global variable to shut the thing down
    global trimvalues

    #escape -> close program
    if pressed_key == pygame.K_ESCAPE: Running = False #TODO: check for safety

    #number -> switch mode
    elif pressed_key == pygame.K_0:Switch_Mode(Mode.MODE_SAFE)
    elif pressed_key == pygame.K_1:Switch_Mode(Mode.MODE_PANIC)
    elif pressed_key == pygame.K_2:Switch_Mode(Mode.MODE_MANUAL)
    elif pressed_key == pygame.K_3:Switch_Mode(Mode.MODE_CALIBRATION)
    elif pressed_key == pygame.K_4:Switch_Mode(Mode.MODE_YAW_CONTROL)
    elif pressed_key == pygame.K_5:Switch_Mode(Mode.MODE_FULL)
    #debug key
    elif pressed_key == pygame.K_SPACE:
        print(MODE)
        print("pitch: ",trimvalues.pitch)
        print("roll: ",trimvalues.roll)
        print("yaw: ",trimvalues.yaw)
        print("lift: ",trimvalues.lift)


    #elif pressed_key == pygame.K_6:Switch_Mode(Mode.MODE_RAW)
    #elif pressed_key == pygame.K_7:Switch_Mode(Mode.MODE_HEIGHT)
    #elif pressed_key == pygame.K_8:Switch_Mode(Mode.MODE_WIRELESS)

    # TODO conditional safty thing
    # lift 
    elif pressed_key == pygame.K_a:change_trimvalue(Trimdirection.UP,joystic_axis_lift) # up
    elif pressed_key == pygame.K_z:change_trimvalue(Trimdirection.DOWN,joystic_axis_lift) # down
    # roll
    elif pressed_key == pygame.K_LEFT:change_trimvalue(Trimdirection.UP,joystic_axis_roll) #down
    elif pressed_key == pygame.K_RIGHT:change_trimvalue(Trimdirection.DOWN,joystic_axis_roll) #up
    #pitch
    elif pressed_key == pygame.K_UP:change_trimvalue(Trimdirection.DOWN,joystic_axis_pitch) #nose more down
    elif pressed_key == pygame.K_DOWN:change_trimvalue(Trimdirection.UP,joystic_axis_pitch) #nose more up
    #yaw
    elif pressed_key == pygame.K_q:change_trimvalue(Trimdirection.DOWN,joystic_axis_yaw) #rotate more counterclockwise
    elif pressed_key == pygame.K_w:change_trimvalue(Trimdirection.UP,joystic_axis_yaw) #rotate more clockwise

    """
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

    global MODE #use the global variable
    regmap =  Registermapping.REGMAP_NEWMODE #destination adress from registermapping.py

    # if the requested mode is the panic mode
    if new_mode == Mode.MODE_PANIC:
        #cant go to panic from normal states
        if MODE == Mode.MODE_SAFE or MODE == Mode.MODE_CALIBRATION:
            print("Can't go to panic mode from this state")
        else:
            MODE = Mode.MODE_PANIC
            send_parameter_message(regmap, Mode.MODE_PANIC)
            print("mode switched to: ",new_mode)

    elif True: #TODO: no motors are spinning & lift is zero on joystick
        if new_mode == Mode.MODE_SAFE:
            MODE = Mode.MODE_SAFE
            send_parameter_message(regmap, Mode.MODE_SAFE)
            print("mode switched to: ",new_mode)

        elif MODE == Mode.MODE_SAFE:
            #calibration
            if new_mode == Mode.MODE_CALIBRATION:
                MODE = Mode.MODE_CALIBRATION
                send_parameter_message(regmap, Mode.MODE_MANUAL)
            #manual
            elif new_mode == Mode.MODE_MANUAL:
                MODE = Mode.MODE_MANUAL
                send_parameter_message(regmap, Mode.MODE_MANUAL)
            #yaw
            elif new_mode == Mode.MODE_YAW_CONTROL:
                MODE = Mode.MODE_YAW_CONTROL
                send_parameter_message(regmap, Mode.MODE_YAW_CONTROL)
            #full
            elif new_mode == Mode.MODE_FULL:
                MODE = Mode.MODE_FULL
                send_parameter_message(regmap, Mode.MODE_FULL)
            #Raw
            elif new_mode == Mode.MODE_RAW:
                MODE = Mode.MODE_RAW
                send_parameter_message(regmap, Mode.MODE_RAW)
            #height
            elif new_mode == Mode.MODE_HEIGHT:
                MODE = Mode.MODE_HEIGHT
                send_parameter_message(regmap, Mode.MODE_HEIGHT)
            #wireless
            elif new_mode == Mode.MODE_WIRELESS:
                MODE = Mode.MODE_WIRELESS
                send_parameter_message(regmap, Mode.MODE_WIRELESS)
            print("mode switched to: ",new_mode)
        else:
    	    print("invalid Mode switch requested, no mode switched")
    return

def draw_gui():
    global screen
    tw = GUI.trimbar_width
    #calculate blue part for trimbars
    GUI.r_trim_roll_a.width = round(((tw/2)/255)*trimvalues.roll)
    GUI.r_trim_pitch_a.width = round(((tw/2)/255)*trimvalues.pitch)
    GUI.r_trim_yaw_a.width = round(((tw/2)/255)*trimvalues.yaw)
    GUI.r_trim_lift_a.width = round(((tw)/255)*trimvalues.lift)

    #trimbar text

    screen = GUI.drawbackground(screen)
    return

def change_trimvalue(trimidirection,trimvar):
    global trimvalues
    #input checking
    if not (trimidirection == Trimdirection.UP or trimidirection == Trimdirection.DOWN):
        print("wrong value given for trim direction")
        return

    #adapt right trim value    
    if trimvar == joystic_axis_lift: 
        if trimidirection == Trimdirection.UP: trimvalues.lift = min(255, trimvalues.lift+1)#trim up
        else: trimvalues.lift = max(0, trimvalues.lift-1) #trim down
        return
    elif trimvar == joystic_axis_pitch: 
        if trimidirection == Trimdirection.UP: trimvalues.pitch = min(255, trimvalues.pitch+1)#trim up
        else: trimvalues.pitch = max(-255, trimvalues.pitch-1) #trim down
        return
    elif trimvar == joystic_axis_roll: 
        if trimidirection == Trimdirection.UP: trimvalues.roll = min(255, trimvalues.roll+1)#trim up
        else: trimvalues.roll = max(-255, trimvalues.roll-1) #trim down
        return
    elif trimvar == joystic_axis_yaw: 
        if trimidirection == Trimdirection.UP: trimvalues.yaw = min(255, trimvalues.yaw+1)#trim up
        else: trimvalues.yaw = max(-255, trimvalues.yaw-1) #trim down
        return
    return

class ConsoleThread(threading.Thread):
    Running = True
    def run(self):
        while self.Running:
            print(ser.readline().decode("utf-8", "backslashreplace"), end='', flush=True)
    def stop(self):
        self.Running = False

class Trimvalues:
    pitch = 0
    yaw = 0
    roll = 0
    lift = 0

ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=1, writeTimeout=0, dsrdtr=True)

console = ConsoleThread(name="Console Thread")
console.start()

trimvalues = Trimvalues()

MODE = Mode.MODE_SAFE
Running = True
in_use = False

joystic_axis_pitch = 1
joystic_axis_yaw = 2
joystic_axis_roll = 0
joystic_axis_lift = 3

if __name__ == '__main__':
    send_control_message_flag = 100 #timer and flag for sending controll messages
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

    screen = pygame.display.set_mode([1000, 800])

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
        print("checking lift")
        while GCS_joystick.get_axis(LIFT) > 1:
        	time.sleep(0.1)
        print("lift is correctly set")

    # --- Main loop ---
    while Running:
        #pygame.event.wait() #wait until event happens, doesnt work 

        # Line below should becommented out as soon as we're using the GUI
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
                    raw_pitch = round(GCS_joystick.get_axis(joystic_axis_pitch) * 127.5)
                    raw_yaw = round(GCS_joystick.get_axis(joystic_axis_yaw) * 127.5)
                    raw_roll = round(GCS_joystick.get_axis(joystic_axis_roll) * 127.5)
                    #TODO fix negativity
                    raw_lift = (255 - round((GCS_joystick.get_axis(joystic_axis_lift) + 1) * 127.5))
                    send_control_message_flag = 0

        if send_control_message_flag == 0:
        	send_control_message()      
        else:
            if has_joystick: send_control_message_flag = send_control_message_flag - 1
        
        #draw the gui and update it
        draw_gui()
        pygame.display.flip()

        time.sleep(0.005)

    print("Shutting down")
    if has_joystick: joystick.quit()
    console.stop()
    ser.flush()
    ser.close()
    pygame.display.quit()
    pygame.quit()
    exit()
