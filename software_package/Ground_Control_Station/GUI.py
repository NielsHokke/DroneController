import pygame

class Guibar(object):
    # b_horizontal 
    # b_unsigned 
    # b_trim
    # top
    # left
    # width 
    # height 
    # f_name 
    # r_back 
    # r_bar 
    # r_trimbar 
    # v_value
    # v_trim

    def __init__(self,name,top,left,hor,uns,trim):
        global allguibars
        self.r_bar = pygame.Rect(0,0,1,1)
        self.f_name = name
        self.b_trim = trim
        self.b_horizontal = hor
        self.b_unsigned = uns
        self.top = top
        self.left = left
        self.v_value = 0
        self.v_trim = 0
        if self.b_horizontal == True:
        	self.r_back = pygame.Rect(left,top,160,20)
        	self.r_trimbar = pygame.Rect(0,top+17,1,3)
        else: self.r_back = pygame.Rect(left,top,20,200)

        #add to all guibars list    
        allguibars.append(self)

    def drawbar(self,screen):
        #update the rects (only horizontal bars have trims)
        if self.b_horizontal == True:
            if self.b_unsigned == True:
                self.r_bar.top = self.top
                self.r_bar.left = self.left
                self.r_bar.width = round(((160)/255)*self.v_value)
                self.r_bar.height = 20
                if self.b_trim == True:
                    self.r_trimbar.left = self.left
                    self.r_trimbar.width = round(((160)/255)*self.v_trim)
            else:
                self.r_bar.top = self.top
                self.r_bar.left = self.left + 80
                self.r_bar.width = round(((80)/255)*self.v_value)
                self.r_bar.height = 20
                if self.b_trim == True:
                    self.r_trimbar.left = self.left+80
                    self.r_trimbar.width = round(((80)/255)*self.v_trim)
        else:     #(vertical bars are always unsigned)
                self.r_bar.top = self.top + 200
                self.r_bar.left = self.left
                self.r_bar.width = 20
                self.r_bar.height = round(-((200)/255)*self.v_value)

        #draw the rects
        pygame.draw.rect(screen,col_grey2,self.r_back)
        pygame.draw.rect(screen,col_blue,self.r_bar)
        if self.b_horizontal == True: pygame.draw.rect(screen,col_green,self.r_trimbar)

    def settrim(self,trim):
    	self.v_trim = trim
    	
    def setval(self,val):
    	self.v_value = val

    def __iter__(self):
        return self

allguibars = []

col_white = pygame.Color(255,255,255)
col_grey1 = pygame.Color(230,230,230) #background stuff
col_grey2 = pygame.Color(180,180,180) #background bars and shadows
col_grey3 = pygame.Color(60,60,60) 	#Buttons and text color
col_blue = pygame.Color(30,120,190) #blue color for the bars
#col_green = pygame.Color(50,140,50) #green color for the trim
col_green = pygame.Color(255,0,0) #green color for the trim
col_black = pygame.Color(0,0,0)

#fonts
pygame.font.init()
f_font_60 = pygame.font.Font('OpenSans.ttf',60)
f_font_20 = pygame.font.Font('OpenSans.ttf',20)
f_font_18 = pygame.font.Font('OpenSans.ttf',18)
f_font_16 = pygame.font.Font('OpenSans.ttf',16)

#texts    
Text_Title = f_font_60.render('Ground Control Station',True,col_grey3)

# Text_live_Lift = f_font_18.render('Lift',True,col_grey3)
# Text_live_M1 = f_font_18.render('M1',True,col_grey3)
# Text_live_M2 = f_font_18.render('M2',True,col_grey3)
# Text_live_M3 = f_font_18.render('M3',True,col_grey3)
# Text_live_M4 = f_font_18.render('M4',True,col_grey3)

# Text_trim_header = f_font_20.render('Trim settings',True,col_grey3)
# Text_trim_roll = f_font_18.render('Roll',True,col_grey3)
# Text_trim_yaw = f_font_18.render('Yaw',True,col_grey3)
# Text_trim_pitch = f_font_18.render('Pitch',True,col_grey3)
# Text_trim_lift = f_font_18.render('Lift',True,col_grey3)

#rects = left,top,widht,height
#titlebanner
r_topbar = pygame.Rect(0,0,740,20)
r_topbar2 = pygame.Rect(0,100,740,20)

r_topbar_shadow1 = pygame.Rect(0,19,740,1)
r_topbar2_shadow1 = pygame.Rect(0,100,740,1)
r_topbar2_shadow2 = pygame.Rect(0,119,740,1)

#background blocks
r_b_gyro = pygame.Rect(40,140,220,220)
r_b_trim = pygame.Rect(40,380,220,280)
r_b_param = pygame.Rect(480,260,220,400)
r_b_motor = pygame.Rect(280,380,180,280)
r_b_bottom = pygame.Rect(40,680,660,100)
r_b_status = pygame.Rect(280,260,180,100)

def draw_all(screen):
	#background
    screen.fill(col_white)
    #topbar
    pygame.draw.rect(screen,col_grey1,r_topbar)
    pygame.draw.rect(screen,col_grey1,r_topbar2)
    pygame.draw.rect(screen,col_grey2,r_topbar_shadow1)
    pygame.draw.rect(screen,col_grey2,r_topbar2_shadow1)
    pygame.draw.rect(screen,col_grey2,r_topbar2_shadow2)

    #background blocks
    pygame.draw.rect(screen,col_grey1,r_b_gyro)
    pygame.draw.rect(screen,col_grey1,r_b_trim)
    pygame.draw.rect(screen,col_grey1,r_b_param)
    pygame.draw.rect(screen,col_grey1,r_b_motor)
    pygame.draw.rect(screen,col_grey1,r_b_bottom)   
    pygame.draw.rect(screen,col_grey1,r_b_status)

    #draw gui bars
    for bar in range(len(allguibars)):
        allguibars[bar].drawbar(screen)

    #draw title text
    screen.blit(Text_Title,(20,19))
    # #live view textr_back
    # screen.blit(Text_live_Lift,(57,150))
    # screen.blit(Text_live_M1,(139,150))
    # screen.blit(Text_live_M2,(179,150))
    # screen.blit(Text_live_M3,(219,150))
    # screen.blit(Text_live_M4,(259,150))
    # #trim
    # screen.blit(Text_trim_header,(40,380))
    # screen.blit(Text_trim_roll,(60,415))
    # screen.blit(Text_trim_yaw,(60,475))
    # screen.blit(Text_trim_pitch,(60,535))
    # screen.blit(Text_trim_lift,(60,595))

    return screen