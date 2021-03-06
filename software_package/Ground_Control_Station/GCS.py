#todo make default regmap

#!/usr/bin/env python
import sys
import copy
import threading
import string
import time
import serial
import pygame
import crcmod
import math
import os
import Registermapping #homebrew registermpping file
import GUI #homebrew gui file
import parameterdefaults #homebrew parameter defaults
from enum import IntEnum
from time import gmtime, strftime
from time import time as milis

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
    MODE_SYSTEM_RESET = 255

class Trimdirection(IntEnum):
    UP = 1
    DOWN = 0

#functions to handle the controll message sending
def send_control_message():    
    global controlvalues
    pitch_byte = controlvalues.pitch.to_bytes(1, byteorder='big', signed=True)
    yaw_byte   = controlvalues.yaw.to_bytes(1, byteorder='big', signed=True)
    roll_byte  = controlvalues.roll.to_bytes(1, byteorder='big', signed=True)
    lift_byte  = controlvalues.lift.to_bytes(1, byteorder='big', signed=False)
    payload    = b'\xAA' + yaw_byte + pitch_byte + roll_byte + lift_byte
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

    # print('myvalue for A is: ', A, '\n')
    # print('myvalue for B is: ', B, '\n')

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
    global logfile
    message = payload + bytearray([crc8(payload)])
    ser.write(message + b'\n')
    ser.flush()
    logfile.write("MESSAGE: {}, {} ,".format(milis(), message) + "\n")
    return

#funtion to handel all the keyboard input
def handle_keypress(pressed_key):
    global Running #use global variable to shut the thing down
    global trimvalues
    global parametervalues
    global newparametervalues

    #escape -> close program
    if pressed_key == pygame.K_ESCAPE: 
        if MODE == Mode.MODE_SAFE:
            Running = False #TODO: check for safety
        else:
            Switch_Mode(Mode.MODE_PANIC)    

    #number -> switch mode
    elif pressed_key == pygame.K_0:Switch_Mode(Mode.MODE_SAFE)
    elif pressed_key == pygame.K_1:Switch_Mode(Mode.MODE_PANIC)
    elif pressed_key == pygame.K_2:Switch_Mode(Mode.MODE_MANUAL)
    elif pressed_key == pygame.K_3:Switch_Mode(Mode.MODE_CALIBRATION)
    elif pressed_key == pygame.K_4:Switch_Mode(Mode.MODE_YAW_CONTROL)
    elif pressed_key == pygame.K_5:Switch_Mode(Mode.MODE_FULL)
    elif pressed_key == pygame.K_6:Switch_Mode(Mode.MODE_RAW)
    # elif pressed_key == pygame.K_7:Switch_Mode(Mode.MODE_HEIGHT)
    # elif pressed_key == pygame.K_8:Switch_Mode(Mode.MODE_WIRELESS)

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
        newparametervalues.PYaw = min(2**16-1,parametervalues.PYaw+1)
        send_parameter_message_2(Registermapping.REGMAP_PARAMETER_YAW,0,newparametervalues.PYaw.to_bytes(2, byteorder='big', signed=False))
        parametervalues.PYaw = newparametervalues.PYaw

    elif pressed_key == pygame.K_j:
        newparametervalues.PYaw = max(0,parametervalues.PYaw-1)
        send_parameter_message_2(Registermapping.REGMAP_PARAMETER_YAW,0,newparametervalues.PYaw.to_bytes(2, byteorder='big', signed=False))
        parametervalues.PYaw = newparametervalues.PYaw

    #rollpitch control P1
    elif pressed_key == pygame.K_i:
        newparametervalues.P1 = min(2**16-1,parametervalues.P1+1)
        send_parameter_message_2(Registermapping.REGMAP_PARAMETER_P1_P2,newparametervalues.P1.to_bytes(2, byteorder='big', signed=False),parametervalues.P2.to_bytes(2, byteorder='big', signed=False))
        parametervalues.P1 = newparametervalues.P1

    elif pressed_key == pygame.K_k:
        newparametervalues.P1 = max(0,parametervalues.P1-1)
        send_parameter_message_2(Registermapping.REGMAP_PARAMETER_P1_P2,newparametervalues.P1.to_bytes(2, byteorder='big', signed=False),parametervalues.P2.to_bytes(2, byteorder='big', signed=False))
        parametervalues.P1 = newparametervalues.P1

    #rollpitch control P2
    elif pressed_key == pygame.K_o:
        newparametervalues.P2 = min(2**16-1,parametervalues.P2+1)
        send_parameter_message_2(Registermapping.REGMAP_PARAMETER_P1_P2,parametervalues.P1.to_bytes(2, byteorder='big', signed=False),newparametervalues.P2.to_bytes(2, byteorder='big', signed=False))
        parametervalues.P2 = newparametervalues.P2

    elif pressed_key == pygame.K_l:
        newparametervalues.P2 = max(0,parametervalues.P2-1)
        send_parameter_message_2(Registermapping.REGMAP_PARAMETER_P1_P2,parametervalues.P1.to_bytes(2, byteorder='big', signed=False),newparametervalues.P2.to_bytes(2, byteorder='big', signed=False))
        parametervalues.P2 = newparametervalues.P2
    return

