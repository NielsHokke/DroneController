import pygame




col_white = pygame.Color(255,255,255)
col_grey1 = pygame.Color(240,240,240) #background stuff
col_grey2 = pygame.Color(190,190,190) #background bars and shadows
col_grey3 = pygame.Color(60,60,60) #Buttons and text color
col_blue = pygame.Color(30,120,190) #Buttons and text color
col_black = pygame.Color(0,0,0)

#fonts
pygame.font.init()
f_font_60 = pygame.font.Font('OpenSans.ttf',60)
f_font_18 = pygame.font.Font('OpenSans.ttf',60)
f_font_16 = pygame.font.Font('OpenSans.ttf',60)

#texts    
Text_Title = f_font_60.render('Ground Control Station',True,col_grey3)


#rects = left,top,widht,height
#titlebanner
r_topbar = pygame.Rect(0,0,1000,20)
r_topbar2 = pygame.Rect(0,100,1000,20)

r_topbar_shadow1 = pygame.Rect(0,19,1000,1)
r_topbar2_shadow1 = pygame.Rect(0,100,1000,1)
r_topbar2_shadow2 = pygame.Rect(0,119,1000,1)

#liftbar
r_liftbar = pygame.Rect(60,180,20,120)

#motorbars
r_m1bar = pygame.Rect(140,180,20,120)
r_m2bar = pygame.Rect(180,180,20,120)
r_m3bar = pygame.Rect(220,180,20,120)
r_m4bar = pygame.Rect(260,180,20,120)

#trimbar


#text


def drawbackground(screen):
    screen.fill(col_white)

    #topbar
    pygame.draw.rect(screen,col_grey1,r_topbar)
    pygame.draw.rect(screen,col_grey1,r_topbar2)
    pygame.draw.rect(screen,col_grey2,r_topbar_shadow1)
    pygame.draw.rect(screen,col_grey2,r_topbar2_shadow1)
    pygame.draw.rect(screen,col_grey2,r_topbar2_shadow2)

    pygame.draw.rect(screen,col_grey2,r_liftbar)

    pygame.draw.rect(screen,col_grey2,r_m1bar)
    pygame.draw.rect(screen,col_grey2,r_m2bar)
    pygame.draw.rect(screen,col_grey2,r_m3bar)
    pygame.draw.rect(screen,col_grey2,r_m4bar)

    #draw title text
    screen.blit(Text_Title,(190,19))


    return screen
