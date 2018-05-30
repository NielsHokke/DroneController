import pygame

trimbar_width = 160

col_white = pygame.Color(255,255,255)
col_grey1 = pygame.Color(240,240,240) #background stuff
col_grey2 = pygame.Color(190,190,190) #background bars and shadows
col_grey3 = pygame.Color(60,60,60) #Buttons and text color
col_blue = pygame.Color(30,120,190) #Buttons and text color
col_black = pygame.Color(0,0,0)

#fonts
pygame.font.init()
f_font_60 = pygame.font.Font('OpenSans.ttf',60)
f_font_20 = pygame.font.Font('OpenSans.ttf',20)
f_font_18 = pygame.font.Font('OpenSans.ttf',18)
f_font_16 = pygame.font.Font('OpenSans.ttf',16)

#texts    
Text_Title = f_font_60.render('Ground Control Station',True,col_grey3)

Text_live_Lift = f_font_18.render('Lift',True,col_grey3)
Text_live_M1 = f_font_18.render('M1',True,col_grey3)
Text_live_M2 = f_font_18.render('M2',True,col_grey3)
Text_live_M3 = f_font_18.render('M3',True,col_grey3)
Text_live_M4 = f_font_18.render('M4',True,col_grey3)

Text_trim_header = f_font_20.render('Trim settings',True,col_grey3)
Text_trim_roll = f_font_18.render('Roll',True,col_grey3)
Text_trim_yaw = f_font_18.render('Yaw',True,col_grey3)
Text_trim_pitch = f_font_18.render('Pitch',True,col_grey3)
Text_trim_lift = f_font_18.render('Lift',True,col_grey3)

#rects = left,top,widht,height
#titlebanner
r_topbar = pygame.Rect(0,0,1000,20)
r_topbar2 = pygame.Rect(0,100,1000,20)

r_topbar_shadow1 = pygame.Rect(0,19,1000,1)
r_topbar2_shadow1 = pygame.Rect(0,100,1000,1)
r_topbar2_shadow2 = pygame.Rect(0,119,1000,1)

#liftbar
r_liftbar = pygame.Rect(60,180,20,160)

#motorbars
r_m1bar = pygame.Rect(140,180,20,160)
r_m2bar = pygame.Rect(180,180,20,160)
r_m3bar = pygame.Rect(220,180,20,160)
r_m4bar = pygame.Rect(260,180,20,160)

#trimbar (a is the dynamic thing)
r_trim_roll = pygame.Rect(60,440,trimbar_width,20)
r_trim_pitch = pygame.Rect(60,500,trimbar_width,20)
r_trim_yaw = pygame.Rect(60,560,trimbar_width,20)
r_trim_lift = pygame.Rect(60,620,trimbar_width,20)
r_trim_roll_a = pygame.Rect(140,440,0,20)
r_trim_pitch_a = pygame.Rect(140,500,0,20)
r_trim_yaw_a = pygame.Rect(140,560,0,20)
r_trim_lift_a = pygame.Rect(60,620,0,20)



def drawbackground(screen):
	#background
    screen.fill(col_white)
    #topbar
    pygame.draw.rect(screen,col_grey1,r_topbar)
    pygame.draw.rect(screen,col_grey1,r_topbar2)
    pygame.draw.rect(screen,col_grey2,r_topbar_shadow1)
    pygame.draw.rect(screen,col_grey2,r_topbar2_shadow1)
    pygame.draw.rect(screen,col_grey2,r_topbar2_shadow2)
    #live lift
    pygame.draw.rect(screen,col_grey2,r_liftbar)
    #live motors
    pygame.draw.rect(screen,col_grey2,r_m1bar)
    pygame.draw.rect(screen,col_grey2,r_m2bar)
    pygame.draw.rect(screen,col_grey2,r_m3bar)
    pygame.draw.rect(screen,col_grey2,r_m4bar)
    #draw trim bars
    pygame.draw.rect(screen,col_grey2,r_trim_roll)
    pygame.draw.rect(screen,col_grey2,r_trim_pitch)
    pygame.draw.rect(screen,col_grey2,r_trim_yaw)
    pygame.draw.rect(screen,col_grey2,r_trim_lift)

    pygame.draw.rect(screen,col_blue,r_trim_roll_a)
    pygame.draw.rect(screen,col_blue,r_trim_pitch_a)
    pygame.draw.rect(screen,col_blue,r_trim_yaw_a)
    pygame.draw.rect(screen,col_blue,r_trim_lift_a)


    #draw title text
    screen.blit(Text_Title,(190,19))
    #live view text
    screen.blit(Text_live_Lift,(57,150))
    screen.blit(Text_live_M1,(139,150))
    screen.blit(Text_live_M2,(179,150))
    screen.blit(Text_live_M3,(219,150))
    screen.blit(Text_live_M4,(259,150))
    #trim
    screen.blit(Text_trim_header,(40,380))
    screen.blit(Text_trim_roll,(60,415))
    screen.blit(Text_trim_yaw,(60,475))
    screen.blit(Text_trim_pitch,(60,535))
    screen.blit(Text_trim_lift,(60,595))





    return screen