#funtion to test the various mode switches
def Switch_Mode(new_mode, force=False, local=False):
    global controlvalues
    global MODE #use the global variable

    global safe_bt
    global panic_bt 
    global man_bt
    global cal_bt
    global yaw_bt
    global full_bt
    global raw_bt

    regmap = Registermapping.REGMAP_NEWMODE #destination adress from registermapping.py

    global ready
    global ready_counter

    ready = False
    ready_counter = 50

    # if the requested mode is the panic mode
    if new_mode == Mode.MODE_PANIC:
        #cant go to panic from normal states
        if (MODE == Mode.MODE_SAFE or MODE == Mode.MODE_CALIBRATION) and not force:
            print("Can't go to panic mode from this state")
        else:
            MODE = Mode.MODE_PANIC
            if not local: send_parameter_message(regmap, Mode.MODE_PANIC)
            safe_bt.set_enable(True)
            panic_bt.set_selected()
            man_bt.set_enable(False)
            cal_bt.set_enable(False)
            yaw_bt.set_enable(False)
            full_bt.set_enable(False)
            raw_bt.set_enable(False)
            print("mode switched to: ",new_mode)

    elif controlvalues.lift == 0 or force:
        if new_mode == Mode.MODE_SAFE:
            MODE = Mode.MODE_SAFE
            if not local: send_parameter_message(regmap, Mode.MODE_SAFE)
            safe_bt.set_selected()
            panic_bt.set_enable(False)
            man_bt.set_enable(True)
            cal_bt.set_enable(True)
            yaw_bt.set_enable(True)
            full_bt.set_enable(True)
            raw_bt.set_enable(True)
            print("mode switched to: ",new_mode)

        elif MODE == Mode.MODE_SAFE or force:
            #calibration
            if new_mode == Mode.MODE_CALIBRATION:
                MODE = Mode.MODE_CALIBRATION
                if not local: send_parameter_message(regmap, Mode.MODE_CALIBRATION)
                safe_bt.set_enable(True)
                panic_bt.set_enable(False)
                man_bt.set_enable(False)
                cal_bt.set_selected()
                yaw_bt.set_enable(False)
                full_bt.set_enable(False)
                raw_bt.set_enable(False)
            #manual
            elif new_mode == Mode.MODE_MANUAL:
                MODE = Mode.MODE_MANUAL
                if not local: send_parameter_message(regmap, Mode.MODE_MANUAL)
                safe_bt.set_enable(True)
                panic_bt.set_enable(True)
                man_bt.set_selected()
                cal_bt.set_enable(False)
                yaw_bt.set_enable(False)
                full_bt.set_enable(False)
                raw_bt.set_enable(False)
            #yaw
            elif new_mode == Mode.MODE_YAW_CONTROL:
                MODE = Mode.MODE_YAW_CONTROL
                if not local: send_parameter_message(regmap, Mode.MODE_YAW_CONTROL)
                safe_bt.set_enable(True)
                panic_bt.set_enable(True)
                man_bt.set_enable(False)
                cal_bt.set_enable(False)
                yaw_bt.set_selected()
                full_bt.set_enable(False)
                raw_bt.set_enable(False)
            #full
            elif new_mode == Mode.MODE_FULL:
                MODE = Mode.MODE_FULL
                if not local: send_parameter_message(regmap, Mode.MODE_FULL)
                safe_bt.set_enable(True)
                panic_bt.set_enable(True)
                man_bt.set_enable(False)
                cal_bt.set_enable(False)
                yaw_bt.set_enable(False)
                full_bt.set_selected()
                raw_bt.set_enable(False)
            #Raw
            elif new_mode == Mode.MODE_RAW:
                MODE = Mode.MODE_RAW
                if not local: send_parameter_message(regmap, Mode.MODE_RAW)
                safe_bt.set_enable(True)
                panic_bt.set_enable(True)
                man_bt.set_enable(False)
                cal_bt.set_enable(False)
                yaw_bt.set_enable(False)
                full_bt.set_enable(False)
                raw_bt.set_selected()

            #height
            # elif new_mode == Mode.MODE_HEIGHT:
            #     MODE = Mode.MODE_HEIGHT
            #     send_parameter_message(regmap, Mode.MODE_HEIGHT)
            #     safe_bt.set_enable(True)
            #     panic_bt.set_enable(True)
            #     man_bt.set_enable(False)
            #     cal_bt.set_enable(False)
            #     yaw_bt.set_enable(False)
            #     full_bt.set_enable(False)
            #     raw_bt.set_selected()
            #wireless
            # elif new_mode == Mode.MODE_WIRELESS:
            #     MODE = Mode.MODE_WIRELESS
            #     send_parameter_message(regmap, Mode.MODE_WIRELESS)
            #     safe_bt.set_enable(True)
            #     panic_bt.set_enable(True)
            #     man_bt.set_enable(False)
            #     cal_bt.set_enable(False)
            #     yaw_bt.set_enable(False)
            #     full_bt.set_enable(False)
            #     raw_bt.set_enable(False)

            print("mode switched to:", MODE)
        else:
            print("invalid Mode switch requested, no mode switched")
    else:
        print("make sure lift is zero before changing modes")

    return

