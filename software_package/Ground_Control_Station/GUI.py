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
        self.f_title = f_font_18.render(name,True,col_grey3)
        self.f_value = f_font_16.render('0',True,col_grey3)
        if self.b_horizontal == True:
        	self.r_back = pygame.Rect(left,top,160,20)
        	self.r_trimbar = pygame.Rect(0,top+17,1,3)
        else: self.r_back = pygame.Rect(left,top,20,200)

        #add to all guibars list    
        allguibars.append(self)

    def draw(self,screen):
        self.f_value = f_font_16.render(str(self.v_value),True,col_grey3)

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
                self.r_bar.width = round(((80)/127)*self.v_value)
                self.r_bar.height = 20
                if self.b_trim == True:
                    self.r_trimbar.left = self.left+80
                    self.r_trimbar.width = round(((80)/127)*self.v_trim)

            screen.blit(self.f_title,(self.left-2,self.top-24))
            screen.blit(self.f_value,(self.left+164,self.top-4))
        else:     #(vertical bars are always unsigned)
                self.r_bar.top = self.top + 200
                self.r_bar.left = self.left
                self.r_bar.width = 20
                self.r_bar.height = round(-((200)/255)*self.v_value)
                screen.blit(self.f_title,(self.left-2,self.top - 24))
                screen.blit(self.f_value,(self.left,self.top + 200))

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


class Button(object):
#text #left #top #height #widht #function r_button
    def __init__(self,name,top,left,width,height,fx,fy, color=pygame.Color(60,60,60)):
        self.name = name
        self.top = top
        self.left = left
        self.width = width
        self.height = height
        self.fx = fx #used for ofsetting the button text
        self.fy = fy #used for ofsetting the button text
        self.r_button = pygame.Rect(left,top, width, height)
        self.f_title = f_font_16.render(name,True,col_grey1)
        self.color = color
        allbuttons.append(self)

    def draw(self,screen):
        pygame.draw.rect(screen,self.color,self.r_button)
        screen.blit(self.f_title,(self.left+self.fy,self.top+self.fx))

    def set_enable(self, able):
        if able:
            self.color = pygame.Color(60,60,60)
        else:
            self.color = pygame.Color(180,180,180)

    def set_selected(self):
        self.color = pygame.Color(30,120,190)

    def checkclicked(self,pos):
    	return self.r_button.collidepoint(pos)

class Multiline(object):

    def __init__(self, top, left, width, height, lines, chars):
        self.top = top
        self.left = left
        self.width = width
        self.height = height
        self.lines = lines
        self.chars = chars
        self.data = [" "] * lines

        # TODO teken vierkant om top left width en height te checken!

    def draw(self, screen):
        background = pygame.Rect(self.left, self.top, self.width ,self.height)
        pygame.draw.rect(screen,pygame.Color(245,245,245),background)

        for i in range(self.lines):
            text = f_font_12.render(self.data[i],True,col_grey3)
            screen.blit(text,(self.left,self.top + i*15))

    def addLine(self, line):
        if len(line) > self.chars:
            line = line[:self.chars]

        self.data.pop(0)
        self.data.append(line)


allguibars = []
allbuttons = []

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
f_font_12 = pygame.font.Font('OpenSans.ttf',12)

#texts    
Text_Title = f_font_60.render('Ground Control Station',True,col_grey3)
Text_serial_header = f_font_20.render('Serial',True,col_grey3)
Text_trim_header = f_font_20.render('Trim settings',True,col_grey3)
Text_motor_header = f_font_20.render('Motors',True,col_grey3)
Text_params_header = f_font_20.render('Parameters',True,col_grey3)
Text_modeswitch_header = f_font_20.render('Modes',True,col_grey3)
Text_gyro_header = f_font_20.render('Gyro',True,col_grey3)

#titlebanner
r_topbar = pygame.Rect(0,0,740,20)
r_topbar2 = pygame.Rect(0,100,740,20)
r_topbar_shadow1 = pygame.Rect(0,19,740,1)
r_topbar2_shadow1 = pygame.Rect(0,100,740,1)
r_topbar2_shadow2 = pygame.Rect(0,119,740,1)

#background blocks
r_b_serial= pygame.Rect(40,140,420,220)
r_b_trim = pygame.Rect(40,380,220,280)
r_b_param = pygame.Rect(480,380,220,280)
r_b_motor = pygame.Rect(280,380,180,280)
r_b_bottom = pygame.Rect(40,680,660,100)
r_b_gyro = pygame.Rect(480,140,220,220)

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
    pygame.draw.rect(screen,col_grey1,r_b_serial)
    pygame.draw.rect(screen,col_grey1,r_b_trim)
    pygame.draw.rect(screen,col_grey1,r_b_gyro)
    pygame.draw.rect(screen,col_grey1,r_b_motor)
    pygame.draw.rect(screen,col_grey1,r_b_bottom)   
    pygame.draw.rect(screen,col_grey1,r_b_param)

    #draw gui bars
    for bar in range(len(allguibars)):
        allguibars[bar].draw(screen)
    #draw buttons
    for button in range(len(allbuttons)):
        allbuttons[button].draw(screen)

    #draw title text
    screen.blit(Text_Title,(60,16))

    screen.blit(Text_serial_header,(57,145))
    screen.blit(Text_trim_header,(57,385))
    screen.blit(Text_motor_header,(297,385))
    screen.blit(Text_params_header,(497,385))
    screen.blit(Text_modeswitch_header,(57,685))
    screen.blit(Text_gyro_header,(497,145))

    return screen