import time
import sys
import pygame
from button import Button
from board import Board
import socket
from socket import AF_INET, SOCK_STREAM

HOST = str(sys.argv[1])
PORT = 1234
TIME_OUT_SEC = 300
# PORT = int(sys.argv[2]);

pygame.init()
FPS = 30
HEIGHT = 900
WIDTH = 800
SQUARE_SIZE = 90


screen = pygame.display.set_mode((WIDTH, HEIGHT))
clock = pygame.time.Clock()
clock.tick(FPS)

font = pygame.font.SysFont("arialblack", 30)
txt_color = (255, 255, 255)


def connect_server():
    """
    return:
     check - connection status
     msg - error or success message
     socket - socket descriptor
    """
    socket1 = socket.socket(AF_INET, SOCK_STREAM)
    socket1.settimeout(TIME_OUT_SEC)

    try:
        socket1.connect((HOST, PORT))
    except socket.timeout:
        msg = "Connection timeout exceeded"
        check = False
    except socket.gaierror:
        msg = "Invalid IP address or hostname"
        check = False
    except ConnectionRefusedError:
        msg = "Server rejected the connection"
        check = False
    else:
        print(HOST+":", PORT)

        print("Client start working...")
        serverResponse = receive(socket1)

        if serverResponse[0] == 'f':
            socket1.close()
            msg = "Server is full, try again later."
            check = False

        elif serverResponse[0] == 'c':
            msg = "Connection successful."
            check = True
    return check, msg, socket1


def send(desc, msg):

    b = msg.encode('utf-8')
    desc.send(b)

    return


def connection_err(msg):
    """
    display error msg and print more details to console
    """
    display_message("Connection error, game will be closed", (255, 0, 0))
    print("Error:", msg)
    pygame.display.update()

    time.sleep(1)

    pygame.quit()
    sys.exit()
    return


def receive(desc, confirm=True):
    """
    receive and validate msg from server, send confirmation if needed
    return: message from server
    """
    serverResponse = desc.recv(255)
    serverResponse1 = serverResponse.decode('utf-8')

    if confirm:
        send(desc, "o.")

    serverResponse2 = ""
    for i in serverResponse1:
        if i == '.':
            break
        serverResponse2 += i
    else:
        connection_err("Missing dot at the end of message!")

    if serverResponse2[0] == 'm' and len(serverResponse2) != 65:
        connection_err("Incorrect number of characters!")

    return serverResponse2


def display_message(msg, color=(255, 255, 255)):
    """
    show msg box
    """
    msgBOX = Button(image=pygame.image.load("assets/Options Rect.png"), pos=(400, 400),
                    text_input=msg, font=font, base_color=color, hovering_color=color)

    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            pygame.quit()
            sys.exit()

    msgBOX.update(screen)
    return


def draw_lobby():
    """
    draw lobby, connect user to the server
    return: socket descriptor
    """
    pygame.display.set_caption("Lobby")
    check = True

    # run=True
    while True:
        CONNECT_MOUSE_POS = pygame.mouse.get_pos()

        screen.fill((52, 78, 91))

        CONNECT = Button(image=pygame.image.load("assets/Options Rect.png"), pos=(400, 400),
                         text_input="CONNECT", font=font, base_color=(255, 255, 255), hovering_color=(0, 255, 0))

        CONNECT.changeColor(CONNECT_MOUSE_POS)
        CONNECT.update(screen)

        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit()
                sys.exit()

            if event.type == pygame.MOUSEBUTTONDOWN:
                if CONNECT.checkForInput(CONNECT_MOUSE_POS):

                    check, msg, socket = connect_server()
                    print(msg)
                    if (check):
                        return socket

        if not (check):
            txt = font.render(msg, True, (255, 0, 0))
            screen.blit(txt, (250, 470))

        pygame.display.update()


def get_row_col_from_mouse(pos, player_color):
    """
    pos - cursor possition

    converts pos to row and column of the selected square
    if the poss is outside the board, the function returns -1 -1

    return: row, col
    """
    x, y = pos
    if x > 40 and x < 40+SQUARE_SIZE*8 and y > 80 and y < 80+SQUARE_SIZE*8:

        row = (y-80) // SQUARE_SIZE
        col = (x-40) // SQUARE_SIZE

        if player_color == 'w':
            return row, col
        else:
            return 7-row, 7-col
    else:
        return -1, -1