def draw_gui():
    global screen
    global trimvalues
    global allguibars
    global controlvalues
    global motorvalues
    global gyrovalues
    global batvalue

    global has_joystick

    global multiline

    #update trim values
    GUI.allguibars[0].settrim(trimvalues.roll)
    GUI.allguibars[1].settrim(trimvalues.pitch)
    GUI.allguibars[2].settrim(trimvalues.yaw)
    GUI.allguibars[3].settrim(trimvalues.lift)
    #update setpoints
    GUI.allguibars[0].setval(controlvalues.roll)
    GUI.allguibars[1].setval(controlvalues.pitch)
    GUI.allguibars[2].setval(controlvalues.yaw)
    GUI.allguibars[3].setval(controlvalues.lift)
    #update bar values for motor
    GUI.allguibars[4].setval(motorvalues.M1)
    GUI.allguibars[5].setval(motorvalues.M2)
    GUI.allguibars[6].setval(motorvalues.M3)
    GUI.allguibars[7].setval(motorvalues.M4)

    GUI.allguibars[8].setval(gyrovalues.pitch)
    GUI.allguibars[9].setval(gyrovalues.yaw)
    GUI.allguibars[10].setval(gyrovalues.roll)

    if batvalue < 1080:
        color = pygame.Color(255,0,0)
    elif batvalue < 1180:
        color = pygame.Color(255,128,0)
    else:
        color = pygame.Color(0,204,0)

    GUI.allguibars[11].setval(batvalue, color)

    #text in middel
    text_pitch = GUI.f_font_16.render("Pitch: " + str(gyrovalues.pitch),True,GUI.col_grey3)
    text_yaw = GUI.f_font_16.render("Yaw: " + str(gyrovalues.yaw),True,GUI.col_grey3)
    text_roll = GUI.f_font_16.render("Roll: " + str(gyrovalues.roll),True,GUI.col_grey3)

    #update parameter display P1 P2 P3
    Text_PY   = GUI.f_font_16.render(str(parametervalues.PYaw),True,GUI.col_grey3)
    Text_P1   = GUI.f_font_16.render(str(parametervalues.P1),True,GUI.col_grey3)
    Text_P2   = GUI.f_font_16.render(str(parametervalues.P2),True,GUI.col_grey3)
    Text_PY_n   = GUI.f_font_16.render(str(newparametervalues.PYaw),True,GUI.col_grey3)
    Text_P1_n   = GUI.f_font_16.render(str(newparametervalues.P1),True,GUI.col_grey3)
    Text_P2_n   = GUI.f_font_16.render(str(newparametervalues.P2),True,GUI.col_grey3)
    Text_PY_h   = GUI.f_font_16.render("PY",True,GUI.col_grey3)
    Text_P1_h   = GUI.f_font_16.render("P1",True,GUI.col_grey3)
    Text_P2_h   = GUI.f_font_16.render("P2",True,GUI.col_grey3)

    #yaw max and angle max text
    Text_ymax   = GUI.f_font_16.render(str(parametervalues.yaw_max),True,GUI.col_grey3)
    Text_ymin   = GUI.f_font_16.render(str(parametervalues.yaw_min),True,GUI.col_grey3)
    Text_amax   = GUI.f_font_16.render(str(parametervalues.angle_max),True,GUI.col_grey3)
    Text_amin   = GUI.f_font_16.render(str(parametervalues.angle_min),True,GUI.col_grey3)
    Text_ymax_n = GUI.f_font_16.render(str(newparametervalues.yaw_max),True,GUI.col_grey3)
    Text_ymin_n = GUI.f_font_16.render(str(newparametervalues.yaw_min),True,GUI.col_grey3)
    Text_amax_n = GUI.f_font_16.render(str(newparametervalues.angle_max),True,GUI.col_grey3)
    Text_amin_n = GUI.f_font_16.render(str(newparametervalues.angle_min),True,GUI.col_grey3)
    Text_ymax_h = GUI.f_font_16.render("Y+",True,GUI.col_grey3)
    Text_ymin_h = GUI.f_font_16.render("Y-",True,GUI.col_grey3)
    Text_amax_h = GUI.f_font_16.render("A+",True,GUI.col_grey3)
    Text_amin_h = GUI.f_font_16.render("A-",True,GUI.col_grey3)

    #draw most of the gui
    screen = GUI.draw_all(screen)

    multiline.draw(screen)

    #update the parameter text
    P_H = 415

    screen.blit(Text_PY_h,(490,P_H))
    screen.blit(Text_P1_h,(490,P_H+30))
    screen.blit(Text_P2_h,(490,P_H+60))
    screen.blit(Text_PY,(520,P_H))
    screen.blit(Text_P1,(520,P_H+30))
    screen.blit(Text_P2,(520,P_H+60))
    screen.blit(Text_PY_n,(560,P_H))
    screen.blit(Text_P1_n,(560,P_H+30))
    screen.blit(Text_P2_n,(560,P_H+60))
    #angle and yaw
    screen.blit(Text_ymax_h,(490,P_H+90))
    screen.blit(Text_ymin_h,(490,P_H+120))
    screen.blit(Text_amax_h,(490,P_H+150))
    screen.blit(Text_amin_h,(490,P_H+180))
    screen.blit(Text_ymax,(520,P_H+90))
    screen.blit(Text_ymin,(520,P_H+120))
    screen.blit(Text_amax,(520,P_H+150))
    screen.blit(Text_amin,(520,P_H+180))
    screen.blit(Text_ymax_n,(560,P_H+90))
    screen.blit(Text_ymin_n,(560,P_H+120))
    screen.blit(Text_amax_n,(560,P_H+150))
    screen.blit(Text_amin_n,(560,P_H+180))
    return

