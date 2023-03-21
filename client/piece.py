import pygame
SQUARE_SIZE=90




def loadImages():
    IMAGES={}
    IMAGES['w']=pygame.transform.scale(pygame.image.load("assets/wp.png"), (SQUARE_SIZE, SQUARE_SIZE))
    IMAGES['b']=pygame.transform.scale(pygame.image.load("assets/bp.png"), (SQUARE_SIZE, SQUARE_SIZE))
    IMAGES['c']=pygame.transform.scale(pygame.image.load("assets/crown.png"), (SQUARE_SIZE//2, SQUARE_SIZE//6))

    return IMAGES

IMAGES=loadImages()


class Piece:
    def __init__(self,color,row,col):
        self.row=row
        self.col=col
        self.color=color
        self.king=False
        self.x=0
        self.y=0
        self.valid_moves=[]

    def calc_pos(self,player_color):
        if(player_color=='w'):
            self.x=SQUARE_SIZE*self.col+40  
            self.y=SQUARE_SIZE*self.row+ 75 
        else:
            self.x=SQUARE_SIZE*(7-self.col)+40
            self.y=SQUARE_SIZE*(7-self.row)+75

    def make_king(self):
        self.king=True

    def check_if_king(self):
        return self.king

    def draw(self,win,player_color):
        self.calc_pos(player_color)
        win.blit(IMAGES[self.color], pygame.Rect(self.x,self.y, SQUARE_SIZE, SQUARE_SIZE))
        
        if self.king:
            win.blit(IMAGES['c'], (self.x+SQUARE_SIZE//4+2, self.y+SQUARE_SIZE//4+5))

    def move(self, row, col,player_color):
        self.row = row
        self.col = col
        self.calc_pos(player_color)

    def get_valid_moves(self):
        return self.valid_moves

    def set_valid_moves(self,moves):
        self.valid_moves=moves
    
    def get_color(self):
        return self.color

    def __repr__(self):
        return str(self.color)
