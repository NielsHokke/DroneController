#todo make default regmap

#!/usr/bin/env python
import sys
import threading
import time
import serial
import pygame
import crcmod
import Registermapping #homebrew registermpping file
import GUI #homebrew gui file
import parameterdefaults #homebrew parameter defaults
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
    return

#per byte register mapping
def send_parameter_message_4(register,byteA,byteB,byteC,byteD):
    if not isinstance(byteA,bytes):byteA = byteA.to_bytes(1, byteorder='big', signed=False)
    if not isinstance(byteB,bytes):byteB = byteB.to_bytes(1, byteorder='big', signed=False)
    if not isinstance(byteC,bytes):byteC = byteC.to_bytes(1, byteorder='big', signed=False)
    if not isinstance(byteD,bytes):byteD = byteD.to_bytes(1, byteorder='big', signed=False)
    A = byteA
    B = byteB
    C = byteC
    D = byteD
    payload = b'\x55'+register+A+B+C+D
    send_message(payload)
    return 

#per 2 bytes register mapping
def send_parameter_message_2(register,byteA,byteB):
    if not isinstance(byteA,bytes):byteA = byteA.to_bytes(2, byteorder='big', signed=False)
    if not isinstance(byteB,bytes):byteB = byteB.to_bytes(2, byteorder='big', signed=False)
    A = byteA
    B = byteB
    payload = b'\x55'+register+A+B
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
    global parametervalues

    #escape -> close program
    if pressed_key == pygame.K_ESCAPE: Running = False #TODO: check for safety

    #number -> switch mode
    elif pressed_key == pygame.K_0:Switch_Mode(Mode.MODE_SAFE)
    elif pressed_key == pygame.K_1:Switch_Mode(Mode.MODE_PANIC)
    elif pressed_key == pygame.K_2:Switch_Mode(Mode.MODE_MANUAL)
    elif pressed_key == pygame.K_3:Switch_Mode(Mode.MODE_CALIBRATION)
    elif pressed_key == pygame.K_4:Switch_Mode(Mode.MODE_YAW_CONTROL)
    elif pressed_key == pygame.K_5:Switch_Mode(Mode.MODE_FULL)
    elif pressed_key == pygame.K_6:Switch_Mode(Mode.MODE_RAW)
    elif pressed_key == pygame.K_7:Switch_Mode(Mode.MODE_HEIGHT)
    elif pressed_key == pygame.K_8:Switch_Mode(Mode.MODE_WIRELESS)

    #debug key
    elif pressed_key == pygame.K_SPACE:
        print(MODE)
        print("pitch: ",trimvalues.pitch)
        print("roll: ",trimvalues.roll)
        print("yaw: ",trimvalues.yaw)
        print("lift: ",trimvalues.lift)

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

    #TODO safegaurd
    #yawcontroll
    elif pressed_key == pygame.K_u:
        parametervalues.P = min(255,parametervalues.P+1)
        #parametervalues.P.to_bytes(1, byteorder='big', signed=False)
        send_parameter_message_4(Registermapping.REGMAP_PARAMETER_YAW,0,0,0,parametervalues.P.to_bytes(1, byteorder='big', signed=False))
    elif pressed_key == pygame.K_j:
        parametervalues.P = max(0,parametervalues.P-1)
        #parametervalues.P.to_bytes(1, byteorder='big', signed=False)
        send_parameter_message_4(Registermapping.REGMAP_PARAMETER_YAW,0,0,0,parametervalues.P.to_bytes(1, byteorder='big', signed=False))

    # elif MODE == MODE.MODE_FULL:
    #     #rollpitch control P1
    #     elif pressed_key == pygame.K_i
    #     elif pressed_key == pygame.K_k 
    #     #rollpitch control P2
    #     elif pressed_key == pygame.K_o
    #     elif pressed_key == pygame.K_l 

    return

#funtion to test the various mode switches
def Switch_Mode(new_mode):

    global MODE #use the global variable
    regmap = Registermapping.REGMAP_NEWMODE #destination adress from registermapping.py

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
                send_parameter_message(regmap, Mode.MODE_CALIBRATION)
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
    global trimvalues
    global allguibars

    #update trim values
    GUI.allguibars[0].settrim(trimvalues.roll)
    GUI.allguibars[1].settrim(trimvalues.pitch)
    GUI.allguibars[2].settrim(trimvalues.yaw)
    GUI.allguibars[3].settrim(trimvalues.lift)

    screen = GUI.draw_all(screen)
    return

def init_gui():
    #name,top,left,hor,uns,trim:
    GUI.Guibar("Roll",440,60,True,False,True)
    GUI.Guibar("Pitch",500,60,True,False,True)
    GUI.Guibar("Yaw",560,60,True,False,True)
    GUI.Guibar("Lift",620,60,True,True,True)

    GUI.Guibar("M1",440,300,False,True,False)
    GUI.Guibar("M2",440,340,False,True,False)
    GUI.Guibar("M3",440,380,False,True,False)
    GUI.Guibar("M4",440,420,False,True,False)
    
    #name,top,left,width,height,function
    GUI.Button("PANIC",720,60,60,40,8,9)       
    GUI.Button("CAL",720,140,60,40,8,14)
    GUI.Button("MAN",720,220,60,40,8,12)
    GUI.Button("REQUEST",600,500,80,40,8,4)
    GUI.Button("SEND",600,600,80,40,8,18)

    #P_YAW buttons
    GUI.Button("<<",300,600,20,20,-1,4)
    GUI.Button("<",300,620,20,20,-1,6)
    GUI.Button(">",300,640,20,20,-1,2)
    GUI.Button(">>",300,660,20,20,-1,2)
    #P1buttons
    GUI.Button("<<",330,600,20,20,-1,4)
    GUI.Button("<",330,620,20,20,-1,6)
    GUI.Button(">",330,640,20,20,-1,2)
    GUI.Button(">>",330,660,20,20,-1,2)
    #P2buttons
    GUI.Button("<<",360,600,20,20,-1,4)
    GUI.Button("<",360,620,20,20,-1,6)
    GUI.Button(">",360,640,20,20,-1,2)
    GUI.Button(">>",360,660,20,20,-1,2)