def init_gui():
    global safe_bt
    global panic_bt 
    global man_bt
    global cal_bt
    global yaw_bt
    global full_bt
    global raw_bt

    #name,top,left,hor,uns,trim:
    GUI.Guibar("Roll",440,60,True,False,True)
    GUI.Guibar("Pitch",500,60,True,False,True)
    GUI.Guibar("Yaw",560,60,True,False,True)
    GUI.Guibar("Lift",620,60,True,True,True)

    GUI.Guibar("M1",440,300,False,True,False,1000)
    GUI.Guibar("M2",440,340,False,True,False,1000)
    GUI.Guibar("M3",440,380,False,True,False,1000)
    GUI.Guibar("M4",440,420,False,True,False,1000)

    GUI.Guibar("Pitch",200,500,True,False,False, 12000, 133)
    GUI.Guibar("Yaw",260,500,True,False,False, 2**16, 182)
    GUI.Guibar("Roll",320,500,True,False,False, 12000, 133)

    GUI.Guibar("",175,410,False,True,False,numscale=100, bat=True, size=160, minr=1000,maxr=1300)
    
    #name,top,left,width,height,function
    safe_bt = GUI.Button("0: SAFE",720,60,80,40,8,12)
    safe_bt.set_selected()
    panic_bt = GUI.Button("1: PANIC",720,150,80,40,8,8)
    panic_bt.set_enable(False)
    man_bt = GUI.Button("2: MAN",720,240,80,40,8,12)
    cal_bt = GUI.Button("3: CAL",720,330,80,40,8,16)
    yaw_bt = GUI.Button("4: YAW",720,420,80,40,8,15)
    full_bt = GUI.Button("5: FULL",720,510,80,40,8,14)
    raw_bt = GUI.Button("6: RAW",720,600,80,40,8,12)

    GUI.Button("SEND",625,600,80,25,2,18)

    P_H = 415

    #P_YAW buttons
    GUI.Button("<<",P_H,600,20,20,-1,4)
    GUI.Button("<",P_H,620,20,20,-1,6)
    GUI.Button(">",P_H,640,20,20,-1,2)
    GUI.Button(">>",P_H,660,20,20,-1,2)
    #P1buttons
    GUI.Button("<<",P_H+30,600,20,20,-1,4)
    GUI.Button("<",P_H+30,620,20,20,-1,6)
    GUI.Button(">",P_H+30,640,20,20,-1,2)
    GUI.Button(">>",P_H+30,660,20,20,-1,2)
    #P2buttons
    GUI.Button("<<",P_H+60,600,20,20,-1,4)
    GUI.Button("<",P_H+60,620,20,20,-1,6)
    GUI.Button(">",P_H+60,640,20,20,-1,2)
    GUI.Button(">>",P_H+60,660,20,20,-1,2)
    #Y+ buttons
    GUI.Button("<<",P_H+90,600,20,20,-1,4)
    GUI.Button("<",P_H+90,620,20,20,-1,6)
    GUI.Button(">",P_H+90,640,20,20,-1,2)
    GUI.Button(">>",P_H+90,660,20,20,-1,2)
    #Y- buttons
    GUI.Button("<<",P_H+120,600,20,20,-1,4)
    GUI.Button("<",P_H+120,620,20,20,-1,6)
    GUI.Button(">",P_H+120,640,20,20,-1,2)
    GUI.Button(">>",P_H+120,660,20,20,-1,2)
    #Y+ buttons
    GUI.Button("<<",P_H+150,600,20,20,-1,4)
    GUI.Button("<",P_H+150,620,20,20,-1,6)
    GUI.Button(">",P_H+150,640,20,20,-1,2)
    GUI.Button(">>",P_H+150,660,20,20,-1,2)
    #Y- buttons
    GUI.Button("<<",P_H+180,600,20,20,-1,4)
    GUI.Button("<",P_H+180,620,20,20,-1,6)
    GUI.Button(">",P_H+180,640,20,20,-1,2)
    GUI.Button(">>",P_H+180,660,20,20,-1,2)

