/*

c. - connection successfull
f. - server is full
w. - your color is white you need to wait for black player
b. - your color is black
o.:
  client -> server - I got your message,
  server -> client - You need to wait for  opponent to move

q. - opponent left the game, you win
m[board_state]. - move request:
    w - white piece
    d - white king
    b- black piece
    u - black king
    x- empty square

v[board_state]. - victory message
l[board_state]. - defeat message


initial board:
"
xbxbxbxb
bxbxbxbx
xbxbxbxb
xxxxxxxx
xxxxxxxx
wxwxwxwx
xwxwxwxw
wxwxwxwx

"
*/
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <errno.h>

#define TIME_OUT_SEC 600
#define GAME_LIMIT 2
#define PLAYERS_LIMIT 2 * GAME_LIMIT
#define MSG_SIZE_LIMIT 255
#define PORT 1234

int players_counter = 0;

int new_read(int, char, char *);
int new_write(int, char *);
bool send_to_client(int, char *, bool);

class Game
{
private:
    char current_board_state[65] = "xbxbxbxbbxbxbxbxxbxbxbxbxxxxxxxxxxxxxxxxwxwxwxwxxwxwxwxwwxwxwxwx";
    int whitePlayer = -1;
    int blackPlayer = -1;
    char nextmove = 'w';
    bool ifStarted = false;
    int winner = -1;

    void board_update(char *msg)
    {

        for (int i = 0; i < 64; i++)
        {
            current_board_state[i] = msg[i + 1];
        }
    }

    bool if_can_move(int direction_row, char (*tab)[8], int row, int col)

    {
        // check if piece at a given cords on the game board can move
        int new_row, direction_col[2], new_col;

        //-1:  left, 1: right
        direction_col[0] = -1;
        direction_col[1] = 1;

        new_row = row + direction_row;

        if (new_row >= 0 && new_row < 8)
        {

            for (int i = 0; i < 2; i++)
            {

                new_col = col + direction_col[i];

                if (new_col >= 0 && new_col < 8)
                {

                    if (tab[new_row][new_col] == 'x')
                        return true;

                    else if ((tab[row][col] == 'w' || tab[row][col] == 'd') && (tab[new_row][new_col] == 'b' || tab[new_row][new_col] == 'u'))
                    {

                        if (tab[new_row + direction_row][new_col + direction_col[i]] == 'x')
                            return true;
                    }

                    else if ((tab[row][col] == 'b' || tab[row][col] == 'u') && (tab[new_row][new_col] == 'w' || tab[new_row][new_col] == 'd'))
                    {

                        if (tab[new_row + direction_row][new_col + direction_col[i]] == 'x')
                            return true;
                    }
                }
            }
        }
        return false;
    }

    void game_over()
    {

        // check if the game is over by checking if either player has any moves left

        bool bCANmove = false;
        bool wCANmove = false;
        int b = 0, w = 0, i = 0;
        char tab[8][8], piece;

        for (int row = 0; row < 8; row++)
        {
            for (int col = 0; col < 8; col++)
            {
                tab[row][col] = current_board_state[i];
                i++;
            }
        }

        for (int row = 0; row < 8; row++)
        {

            if (bCANmove && wCANmove)
                break;

            for (int col = 0; col < 8; col++)
            {

                piece = tab[row][col];

                if (piece == 'w')
                {
                    w++;

                    if (wCANmove)
                        continue;
                    wCANmove = if_can_move(-1, tab, row, col);
                }
                else if (piece == 'd')
                {
                    w++;
                    if (wCANmove)
                        continue;
                    wCANmove = (if_can_move(-1, tab, row, col) || if_can_move(1, tab, row, col));
                }
                else if (piece == 'b')
                {

                    b++;
                    if (bCANmove)
                        continue;
                    bCANmove = if_can_move(1, tab, row, col);
                }
                else if (piece == 'u')
                {
                    b++;
                    if (bCANmove)
                        continue;
                    bCANmove = (if_can_move(-1, tab, row, col) || if_can_move(1, tab, row, col));
                }

                if (bCANmove && wCANmove)
                    break;
            }
        }

        if (b == 0 || !bCANmove)
            winner = whitePlayer;
        else if (w == 0 || !wCANmove)
            winner = blackPlayer;
    }

