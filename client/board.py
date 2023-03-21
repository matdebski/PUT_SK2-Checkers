import pygame
SQUARE_SIZE=90
from piece import Piece



class Board:
    def __init__(self,color):

        self.board = []
        self.black_left = self.white_left = 12
        self.color=color
        self.create_board()

    def draw_squares(self,win):

        for i in range(8):
            for j in range(8):
                if j %2==((i+1)%2):
                    color=(128,195,216)
                else:
                    color=(255,255,255)

                pygame.draw.rect(win,color,(SQUARE_SIZE*j+40,SQUARE_SIZE*i+80,SQUARE_SIZE,SQUARE_SIZE))


    def create_board(self):
        
        for row in range(8):
            self.board.append([])
            for col in range(8):
                if col %2==(row+1)%2:
                    if row<3:
                        self.board[row].append(Piece('b',row,col))
                    elif row >4:
                        self.board[row].append(Piece('w',row,col))
                    else:
                        self.board[row].append(0)
                else:
                    self.board[row].append(0)
    

    def update_board(self,serverResponse): #decode server message and update board

        i=0
        serverResponse=serverResponse[1:]

        for row in range(8):
            for col in range(8):
                if serverResponse[i]=='x':
                    self.board[row][col]=0
                    i+=1
                else:
                    if serverResponse[i]=='d': #white king
                      piece=Piece('w',row,col)
                      piece.make_king()
                    elif serverResponse[i]=='u': #black king
                        piece=Piece('b',row,col)
                        piece.make_king()  
                    else:
                        piece=Piece(serverResponse[i],row,col)
                    i+=1
                    self.board[row][col]=piece
        return    

    def draw(self,win):
        if self.color=='w':
            color='w'
        else:
            color='b'

        self.draw_squares(win)

        for row in range(8):
            for col in range(8):

                piece=self.board[row][col]
                if piece!=0:
                    piece.draw(win,color)

    def move(self, piece, row, col):
        if self.color=='w':
            color='w'
        else:
            color='b'

        diff_row=row-piece.row
        diff_col=col-piece.col

        
        self.board[piece.row][piece.col], self.board[row][col] = self.board[row][col], self.board[piece.row][piece.col]
        piece.move(row, col,color)

        
        if abs(diff_row) == 2: #capture piece
            if diff_row<0:
                delete_row=piece.row+1 
            else:
                delete_row=piece.row-1
            
            if diff_col<0:
                delete_col=piece.col+1
            else:
                delete_col=piece.col-1

            self.board[delete_row][delete_col]=0

        if row == 7 or row == 0:
            piece.make_king()

        self.board[row][col].set_valid_moves([]) #delete valid moves for this piece

    def get_piece(self, row, col):
        return self.board[row][col]


    def get_valid_moves(self,row,col): 

        moves=[]
        #empty field
        if self.board[row][col]==0:
            return moves

        piece=self.board[row][col]

        #check if it's player's color
        if piece.__repr__()!=self.color:
            return moves

        else:
            
            moves=piece.get_valid_moves()
            if len(moves)==0:
                
                if piece.check_if_king():
                    left_up=[row+1,col-1]
                    left_down=[row-1,col-1]
                    right_up=[row+1,col+1]
                    right_down=[row-1,col+1]
                    fields=[left_up,right_up,left_down,right_down]
                    row_add=1
                else:
                    if self.color=='w':
                        row_add=-1
                    else:
                        row_add=1 
                    left=[row+row_add,col-1]
                    right=[row+row_add,col+1]    
                    fields=[left,right]

                for i in range(len(fields)):

                    if not(fields[i][0]<0 or fields[i][1]<0 or fields[i][0]>7 or fields[i][1]>7):
                        
                        #field is empty player can move
                        f=self.board[fields[i][0]][fields[i][1]]
                        if(f==0):
                            moves.append(fields[i])
                        
                        #oppenent's piece on this field
                        elif f.__repr__()!=piece.__repr__():
                            
                            if i%2==0:
                                col_add=-1
                            else:
                                col_add=1
                            if(i>1):
                                row_add=-1
                            
                            new_field=[fields[i][0]+row_add,fields[i][1]+col_add]
                            
                            if not(new_field[0]<0 or new_field[1]<0 or new_field[0]>7 or new_field[1]>7):
                                f=self.board[new_field[0]][new_field[1]]
                                
                                #check if player can capture opponent's piece
                                if(f==0):
                                    moves.append(new_field)

                self.board[row][col].set_valid_moves(moves)
             
        return moves
                    
    def calc_pos(self,row,col):
        
        
        if(self.color=='w'):
            x=(SQUARE_SIZE*col+40)+SQUARE_SIZE//2 
            y=(SQUARE_SIZE*row+ 75)+SQUARE_SIZE//2
        #rotate board if black
        else:
            x=(SQUARE_SIZE*(7-col)+40)+SQUARE_SIZE//2
            y=(SQUARE_SIZE*(7-row))+75+SQUARE_SIZE//2
        return (x,y)

    def draw_valid_moves(self,win,moves):

        for move in moves:
            row, col = move
            pygame.draw.circle(win, (0,255,0), self.calc_pos(row,col), 15)

    def msg(self): #encode current board state to message
        msg="m"

        for row in range(8):
            for col in range(8):
                piece=self.board[row][col]
                if piece==0:
                    msg+="x"
                else:
                    if piece.check_if_king() and piece.__repr__()=='w':
                        msg+="d"
                    elif piece.check_if_king() and piece.__repr__()=='b':
                        msg+="u"
                    else:
                        msg+=piece.__repr__()
        msg+="."
        return msg
    