def handlebuttonfunction(button):
    global newparametervalues
    global parametervalues

    step = 1
    bigstep = 32

    if button == 0 : Switch_Mode(Mode.MODE_SAFE)#SAFE
    elif button == 1: Switch_Mode(Mode.MODE_PANIC)#PANIC
    elif button == 2: Switch_Mode(Mode.MODE_MANUAL)#MAN
    elif button == 3: Switch_Mode(Mode.MODE_CALIBRATION)#CAlIBRATION
    elif button == 4: Switch_Mode(Mode.MODE_YAW_CONTROL)#YAW
    elif button == 5: Switch_Mode(Mode.MODE_FULL)#FULL
    elif button == 6: Switch_Mode(Mode.MODE_RAW)#HEIGHT
    elif button == 7: #SEND
        print("send")
        setparams()
        parametervalues = copy.copy(newparametervalues)

    #YAW
    elif button == 8: newparametervalues.PYaw = max(0,newparametervalues.PYaw-bigstep)#<<
    elif button == 9: newparametervalues.PYaw = max(0,newparametervalues.PYaw-step)#<
    elif button == 10: newparametervalues.PYaw = min(2**16-1,newparametervalues.PYaw+step)#>
    elif button == 11: newparametervalues.PYaw = min(2**16-1,newparametervalues.PYaw+bigstep)#>>
    
    #P1
    elif button == 12:  newparametervalues.P1 = max(0,newparametervalues.P1-bigstep)#<<
    elif button == 13: newparametervalues.P1 = max(0,newparametervalues.P1-step)#<
    elif button == 14: newparametervalues.P1 = min(2**16-1,newparametervalues.P1+step)#>
    elif button == 15: newparametervalues.P1 = min(2**16-1,newparametervalues.P1+bigstep)#>>
    
    #P2
    elif button == 16: newparametervalues.P2 = max(0,newparametervalues.P2-bigstep)#<<
    elif button == 17: newparametervalues.P2 = max(0,newparametervalues.P2-step)#<
    elif button == 18: newparametervalues.P2 = min(2**16-1,newparametervalues.P2+step)#>
    elif button == 19: newparametervalues.P2 = min(2**16-1,newparametervalues.P2+bigstep)#>>
    
    #Y+
    elif button == 20: newparametervalues.yaw_max = max(0,newparametervalues.yaw_max-5)#<<
    elif button == 21: newparametervalues.yaw_max = max(0,newparametervalues.yaw_max-1)#<
    elif button == 22: newparametervalues.yaw_max = min(2**8-1,newparametervalues.yaw_max+1)#>
    elif button == 23: newparametervalues.yaw_max = min(2**8-1,newparametervalues.yaw_max+5)#>>
    
    #Y-
    elif button == 24: newparametervalues.yaw_min = max(0,newparametervalues.yaw_min-5)#<<
    elif button == 25: newparametervalues.yaw_min = max(0,newparametervalues.yaw_min-1)#<
    elif button == 26: newparametervalues.yaw_min = min(2**8-1,newparametervalues.yaw_min+1)#>
    elif button == 27: newparametervalues.yaw_min = min(2**8-1,newparametervalues.yaw_min+5)#>>
    
    #A+
    elif button == 28: newparametervalues.angle_max = max(0,newparametervalues.angle_max-5)#<<
    elif button == 29: newparametervalues.angle_max = max(0,newparametervalues.angle_max-1)#<
    elif button == 30: newparametervalues.angle_max = min(2**8-1,newparametervalues.angle_max+1)#>
    elif button == 31: newparametervalues.angle_max = min(2**8-1,newparametervalues.angle_max+5)#>>
    
    #A-
    elif button == 32: newparametervalues.angle_min = max(0,newparametervalues.angle_min-5)#<<
    elif button == 33: newparametervalues.angle_min = max(0,newparametervalues.angle_min-1)#<
    elif button == 34: newparametervalues.angle_min = min(2**8-1,newparametervalues.angle_min+1)#>
    elif button == 35: newparametervalues.angle_min = min(2**8-1,newparametervalues.angle_min+5)#>>

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