    void Reset()
    {
        // reset the game to its initial state, decrement players counter
        strcpy(current_board_state, "xbxbxbxbbxbxbxbxxbxbxbxbxxxxxxxxxxxxxxxxwxwxwxwxxwxwxwxwwxwxwxwx");
        nextmove = 'w';
        ifStarted = false;
        winner = -1;
        players_counter -= 2;
        if (players_counter < 0)
            players_counter = 0;
        return;
    }

public:
    int joinGame(int playerID, int game_index)
    {

        // player who join game first will get white white color, return 1
        // else if game not started and the black player slot is empty, the player will get black color, return 0
        // otherwise, if the game has already started or both player slots are already filled, return -1

        int r;
        if (!ifStarted && whitePlayer == -1)
        {
            whitePlayer = playerID;
            r = 1;
            printf("Player %d joined %d game, he is white side\n", playerID, game_index);
        }
        else if (!ifStarted && blackPlayer == -1)
        {
            blackPlayer = playerID;
            r = 0;
            printf("Player %d joined game: white: %d vs black: %d\n", playerID, whitePlayer, blackPlayer);
            ifStarted = true;
        }
        else
        {
            r = -1;
        }

        return r;
    }

    bool waiting_for_opponent(int playerID)
    {

        // waiting room for white player
        // if the player have black color, return true immediately because black player always joins as second, don't need to wait
        // if the white player leaves the waiting room before the black player joins, return false.

        bool player_alone = true;
        char msg[MSG_SIZE_LIMIT];

        if (playerID == blackPlayer)
            return true;
        strcpy(msg, "o.");

        while (player_alone)
        {

            if (!send_to_client(whitePlayer, msg, true))
            {
                printf("Player %d left the game during waiting for opponent!\n", playerID);
                return false;
            };

            if (blackPlayer != -1)
            {
                printf("opponent's been found %d %d!\n", whitePlayer, blackPlayer);
                strcpy(msg, "j.");
                send_to_client(whitePlayer, msg, true);
                return true;
            }
            sleep(0.2);
        }
        return false;
    }

    bool waiting_for_opponent_move(int playerID)
    {

        // return true if it's the player's turn to move
        // return false if the opponent left the game.

        bool OpponentTurn = true;
        char msg[MSG_SIZE_LIMIT];

        strcpy(msg, "o.");
        while (OpponentTurn)
        {

            if (whitePlayer == -1 || blackPlayer == -1)
            { // opponent left the game
                strcpy(msg, "q.");
                send_to_client(playerID, msg, false);
                return false;
            }
            if (!send_to_client(playerID, msg, true))
            {

                printf("PLayer: %d left the game\n", playerID);
                return false;
            };

            if ((nextmove == 'w' && playerID == whitePlayer) || (nextmove == 'b' && playerID == blackPlayer))
            {
                return true;
            }
        }
        return false;
    }

    int send_move_request(int playerID)
    {

        // return
        //  1 if it is the player's turn and send move request
        // 0 if the game is over, and send a message which depends whether the player won or lost
        //-1 if it is the opponent's turn

        char msg[MSG_SIZE_LIMIT];
        memset(msg, 0, MSG_SIZE_LIMIT);

        if ((playerID == whitePlayer && nextmove == 'w') || (playerID == blackPlayer && nextmove == 'b'))
        {
            if (winner == -1)
            {
                strcat(msg, "m");
                strcat(msg, current_board_state);
                strcat(msg, ".");
                send_to_client(playerID, msg, true);
                return 1;
            }
            else if (winner != playerID)
            {
                strcat(msg, "l");
                strcat(msg, current_board_state);
                strcat(msg, ".");
                send_to_client(playerID, msg, false);
                printf("msg: %s\n", msg);
                return 0;
            }
            else
            {
                strcat(msg, "v");
                strcat(msg, current_board_state);
                strcat(msg, ".");
                send_to_client(playerID, msg, false);
                printf("msg: %s\n", msg);
                return 0;
            }
        }
        else
            return -1;
    }