def handlebuttonfunction(button):
    global newparametervalues

    step = 4
    bigstep = 64

    if button == 0 : return#PANIC
    elif button == 1: return#CAL
    elif button == 2: return#MAN
    elif button == 3: return#REQUEST
    elif button == 4: setparams()#SEND
    #YAW
    elif button == 5: newparametervalues.PYaw = max(0,newparametervalues.PYaw-bigstep)#<<
    elif button == 6: newparametervalues.PYaw = max(0,newparametervalues.PYaw-step)#<
    elif button == 7: newparametervalues.PYaw = min(2**16,newparametervalues.PYaw+step)#>
    elif button == 8: newparametervalues.PYaw = min(2**16,newparametervalues.PYaw+bigstep)#>>
    #P1
    elif button == 9:  newparametervalues.P1 = max(0,newparametervalues.P1-bigstep)#<<
    elif button == 10: newparametervalues.P1 = max(0,newparametervalues.P1-step)#<
    elif button == 11: newparametervalues.P1 = min(2**16,newparametervalues.P1+step)#>
    elif button == 12: newparametervalues.P1 = min(2**16,newparametervalues.P1+bigstep)#>>
    #P2
    elif button == 13: newparametervalues.P2 = max(0,newparametervalues.P2-bigstep)#<<
    elif button == 14: newparametervalues.P2 = max(0,newparametervalues.P2-step)#<
    elif button == 15: newparametervalues.P2 = min(2**16,newparametervalues.P2+step)#>
    elif button == 16: newparametervalues.P2 = min(2**16,newparametervalues.P2+bigstep)#>>

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

def setparams():
    global parametervalues
    #bounderies
    b1 = newparametervalues.P_angle_max.to_bytes(1, byteorder='big', signed=False)
    b2 = newparametervalues.P_angle_min.to_bytes(1, byteorder='big', signed=False)
    b3 = newparametervalues.P_yaw_max.to_bytes(1, byteorder='big', signed=False)
    b4 = newparametervalues.P_yaw_min.to_bytes(1, byteorder='big', signed=False)
    send_parameter_message_4(Registermapping.REGMAP_BOUNDARIES,b1,b2,b3,b4)

    send_parameter_message_2(Registermapping.REGMAP_PARAMETER_YAW,0,newparametervalues.PYaw.to_bytes(2, byteorder='big', signed=False))
    send_parameter_message_2(Registermapping.REGMAP_PARAMETER_P1_P2,newparametervalues.P2.to_bytes(2, byteorder='big', signed=False),newparametervalues.P1.to_bytes(2, byteorder='big', signed=False))

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

class Parametervalues:
    PYaw = parameterdefaults.P_Yaw
    P1 = parameterdefaults.P_P1
    P2 = parameterdefaults.P_P2
    yaw_min = parameterdefaults.P_yaw_min
    yaw_max = parameterdefaults.P_yaw_max
    angle_min = parameterdefaults.P_angle_min
    angle_max = parameterdefaults.P_angle_max

#TODO commport as argument
# #if no comport is given as a argument default to ttyUSB0
# if len(sys.argv) == 1:
#     ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=1, writeTimeout=0, dsrdtr=True)
# else:
#     print(str(sys.argv))
#     argslist = str(sys.argv)
#     print(argslist)
#     print(argslist[1],type(argslist[1]))
#     #ser = serial.Serial(str(sys.argv)[1], 115200, timeout=1, writeTimeout=0, dsrdtr=True)

ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=1, writeTimeout=0, dsrdtr=True)

console = ConsoleThread(name="Console Thread")
console.start()

trimvalues = Trimvalues()
parametervalues = Parametervalues()
newparametervalues = Parametervalues()


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

    screen = pygame.display.set_mode([740, 800])
    init_gui() #init all the homebrew gui stuff
    pygame.display.set_caption("Ground Control Station")
    
    setparams()


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
        while GCS_joystick.get_axis(joystic_axis_lift) > 1:
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

            if event.type == pygame.MOUSEBUTTONDOWN:
                for button in range(len(GUI.allbuttons)):
                    if GUI.allbuttons[button].checkclicked(event.pos):
                        handlebuttonfunction(button)
                        print(newparametervalues.PYaw)

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
            send_control_message_flag = 100
      
        else:
            if has_joystick: send_control_message_flag = send_control_message_flag - 1
        
        #draw the gui and update it
        draw_gui()
        pygame.display.flip()

        time.sleep(0.005)

    print("Shutting down")
    if has_joystick: GCS_joystick.quit()
    console.stop()
    ser.flush()
    ser.close()
    pygame.display.quit()
    pygame.quit()
    exit()