#send all parameters
def setparams():
    global parametervalues
    global newparametervalues
    global motorvalues

    #bounderies
    b1 = newparametervalues.angle_max.to_bytes(1, byteorder='big', signed=False)
    b2 = newparametervalues.angle_min.to_bytes(1, byteorder='big', signed=False)
    b3 = newparametervalues.yaw_max.to_bytes(1, byteorder='big', signed=False)
    b4 = newparametervalues.yaw_min.to_bytes(1, byteorder='big', signed=False)

    send_parameter_message_4(Registermapping.REGMAP_BOUNDARIES,b1,b2,b3,b4)
    send_parameter_message_2(Registermapping.REGMAP_PARAMETER_YAW,0,newparametervalues.PYaw.to_bytes(2, byteorder='big', signed=False))
    send_parameter_message_2(Registermapping.REGMAP_PARAMETER_P1_P2,newparametervalues.P1.to_bytes(2, byteorder='big', signed=False),newparametervalues.P2.to_bytes(2, byteorder='big', signed=False))

class ConsoleThread(threading.Thread):
    global logfile
    Running = True
    printable = set(string.printable)
    def run(self):
        global MODE
        global ready
        global motorvalues
        global gyrovalues
        global multiline
        global batvalue

        data = bytearray()
        while self.Running:
            byte = ser.read()
            inte = int.from_bytes(byte, byteorder='big')
            data.append(int.from_bytes(byte, byteorder='big'))
            if inte == 10 and len(data) > 1:

                dataString = data[:-2].decode("utf-8", errors='ignore')
                if '@@@' in dataString and not 'zzz' in dataString:
                    continue

                print(data[:-2])
                fout = data[-2]

                
                if '@@@' in dataString or 'zzz' in dataString:
                    if '@@@' in dataString and 'zzz' in dataString:
                        index = 0
                        found_A = False
                        for byt in data:
                            if not found_A:
                                if data[index] != ord('@'):
                                    index += 1
                                else:
                                    found_A = True
                            else:
                                if data[index] == ord('@'):
                                    index += 1
                                else:
                                    break

                        if ready and len(data) >= index+16:

                            mode = data[index]
                            m1 = int(data[index+1] * 256 + data[index+2])
                            m2 = int(data[index+3] * 256 + data[index+4])
                            m3 = int(data[index+5] * 256 + data[index+6])
                            m4 = int(data[index+7] * 256 + data[index+8])

                            pitch = int(data[index+9] * 256 + data[index+10])
                            yaw = int(data[index+11] * 256 + data[index+12])
                            roll = int(data[index+13] * 256 + data[index+14])

                            bat = int(data[index+15] * 256 + data[index+16])

                            drone_time = int(data[index+17] * 2**24 + data[index+18]* 2**16 +data[index+19] * 2**8 + data[index+20])
                            # print(drone_time)
                            batvalue = bat

                            if pitch > 2**15:
                                pitch -= 2**16

                            if yaw > 2**15:
                                yaw -= 2**16

                            if roll > 2**15:
                                roll -= 2**16

                            if mode != MODE:
                                print("adjusted by drone", mode)
                                Switch_Mode(mode, True, True)

                            motorvalues.M1 = m1
                            motorvalues.M2 = m2
                            motorvalues.M3 = m3
                            motorvalues.M4 = m4

                            gyrovalues.pitch = pitch
                            gyrovalues.yaw = yaw
                            gyrovalues.roll = roll

                            # TODO print downlink to log
                            logfile.write("DRONE: {},{},{},{},{},{},{},{},{},{}, {}\n".format(milis(), mode,m1,m2,m3,m4,pitch,roll,yaw,bat, drone_time))

                            # print("found information string")
                            # print(data)
                            # print("Mode", mode)
                            # print("M1", m1)
                            # print("M2", m2)
                            # print("M3", m3)
                            # print("M4", m4)
                            # print("Pitch", pitch)
                            # print("Yaw", yaw)
                            # print("Roll", roll, "\n")
                    else:
                        pass
                        # half a message recieved
                else:
                    if '@' not in dataString:
                        tijd = strftime("%H:%M:%S", gmtime())
                        multiline.addLine(tijd+" "+ "".join(i for i in dataString if ord(i)<128 and ord(i)>31))
                        print (dataString)
                        logfile.write("DEBUG: {},".format(milis()) + dataString + "\n")


                data = bytearray()
                data.append(fout)

    def stop(self):
        self.Running = False