    bool receive_move(int clientID)
    {

        // receive a move from a client, update the board state, and checking if the game is over.
        // if the game is not over, return true and set the next player to move
        // if the game is over, returns false, it sends a message to the client indicating if they won or lost

        char buf[MSG_SIZE_LIMIT];
        char msg[MSG_SIZE_LIMIT];

        memset(buf, 0, MSG_SIZE_LIMIT);
        memset(msg, 0, MSG_SIZE_LIMIT);

        if (new_read(clientID, '.', buf) < 0)
            return false;
        if (buf[0] != 'm')
            return false;

        board_update(buf);

        game_over();

        if (winner == -1)
        {

            if (blackPlayer == -1 || whitePlayer == -1)
            {
                strcpy(msg, "q.");
                send_to_client(clientID, msg, false);
                return false;
            }
            else
                strcpy(msg, "o.");

            if (!send_to_client(clientID, msg, false))
                return false;

            if (clientID == whitePlayer)
                nextmove = 'b';
            else
                nextmove = 'w';

            return true;
        }
        else if (winner == clientID)
        {

            strcat(msg, "v");
            strcat(msg, current_board_state);
            strcat(msg, ".");
            send_to_client(clientID, msg, false);
            printf("msg: %s\n", msg);

            if (clientID == whitePlayer)
                nextmove = 'b';
            else
                nextmove = 'w';
            return false;
        }
        else
        {
            strcat(msg, "l");
            strcat(msg, current_board_state);
            strcat(msg, ".");
            send_to_client(clientID, msg, false);
            printf("msg: %s\n", msg);

            if (clientID == whitePlayer)
                nextmove = 'b';
            else
                nextmove = 'w';
            return false;
        }
    }

    void leftGame(int playerID)
    { //

        if (whitePlayer == playerID)
            whitePlayer = -1;
        else
            blackPlayer = -1;

        // if both players left, reset game
        if (whitePlayer == -1 && blackPlayer == -1)
            Reset();

        return;
    }
};

Game games[GAME_LIMIT];

int new_read(int fd, char sep, char *buf)
{

    // read data until it came across a separator character

    int count = 0;
    memset(buf, 0, MSG_SIZE_LIMIT);

    while (count < MSG_SIZE_LIMIT)
    {
        if (read(fd, buf + count, 1) < 0)
        {
            return -1;
        }

        if (buf[count] == sep)
        {
            return count;
        }
        else
        {
            count++;
        }
    }
    return -1;
}

int new_write(int fd, char *msg)
{

    int toSend = strlen(msg);
    int dataCount = 0, w;
    while (dataCount < toSend)
    {
        w = write(fd, msg + dataCount, toSend - dataCount);
        if (w < 0)
        {

            return -1;
        }
        dataCount += w;
    }
    return dataCount;
}

bool send_to_client(int clientID, char *msg, bool waitFORresponse)
{

    char buf[MSG_SIZE_LIMIT];
    bool result = true;
    memset(buf, 0, MSG_SIZE_LIMIT);

    if (new_write(clientID, msg) < 0)
        result = false;

    if (waitFORresponse)
    {
        if (new_read(clientID, '.', buf) < 0)
            result = false;
        if (buf[0] != 'o')
            result = false;
    }

    return result;
}

int findGame(int playerID)
{
    // return the gameID of the game that the player has joined  or -1 if not found any game, send color to player
    // if a game is found, the function ,send a message to the player containing their color

    int status, i;
    char msg[MSG_SIZE_LIMIT];
    char color;

    for (i = 0; i < GAME_LIMIT; i++)
    {
        status = games[i].joinGame(playerID, i);
        if (status >= 0)
            break;
    }

    if (status == -1)
    {
        return -1;
    }
    else
    {
        if (status == 1)
            color = 'w';
        else
            color = 'b';
        strcat(msg, &color);
        strcat(msg, ".");
        if (send_to_client(playerID, msg, true))
            return i;
        else
            return -1;
    }
}