def draw_game(board, showMoves=False, moves=[]):
    """
    draws interface, if the showMoves parameter is set to True, it also draws possible moves passed through the moves parameter
    """
    if board.color == 'w':
        op = '[black]'
        you = '[white]'
    else:
        op = '[white]'
        you = '[black]'
    screen.fill((52, 78, 91))
    txt = font.render("Opponent "+op, True, (255, 0, 0))
    screen.blit(txt, (50, 20))

    txt = font.render("You "+you, True, (0, 255, 0))
    screen.blit(txt, (50, 840))

    board.draw(screen)
    if showMoves:
        board.draw_valid_moves(screen, moves)

    return


def make_move(board, serverResponse, color):
    """
    handle move request from server:
    1.Decode the server response and update the board.
    2.Wait for the player to select a piece.
    3.Show possible moves for the selected piece.
    4.Wait for the player to choose a move.
    5.Update the board.
    6.Encode the current board state as a message
    7.return message
    """

    moves = []
    board.update_board(serverResponse)
    showMoves = False
    waitFORmove = True

    while (waitFORmove):
        draw_game(board, showMoves, moves)

        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit()
                sys.exit()

            if event.type == pygame.MOUSEBUTTONDOWN:
                pos = pygame.mouse.get_pos()
                row, col = get_row_col_from_mouse(pos, color)

                # click outside of board
                if row == -1 or col == -1:
                    moves = []
                    showMoves = False
                    continue

                if showMoves and [row, col] in moves:

                    board.move(piece, row, col)
                    result = board.msg()
                    waitFORmove = False
                else:
                    moves = board.get_valid_moves(row, col)

                    if len(moves) == 0:  # no possible move for this piece
                        showMoves = False
                        continue
                    piece = board.get_piece(row, col)
                    showMoves = True
        pygame.display.update()
    return result


def game_loop(board, color, socket):

    pygame.display.set_caption("Game")
    run = True

    if color == 'w':
        waitFORblack = True
    else:
        waitFORblack = False

    while run:

        draw_game(board)

        serverResponse = receive(socket)

        if (waitFORblack):
            display_message("Waiting for another player...")

            if (serverResponse[0] == 'j'):
                waitFORblack = False

        if (not (waitFORblack) and serverResponse[0] == 'o'):
            display_message("Waiting for opponent move...")

        if serverResponse[0] == 'q':
            display_message("Opponent is AFK, VICTORY!")
            pygame.display.update()
            time.sleep(1)
            break

        if (not (waitFORblack) and serverResponse[0] == 'l'):

            print("You lost after op move")
            board.update_board(serverResponse)
            draw_game(board)
            display_message("--GAME OVER--")
            pygame.display.update()
            break

        if (not (waitFORblack) and serverResponse[0] == 'v'):
            print("You win after op move")
            board.update_board(serverResponse)
            draw_game(board)
            display_message("--VICTORY--", (255, 223, 0))

            pygame.display.update()
            break

        if (not (waitFORblack) and serverResponse[0] == 'm'):
            result = make_move(board, serverResponse, color)
            send(socket, result)

            serverResponse = receive(socket, confirm=False)

            if serverResponse[0] == 'q':
                display_message("Opponent is AFK, VICTORY!")
                pygame.display.update()
                break

            elif serverResponse[0] == 'v':
                print("You win after your move")
                board.update_board(serverResponse)
                draw_game(board)
                display_message("--VICTORY--", (255, 223, 0))

                pygame.display.update()
                break

            elif serverResponse[0] == 'l':
                print("You lost after your move")
                board.update_board(serverResponse)
                draw_game(board)
                display_message("--GAME OVER--")
                pygame.display.update()
                break

            elif serverResponse[0] != 'o':
                connection_err("Expected server response!")

        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit()
                sys.exit()

        pygame.display.update()
    return


def main():

    socket = draw_lobby()

    serverResponse = receive(socket)

    if serverResponse[0] == 'w':
        print("color: white")
        color = 'w'

    elif serverResponse[0] == 'b':
        color = 'b'
        print("color: black")

    board = Board(color)

    game_loop(board, color, socket)

    socket.close()

    while (1):
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit()
                sys.exit()


if __name__ == "__main__":
    main()