class Controlvalues:
    pitch = 0
    yaw = 0
    roll = 0
    lift = 0

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

class Motorvalues:
    M1 = 0
    M2 = 0
    M3 = 0
    M4 = 0

class Gyrovalues:
    pitch = 0
    yaw = 0
    roll = 0

#get comport as argument
if len(sys.argv) < 2:
    print("No serial port specified, picking '/dev/ttyUSB0/")
    serial_port = '/dev/ttyUSB0'
else:
    serial_port = sys.argv[1]

# sudo apt-get setserial before it would work
os.system("setserial " + serial_port + " low_latency")
ser = serial.Serial(serial_port, 115200, timeout=1, writeTimeout=0, dsrdtr=True)
logfile = open('log.cvs', 'w+')
console = ConsoleThread(name="Console Thread")
console.start()

#global values
trimvalues = Trimvalues()
parametervalues = Parametervalues()
newparametervalues = Parametervalues()
controlvalues = Controlvalues()
motorvalues = Motorvalues()
gyrovalues = Gyrovalues()
batvalue = 0

multiline = GUI.Multiline(175, 60, 280, 180, 12, 54)

MODE = Mode.MODE_SAFE
Running = True
in_use = False

joystic_axis_pitch = 1
joystic_axis_yaw = 2
joystic_axis_roll = 0
joystic_axis_lift = 3