void *ThreadBehavior(void *pconnection_socket_descriptor)
{
    int playerID = *(int *)pconnection_socket_descriptor;
    int gameID;
    bool connection_check = true;
    int r;

    // when thread terminates its resources will automatically relased
    pthread_detach(pthread_self());

    struct timeval tv;
    char buf[MSG_SIZE_LIMIT];
    char msg[MSG_SIZE_LIMIT];
    memset(buf, 0, MSG_SIZE_LIMIT);
    memset(msg, 0, MSG_SIZE_LIMIT);

    tv.tv_sec = TIME_OUT_SEC;
    tv.tv_usec = 0;

    // set timeout on socket
    if (setsockopt(playerID, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof tv) < 0)
    {
        perror("setsockopt TIME");
        connection_check = false;
    }

    gameID = findGame(playerID);

    if (gameID < 0)
        connection_check = false;

    if (!games[gameID].waiting_for_opponent(playerID))
        connection_check = false;

    // game loop
    while (connection_check)
    {

        if ((r = games[gameID].send_move_request(playerID)) == -1)
        {
            if (!games[gameID].waiting_for_opponent_move(playerID))
                connection_check = false;
        }
        else if (r == 1)
        {
            if (!games[gameID].receive_move(playerID))
                connection_check = false;
        }
        else
            connection_check = false;
    }
    printf("Client doesn't respond. Socket: %d will be closed.\n", playerID);
    games[gameID].leftGame(playerID);

    close(playerID);
    delete (int *)pconnection_socket_descriptor;

    printf("%d Client disconnected!\n", playerID);

    pthread_exit(NULL);
}

void handleConnection(int playerID)
{

    // create a new thread to handle a new client connection
    int create_result = 0;

    pthread_t thread1;

    int *ptr = new int;
    *ptr = playerID;

    create_result = pthread_create(&thread1, NULL, ThreadBehavior, ptr);

    if (create_result)
    {
        printf("pthread_created failed error: %d\n", create_result);
        exit(-1);
    }
}

int main(int argc, char *argv[])
{

    socklen_t slt;
    int sfd, cfd, on, port;
    struct sockaddr_in serv_adr, cli_adr;
    char msg[MSG_SIZE_LIMIT];

    port = PORT;
    printf("Port: %d\n", port);
    // port=atoi(argv[1]);
    // if (argc < 2) {
    //     fprintf(stderr,"ERROR, no port provided\n");
    //     exit(1);
    // }

    sfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sfd < 0)
    {
        perror("Error opening socket");
        exit(1);
    }

    memset(&serv_adr, 0, sizeof serv_adr);

    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = INADDR_ANY;
    serv_adr.sin_port = htons(port);

    if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int)) < 0)
    {
        perror("setsockopt REUSE");
        exit(1);
    }

    if (bind(sfd, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) < 0)
    {
        perror("ERROR on binding");
        exit(1);
    }

    if (listen(sfd, PLAYERS_LIMIT + 1) < 0)
    {
        perror("Error setting queue!");
        exit(1);
    }

    printf("Server: %s:%d Hello, It's me!\n", inet_ntoa(serv_adr.sin_addr), ntohs(serv_adr.sin_port));

    // server loop
    while (1)
    {

        slt = sizeof(cli_adr);
        cfd = accept(sfd, (struct sockaddr *)&cli_adr, &slt);

        if (cfd < 0)
            perror("Error on accept");
        else
        {
            if (players_counter == PLAYERS_LIMIT)
            {
                strcpy(msg, "f.");
                // printf("Player %d tried join to the game, but server is full\n",cfd);
                send_to_client(cfd, msg, false);

                close(cfd);
                continue;
            }
            else
            {
                strcpy(msg, "c.");
                if (!send_to_client(cfd, msg, true))
                {
                    close(cfd);
                    continue;
                };
                printf("Client: %s:%d joined to server! online: %d\n", inet_ntoa(cli_adr.sin_addr), ntohs(cli_adr.sin_port), players_counter);

                players_counter++;
                handleConnection(cfd);
            }
        }
    }
    close(sfd);
    return 0;
}
