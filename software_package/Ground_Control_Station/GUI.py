import pygame


col_white = pygame.Color(255,255,255)
col_grey1 = pygame.Color(240,240,240) #background stuff
col_grey2 = pygame.Color(190,190,190) #background bars and shadows
col_grey2 = pygame.Color(60,60,60) #Buttons and text color
col_blue = pygame.Color(30,120,190) #Buttons and text color
col_black = pygame.Color(0,0,0)

r_rect1 = pygame.Rect(30,30,30,30)
r_rect2 = pygame.Rect(90,90,30,30)

#left,top,widht,height

#titlebanner
r_topbar = pygame.Rect(0,0,1000,20)
r_topbar2 = pygame.Rect(0,100,1000,20)

#liftbar
r_liftbar = pygame.Rect(60,180,20,120)

#motorbars
r_m1bar = pygame.Rect(140,180,20,120)
r_m2bar = pygame.Rect(180,180,20,120)
r_m3bar = pygame.Rect(220,180,20,120)
r_m4bar = pygame.Rect(260,180,20,120)

#trimbar

def drawbackground(screen):
    pygame.draw.rect(screen,col_grey1,r_topbar)
    pygame.draw.rect(screen,col_grey1,r_topbar2)

    pygame.draw.rect(screen,col_grey1,r_liftbar)

    pygame.draw.rect(screen,col_grey1,r_m1bar)
    pygame.draw.rect(screen,col_grey1,r_m2bar)
    pygame.draw.rect(screen,col_grey1,r_m3bar)
    pygame.draw.rect(screen,col_grey1,r_m4bar)

    return screen