safe_bt = None
panic_bt = None
man_bt = None
cal_bt = None
yaw_bt = None
full_bt = None
raw_bt = None

ready = False

ready_counter = 10

if __name__ == '__main__':
    send_control_message_flag = 100 #timer and flag for sending controll messages
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

    ready = True

    # --- Main loop ---
    while Running:
        #pygame.event.wait() #wait until event happens, doesnt work 
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                print("someone pressed \n")
                if MODE == Mode.MODE_SAFE:
                    Running = False #TODO: check for safety
                else:
                    Switch_Mode(Mode.MODE_PANIC)    

            if event.type == pygame.KEYDOWN:
                handle_keypress(event.key)

            if event.type == pygame.MOUSEBUTTONDOWN:
                for button in range(len(GUI.allbuttons)):
                    if GUI.allbuttons[button].checkclicked(event.pos):
                        handlebuttonfunction(button)

            if has_joystick:
                # Possible joystick actions: JOYAXISMOTION JOYBALLMOTION JOYBUTTONDOWN JOYBUTTONUP JOYHATMOTION
                if event.type == pygame.JOYBUTTONDOWN:
                    if (GCS_joystick.get_button(0)):
                        print("Joystick button pressed. go to panic mode")
                        Switch_Mode(Mode.MODE_PANIC)

                # if event.type == pygame.JOYBUTTONUP:
                #     print("Joystick button released.")

                if event.type == pygame.JOYAXISMOTION:
                    #add trim values and bounds
                    controlvalues.pitch = min(127,max(-127,round(GCS_joystick.get_axis(joystic_axis_pitch) * 127.5)+trimvalues.pitch))
                    controlvalues.yaw   = min(127,max(-127,round(GCS_joystick.get_axis(joystic_axis_yaw) * 127.5)+trimvalues.yaw))
                    controlvalues.roll  = min(127,max(-127,round(GCS_joystick.get_axis(joystic_axis_roll) * 127.5)+trimvalues.roll))
                    controlvalues.lift  = min(255,max(0,(255 - (round((GCS_joystick.get_axis(joystic_axis_lift) + 1) * 127.5))+trimvalues.lift)))
                    if controlvalues.lift < 45:
                        controlvalues.lift = controlvalues.lift * 4
                    else:
                        value = controlvalues.lift
                        templift = round(180 + 75 * math.sqrt((value-45)/210))
                        controlvalues.lift = max(0,min(255, templift))

                    send_control_message_flag = 0
            else:
                controlvalues.pitch = min(127,max(-127,trimvalues.pitch))
                controlvalues.yaw   = min(127,max(-127,trimvalues.yaw))
                controlvalues.roll  = min(127,max(-127,trimvalues.roll))
                controlvalues.lift  = min(255,max(0,trimvalues.lift))
                if controlvalues.lift < 45:
                        controlvalues.lift = controlvalues.lift * 4
                else:
                    value = controlvalues.lift
                    templift = round(180 + 75 * math.sqrt((value-45)/210))
                    controlvalues.lift = max(0,min(255, templift))


        if send_control_message_flag == 0:
            send_control_message()
            send_control_message_flag = 80

        send_control_message_flag += -1

        if not ready:
            # print("not ready")
            ready_counter -= 1
            if ready_counter == 0:
                ready = True

        #draw the gui and update it
        draw_gui()
        pygame.display.flip()
        time.sleep(0.005)

    print("Shutting down")
    
    if has_joystick: GCS_joystick.quit()
    console.stop()
    ser.flush()
    time.sleep(0.01)
    ser.close()
    pygame.display.quit()
    pygame.quit()
    exit()
