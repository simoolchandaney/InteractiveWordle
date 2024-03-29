#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdbool.h>
#include "../cJSON.h"


/* Global Defines */
#define BUFFER_MAX 1000
#define BACKLOG 10   // how many pending connections queue will hold

/* Global Variables */
pthread_mutex_t     g_BigLock;

struct time {
    int tm_sec; // seconds (from 0 to 59)
    int tm_min; // minutes (from 0 to 59)
    int tm_hour; // hours (from 0 to 23)
};

struct ClientInfo
{
    pthread_t threadClient;
    char szIdentifier[100];
    int  socketClient;
};

typedef struct Input {
    int numPlayers; //total number expected
    char *lobbyPort;
    char *playPort;
    int numRounds;
    char *fileName;
    int dbg;
} Input;

typedef struct Game_Player {
    char *name;
    char *client;
    int number;
    char *guess;
    int score;
    char *correct;
    char receipt_time[100];
    char *result;
    int  score_earned;
    char *winner;
    struct ClientInfo clientInfo;
} Game_Player;

typedef struct Lobby_Player {
    char *name;
    char *client;
    struct ClientInfo clientInfo;
} Lobby_Player;

typedef struct Wordle
{
    struct Lobby_Player lobbyPlayers[10];
    struct Game_Player players[10];
    char *word;
    int num_in_lobby; //current num in lobby
    int num_players; //current num in game
    int nonce;
    char *winner_name;
    char *winner;
    int created_game;
    int game_over;
    char *chat_message;
    int num_guessed;
    struct Input inputs;
    int in_lobby;
} Wordle;

Wordle wordle;
bool dbg;

#define BACKLOG 10   // how many pending connections queue will hold
char *interpret_message(cJSON *message, struct ClientInfo threadClient);
cJSON *get_message(char *message_type, char *contents[], char *fields[], int content_length);
void sigchld_handler(int s) {
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

char *get_IP() {
    //get ip of currect machine
    char hostbuffer[256];
    char *IPbuffer;
    struct hostent *host_entry;

  
    // To retrieve hostname
    gethostname(hostbuffer, sizeof(hostbuffer));
  
    // To retrieve host information
    host_entry = gethostbyname(hostbuffer);
  
    // To convert an Internet network
    // address into ASCII string
    IPbuffer = inet_ntoa(*((struct in_addr*)
                           host_entry->h_addr_list[0]));

    return IPbuffer;
}


void send_data(struct ClientInfo threadClient, char *response) {
    if(dbg) {
        printf("sending to %d: %s\n", threadClient.socketClient, response);
    }
    if (send(threadClient.socketClient, response, strlen(response), 0) == -1)
    {
        perror("send");     
    }
}

void send_data_all(char *response) {
    //send data to al clients
    for(int i = 0; i < wordle.inputs.numPlayers; i++) {
        struct ClientInfo cInfo = (wordle.in_lobby == 0) ? wordle.players[i].clientInfo : wordle.lobbyPlayers[i].clientInfo;
        send_data(cInfo, response);
    }
}


char *receive_data(struct ClientInfo threadClient, int timeout) {
    //receive data from socket
    int numBytes;
    char szBuffer[BUFFER_MAX];
    struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    setsockopt(threadClient.socketClient, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    if ((numBytes = recv(threadClient.socketClient, szBuffer, BUFFER_MAX-1, 0)) == -1) {
        return "";
    }
    szBuffer[numBytes] = '\0';

    char *return_val = malloc(sizeof(szBuffer));
    memcpy(return_val, szBuffer, numBytes);
    if(dbg) {
        printf("received: %s\n", return_val);
    }
    return return_val;
}

int get_nonce() {
    //random nonce
    int r = rand() % 1000;
    wordle.nonce = r;
    return r;
}

// function that selects random word from text file
char * word_to_guess() {
    int wordCount = 0;
    FILE *file_handle = fopen ("terms.txt", "r");
    char chunk[BUFSIZ];
    strcpy(chunk, "");
    while(fgets(chunk, sizeof(chunk), file_handle) != NULL) {
        //fputs(chunk, stdout);
        wordCount++;
    }
    //printf("wordCount is: %d\n", wordCount);
    srand(time(NULL));
    int r = (rand() % wordCount) + 1;
    //printf("%d\n", r);
    rewind(file_handle);

    int i;
    char chunk2[BUFSIZ];
    for (i = 0; i < r; i++) {
       fgets(chunk2, sizeof(chunk2), file_handle); 
    }
    chunk2[strlen(chunk2)  - 1] = '\0';
    fclose(file_handle);
    char *word = chunk2;
    for(int i = 0; i < strlen(word); i++) {
        word[i] = toupper(word[i]);
    }
    return word;
}

int score_calc(char *color_word) {
    //sophisticated scoring algorithm
    int total = 0;
    bool all_correct = true;

    for(int i = 0; i < strlen(color_word); i++) {

        if(color_word[i] == 'G') {
            total+=5;
        }
        else if(color_word[i] == 'Y') {
            total+=3;
            all_correct = false;
        }
        else {
            all_correct = false;
        }
    }

    if(all_correct) {
        total += 5;
    }
    return total;
}


char * word_guess_color_builder(char * guess, char * key) {
    //build result string
    char letters[BUFSIZ];

    int l, i, j, k; 
    int count = 0;
    int *found_pos = malloc(sizeof(guess));
    for (l = 0; l < strlen(guess); l++) {
        letters[l] = 'B'; // grey
    }

    for (k = 0; k < strlen(guess); k++) {
        if (guess[k] == key[k]) {
            letters[k] = 'G'; // green 
            found_pos[count] = k;
        }
    }

    for (i = 0; i < strlen(guess); i++) {
        for (j = 0; j < strlen(key); j++) {
            if (guess[i] == key[j] && j != found_pos[count]) {
                letters[i] = 'Y'; // yellow 
            }
        }
    }
    for (k = 0; k < strlen(guess); k++) {
        if (guess[k] == key[k]) {
            letters[k] = 'G'; // green 
        }
    }
    free(found_pos);
    char *str = letters;
    return str;
}


int check_profanity(char *message) {
    //filter out profanity
    char *profanity[] = {"fuck", "shit", "bitch", "damn", "pussy", "penis", "dick", "cock", "bastard", "asshole"};

    for(int i = 0; i < sizeof(profanity)/sizeof(profanity[0]); i++) {
        if(strstr(message, profanity[i])) {
            return 1;
        }
    }
    return 0;
}


void promptGuess(int guess_number, char *name, char *word_length_s, struct ClientInfo threadClient) {
    //ask client to guess a word
    char *contents1[3] = {"WordLength", "Name", "GuessNumber"};
    char guess_number_s[2];
    sprintf(guess_number_s, "%d", guess_number);
    char *fields1[3]  = {"", name, ""};
    fields1[0] = word_length_s;
    fields1[2] = guess_number_s;
    char *response = cJSON_Print(get_message("PromptForGuess", contents1, fields1, 3));
    send_data(threadClient, response);
}


cJSON *get_message(char *message_type, char *contents[], char *fields[], int content_length) {
    //create json for inputted message
    cJSON *message = cJSON_CreateObject();
    cJSON_AddStringToObject(message, "MessageType", message_type);
    cJSON *data = cJSON_CreateObject();
    for(int i = 0; i < content_length; i++) {
        if(!strcmp(message_type, "StartGame") && (!strcmp(contents[i], "PlayerInfo"))) {
            cJSON *players = cJSON_CreateArray();
            for(int j = 0; j < wordle.num_players; j++) {
                cJSON *item = cJSON_CreateObject();
                cJSON_AddStringToObject(item, "Name", wordle.players[j].name);
                char buffer[2];
                sprintf(buffer, "%d", wordle.players[j].number);
                cJSON_AddStringToObject(item, "Number", buffer);
                cJSON_AddItemToArray(players, item);
            }

            cJSON_AddItemToObject(data, contents[i], players);
        }
        else if(!strcmp(message_type, "StartRound") && (!strcmp(contents[i], "PlayerInfo"))) {
            cJSON *players = cJSON_CreateArray();
            for(int j = 0; j < wordle.num_players; j++) {
                cJSON *item = cJSON_CreateObject();
                cJSON_AddStringToObject(item, "Name",wordle.players[j].name);
                char buffer[2];
                sprintf(buffer, "%d", wordle.players[j].number);
                cJSON_AddStringToObject(item, "Number", buffer);
                char buffer2[2];
                sprintf(buffer2, "%d", wordle.players[j].score);
                cJSON_AddStringToObject(item, "Score", buffer2);
                cJSON_AddItemToArray(players, item);
            }

            cJSON_AddItemToObject(data, contents[i], players);
        }

        else if(!strcmp(message_type, "GuessResult") && (!strcmp(contents[i], "PlayerInfo"))) {
            cJSON *players = cJSON_CreateArray();
            for(int j = 0; j < wordle.num_players; j++) {
                cJSON *item = cJSON_CreateObject();
                cJSON_AddStringToObject(item, "Name", wordle.players[j].name);
                char buffer[2];
                sprintf(buffer, "%d", wordle.players[j].number);
                cJSON_AddStringToObject(item, "Number", buffer);
                cJSON_AddStringToObject(item, "Correct", wordle.players[j].correct);
                cJSON_AddStringToObject(item, "ReceiptTime", wordle.players[j].receipt_time);
                cJSON_AddStringToObject(item, "Result", wordle.players[j].result);
                cJSON_AddItemToArray(players, item);
            }

            cJSON_AddItemToObject(data, contents[i], players);
        }

        else if(!strcmp(message_type, "EndRound") && (!strcmp(contents[i], "PlayerInfo"))) {
            cJSON *players = cJSON_CreateArray();
            for(int j = 0; j < wordle.num_players; j++) {
                cJSON *item = cJSON_CreateObject();
                cJSON_AddStringToObject(item, "Name", wordle.players[j].name);
                char buffer[2];
                sprintf(buffer, "%d", wordle.players[j].number);
                cJSON_AddStringToObject(item, "Number", buffer);
                char buffer2[2];
                sprintf(buffer2, "%d", wordle.players[j].score_earned);
                cJSON_AddStringToObject(item, "ScoreEarned", buffer2);
                cJSON_AddStringToObject(item, "Winner", wordle.players[j].winner);
                cJSON_AddItemToArray(players, item);
            }

            cJSON_AddItemToObject(data, contents[i], players);
        }

        else if(!strcmp(message_type, "EndGame") && (!strcmp(contents[i], "PlayerInfo"))) {
            cJSON *players = cJSON_CreateArray();
            for(int j = 0; j < wordle.num_players; j++) {
                cJSON *item = cJSON_CreateObject();
                cJSON_AddStringToObject(item, "Name", wordle.players[j].name);
                char buffer[2];
                sprintf(buffer, "%d", wordle.players[j].number);
                cJSON_AddStringToObject(item, "Number", buffer);
                char buffer2[2];
                sprintf(buffer2, "%d", wordle.players[j].score);
                cJSON_AddStringToObject(item, "Score", buffer2);
                cJSON_AddItemToArray(players, item);
            }

            cJSON_AddItemToObject(data, contents[i], players);
        }

        else {
            cJSON *field = cJSON_CreateString(fields[i]);
            cJSON_AddItemToObject(data, contents[i], field);
        }
    }
    cJSON_AddItemToObject(message, "Data", data);
    return message;
}

void join(char *name, char *client, struct ClientInfo threadClient) {
    //user wishes to join lobby

    //make sure name is unique
    int unique  = 1;
    for(int i = 0; i < wordle.num_in_lobby; i++) {
        if (!strcmp(name, wordle.lobbyPlayers[i].name)) {
            unique = 0;
        }
    }
    //reserved name
    if(!strcmp(name, "mpwordle")) {
        unique = 0;
    }

    //filter names
    if(check_profanity(name) == 1) {
        unique = 0;
    }


    //name not accepted
    char *fields[2] = {name, ""};
    if(unique == 0) {
        fields[1] = "No";
    }
    else {
        fields[1] = "Yes";
        Lobby_Player player;
        player.name = name;
        player.client = client;
        player.clientInfo = threadClient;
        wordle.lobbyPlayers[wordle.num_in_lobby] = player;
    }
    char *contents[2] = {"Name", "Result"};
    char *response = cJSON_Print(get_message("JoinResult", contents, fields, 2));
    wordle.num_in_lobby += 1;
    send_data(threadClient, response);
}

void chat(char *name, char *text, struct ClientInfo threadClient) {
    //send chat message to all clients

    //filter out profanity
    if(check_profanity(text) == 1) {
        char new_text[BUFFER_MAX];
        snprintf(new_text, sizeof(new_text), "%s SAID A BAD WORD. KEEP IT CLEAN!", name);
        text = new_text;
        name = "mpwordle";
    }
    char *contents[2] = {"Name", "Text"};
    char *fields[2] = {name, text};
    char *response = cJSON_Print(get_message("Chat", contents, fields, 2));
    send_data_all(response);
}

void joinInstance(char *name, char *nonce, struct ClientInfo threadClient) {
    //client wants to join game lobby

    //check if name and nonce is valid
    int unique = 1;
    for(int i =0; i < wordle.num_players; i++) {
        if(!strcmp(name, wordle.players[i].name)) {
            unique = 0;
        }
    }
    //reserved word
    if(!strcmp(name, "mpwordle")) {
        unique = 0;
    }

    char buffer[2];
    sprintf(buffer, "%d", wordle.num_in_lobby);
    char *fields[3] = {name, buffer, ""};
    if(!unique || atoi(nonce) != wordle.nonce) {
        fields[2] = "No";
    }
    else {
        fields[2] = "Yes";
        Game_Player player;
        player.name = name;
        player.number = wordle.num_players + 1;
        player.score = 0;
        player.score_earned = 0;
        player.clientInfo = threadClient;
        wordle.players[wordle.num_players] = player;
        wordle.num_players += 1;
    }
    char *contents[3] = {"Name", "Number", "Result"};
    char *response = cJSON_Print(get_message("JoinInstanceResult", contents, fields, 3));
    send_data(threadClient, response);
}

char *checkGuess(char *name, char *guess, struct ClientInfo threadClient) {
    //check if guess is correct

    char *accepted = "Yes";
    for(int i = 0; i < strlen(guess); i++) {
        guess[i] = toupper(guess[i]);
    }
    //check for invalid word
    if(strlen(wordle.word) != strlen(guess)) {
        accepted = "No";
    }

    //check if word is correct
    else {
        for(int i = 0; i < wordle.num_players; i++) {
            if(!strcmp(wordle.players[i].name, name)) {
                if(!strcmp(wordle.word, guess)) {
                    wordle.players[i].correct = "Yes";
                    wordle.winner = "Yes";
                }
                else {
                    wordle.players[i].correct = "No";
                }
                wordle.players[i].guess = guess;
                time_t tmi;
                struct  time* utcTime;
                time(&tmi);
                utcTime = (struct time *) gmtime(&tmi);
                char time_buff[100];
                snprintf(time_buff, sizeof(time_buff), "%2d:%02d:%02d", (utcTime->tm_hour) % 24, utcTime->tm_min, utcTime->tm_sec);
                //printf("time: %s\n", time_buff);
                strcpy(wordle.players[i].receipt_time, time_buff);
                wordle.players[i].result = word_guess_color_builder(guess, wordle.word);
                wordle.players[i].score_earned = score_calc(wordle.players[i].result);
                wordle.players[i].score+= wordle.players[i].score_earned;
                break;
            }
        }
    }
    
    char *contents[3] = {"Name", "Guess", "Accepted"};
    char *fields[3] = {name, guess, accepted};
    char *response = cJSON_Print(get_message("GuessResponse", contents, fields, 3));
    send_data(threadClient, response);
    return accepted;
}

char *interpret_message(cJSON *message, struct ClientInfo threadClient) {
    //do something based on wht is received
    char * message_type = cJSON_GetStringValue(cJSON_GetObjectItem(message, "MessageType"));
    if(!strcmp(message_type, "Join")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        char *name = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Name"));
        char *client = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Client"));
        join(name, client, threadClient); 

    }
    else if(!strcmp(message_type, "Chat")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        char *name = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Name"));
        char *text = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Text"));
        chat(name, text, threadClient);
    }

    else if(!strcmp(message_type, "JoinInstance")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        char *name = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Name"));
        char *nonce = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Nonce"));
        joinInstance(name, nonce, threadClient);
        return name;
    }

    else if(!strcmp(message_type, "Guess")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        char *name = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Name"));
        char *guess = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Guess"));
        return checkGuess(name, guess, threadClient);
    }
    else {
        //invalid json, ignore
        char *message = receive_data(threadClient, 1);
        interpret_message(cJSON_Parse(message), threadClient);
    }
    return "";
}

void * Thread_Game (void * pData)
{
    //thread for a client in a game
    struct ClientInfo * pClient;
    struct ClientInfo   threadClient;
    
    char *szBuffer;
    
    // Typecast to what we need 
    pClient = (struct ClientInfo *) pData;
    
    // Copy it over to a local instance 
    threadClient = *pClient;

    //receive join instance
    szBuffer = receive_data(threadClient, 0);
    pthread_mutex_lock(&g_BigLock);
    char *name = interpret_message(cJSON_Parse(szBuffer), threadClient);
    free(szBuffer);
    pthread_mutex_unlock(&g_BigLock);

    //loop until all players have joined
    while(1) {
        szBuffer = receive_data(threadClient, 1);
        pthread_mutex_lock(&g_BigLock);
        if(strlen(szBuffer) > 0) {
            interpret_message(cJSON_Parse(szBuffer), threadClient);
            free(szBuffer);
        }
        if(wordle.num_players == wordle.inputs.numPlayers) {
            pthread_mutex_unlock(&g_BigLock);;
            break;
        }
        else {
            pthread_mutex_unlock(&g_BigLock);
        }
    }
    //keep playing games
    while(1) {
        //send start game message
        pthread_mutex_lock(&g_BigLock);
        char *contents0[2] = {"Name", "Text"};
        char *fields0[2] = {"mpwordle", "Starting Game"};
        char *response0 = cJSON_Print(get_message("Chat", contents0, fields0, 2));
        send_data(threadClient, response0);
        sleep(1);
        wordle.winner = "No";
        char *contents[2] = {"Rounds", "PlayerInfo"};
        char buffer[2];
        sprintf(buffer, "%d", wordle.inputs.numRounds);
        char *fields[2] = {buffer, NULL};
        char *response = cJSON_Print(get_message("StartGame", contents, fields, 2));
        send_data(threadClient, response);
        pthread_mutex_unlock(&g_BigLock);

        int num_round = 1;
        pthread_mutex_lock(&g_BigLock);
        //loop through each round
        int rounds_remaining = wordle.inputs.numRounds;
        while(rounds_remaining > 0) {
            char *contents[4] = {"WordLength", "Round", "RoundsRemaining", "PlayerInfo"};
            char word_length_s[2];
            sprintf(word_length_s, "%d", (int)strlen(wordle.word));
            char round_s[2];
            sprintf(round_s, "%d", num_round);
            char rounds_remaining_s[2];
            sprintf(rounds_remaining_s, "%d", rounds_remaining);
            char *fields[4] = {"", "", "", NULL};
            fields[0] = word_length_s;
            fields[1] = round_s;
            fields[2] = rounds_remaining_s;
            //reset winner fields
            for(int i = 0; i < wordle.num_players; i++) {
                wordle.players[i].winner = "No";
            }
            //send start round message
            char *contents00[2] = {"Name", "Text"};
            char start_round[50] = "Starting Round ";
            strcat(start_round, round_s);
            char *fields00[2] = {"mpwordle", start_round};
            char *response00 = cJSON_Print(get_message("Chat", contents00, fields00, 2));
            send_data(threadClient, response00);
            sleep(1);
            char *response = cJSON_Print(get_message("StartRound", contents, fields, 4));
            send_data(threadClient, response);
            pthread_mutex_unlock(&g_BigLock);
            sleep(1);
            pthread_mutex_lock(&g_BigLock);
            wordle.num_guessed = 0;
            pthread_mutex_unlock(&g_BigLock);
            rounds_remaining--;
            sprintf(rounds_remaining_s, "%d", rounds_remaining);
            int guess_number = 1;
            char *check = "Yes";
            char * data = "";
            //prompt for guess
            promptGuess(guess_number, name, word_length_s, threadClient);

            //check for messages while awaiting guess
            while(1) {
                data = receive_data(threadClient, 1);
                pthread_mutex_lock(&g_BigLock);
                //received a message
                if(strlen(data) > 0) {
                    check = interpret_message(cJSON_Parse(data), threadClient); //returns yes if accepted string
                    if(!strcmp(check, "Yes")) {
                        wordle.num_guessed++;
                        pthread_mutex_unlock(&g_BigLock);
                        break;
                    }  
                    else {
                        pthread_mutex_unlock(&g_BigLock);
                    }
                    free(data);
                }
                else {
                    pthread_mutex_unlock(&g_BigLock);
                }
            }
            sleep(1); 

            //stall until all guesses are received
            while(1) {
                data = receive_data(threadClient, 1);
                pthread_mutex_lock(&g_BigLock);
                if(strlen(data) > 0) {
                    interpret_message(cJSON_Parse(data), threadClient);
                    free(data);
                }
                if(wordle.num_guessed == wordle.num_players) {
                    pthread_mutex_unlock(&g_BigLock);
                    break;
                }
                else {
                    pthread_mutex_unlock(&g_BigLock);
                }
            }

            //send guess result
            pthread_mutex_lock(&g_BigLock);
            char *contents2[2] = {"Winner", "PlayerInfo"};
            char *fields2[2] = {wordle.winner, NULL};
            response = cJSON_Print(get_message("GuessResult", contents2, fields2, 2));
            send_data(threadClient, response);
            pthread_mutex_unlock(&g_BigLock);

            sleep(1);
            pthread_mutex_lock(&g_BigLock);
            if(!strcmp(wordle.winner, "Yes")) {
            strcpy(rounds_remaining_s, "0");
            } 
            //end round
            char *contents3[2] = {"RoundsRemaining", "PlayerInfo"};
            char *fields3[2] = {rounds_remaining_s, NULL};
            response = cJSON_Print(get_message("EndRound", contents3, fields3, 2));
            send_data(threadClient, response);
            pthread_mutex_unlock(&g_BigLock);

            pthread_mutex_lock(&g_BigLock);
            if(!strcmp(wordle.winner, "Yes")) {
                break;
            }
        }
        pthread_mutex_unlock(&g_BigLock);

        //send end game since done with every round
        pthread_mutex_lock(&g_BigLock);
        sleep(1);
        //find winner based on total score
        int max_score = 0;
        for(int i = 0; i < wordle.inputs.numPlayers; i++) {
            if(wordle.players[i].score >= max_score) {
                max_score = wordle.players[i].score;
                wordle.winner_name = wordle.players[i].name;
            }
        }
        char *contents4[2] = {"WinnerName", "PlayerInfo"};
        char *fields4[2] = {wordle.winner_name, NULL};
        response = cJSON_Print(get_message("EndGame", contents4, fields4, 2));
        send_data(threadClient, response);
        wordle.game_over = 1;
        //reset scores
        pthread_mutex_unlock(&g_BigLock);
        sleep(5); //wait for all threads to be done
        pthread_mutex_lock(&g_BigLock);
        for(int i = 0; i < wordle.inputs.numPlayers; i++) {
            wordle.players[i].score = 0;
        }
        pthread_mutex_unlock(&g_BigLock);
    }

        return NULL;
    }

    void * Thread_Lobby (void * pData)
    {
        //thread for client in lobby
        struct ClientInfo * pClient;
        struct ClientInfo   threadClient;
        
        char *szBuffer;
        
        // Typecast to what we need 
        pClient = (struct ClientInfo *) pData;

        // Copy it over to a local instance 
        threadClient = *pClient;

        //receive join
        szBuffer = receive_data(threadClient, 0);
        pthread_mutex_lock(&g_BigLock);
        //interpret Join message and send JoinResult back
        interpret_message(cJSON_Parse(szBuffer), threadClient);
        free(szBuffer);
        pthread_mutex_unlock(&g_BigLock);
        

        //wait until all players are in lobby
        while(1) {
            szBuffer = receive_data(threadClient, 1);
            pthread_mutex_lock(&g_BigLock);
            if(strlen(szBuffer) > 0) {
                interpret_message(cJSON_Parse(szBuffer), threadClient);
                free(szBuffer);
            }
            if(wordle.num_in_lobby == wordle.inputs.numPlayers) {
                pthread_mutex_unlock(&g_BigLock);
                break;
            }
            else {
                pthread_mutex_unlock(&g_BigLock);
            } 
        }

        //chat messages while starting game
        while(1) {
            szBuffer = receive_data(threadClient, 1);
            pthread_mutex_lock(&g_BigLock);
            if(strlen(szBuffer) > 0) {
                interpret_message(cJSON_Parse(szBuffer), threadClient);
                free(szBuffer);
            }
            else {
                pthread_mutex_unlock(&g_BigLock);
                break;
            }
            pthread_mutex_unlock(&g_BigLock);
        }

        //send startInstance to all clients
        pthread_mutex_lock(&g_BigLock);
        char *contents[3] = {"Server", "Port", "Nonce"};
        char buffer[5];
        sprintf(buffer, "%d", wordle.nonce);
        char *IP = get_IP();
        char *fields[3] = {IP, wordle.inputs.playPort, buffer};
        cJSON *message = get_message("StartInstance", contents, fields, 3);
        sleep(1); //make sure message sends seperately
        send_data(threadClient, cJSON_Print(message));
        wordle.in_lobby = 0;
        pthread_mutex_unlock(&g_BigLock);
        

    return NULL;
    
}

void Get_Clients(char *nPort, int choice)
{
    
    // Adapting this from Beej's Guide
    
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    //struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;
    
    struct ClientInfo theClientInfo; 

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, nPort, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    for(int i = 0; i< wordle.inputs.numPlayers; i++) 
    {  
        sin_size = sizeof their_addr;       
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        
        if (new_fd == -1) 
        {
            perror("accept");
            continue;
        }

        // Simple bit of code but this can be helpful to detect successful connections 
         
         
        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
s, sizeof s);
        printf("server: got connection from %s\n", s);

        // Print out this client information 
        //sprintf(theClientInfo.szIdentifier, "%s-%d", s, nClientCount);
        theClientInfo.socketClient = new_fd;

        // From OS: Three Easy Pieces 
        //   https://pages.cs.wisc.edu/~remzi/OSTEP/threads-api.pdf 
        if(choice == 0) {
            pthread_create(&(theClientInfo.threadClient), NULL, Thread_Lobby, &theClientInfo);
        }
        else {
            pthread_create(&(theClientInfo.threadClient), NULL, Thread_Game, &theClientInfo);
        }

    }

    close(sockfd);  
}

int main(int argc, char *argv[]) {   
    srand(time(NULL));
    int numPlayers = 2;
    char *lobbyPort = "41345";
    char *playPort = "41346";
    int numRounds = 3;
    char fileName[BUFSIZ] = "../terms.txt";

    for (int i = 1; i < argc; i+=2) {
        if (!strcmp(argv[i], "-np")) {
            numPlayers = atoi(argv[i+1]);
        }
        else if (!strcmp(argv[i], "-lp")) {
            lobbyPort = argv[i+1];
        }
        else if (!strcmp(argv[i], "-pp")) {
            playPort = argv[i+1];
        }
        else if (!strcmp(argv[i], "-nr")) {
            numRounds = atoi(argv[i+1]);
        }
        else if (!strcmp(argv[i], "-d")) {
            sprintf(fileName, "../%s", argv[i+1]);

        }  
        else if (!strcmp(argv[i], "-dbg")) {
            dbg = true;
        }
    }

    //set globals
    pthread_mutex_init(&g_BigLock, NULL);
    pthread_mutex_lock(&g_BigLock);
    get_nonce();
    wordle.num_in_lobby = 0;
    wordle.game_over = 0;
    wordle.created_game = 0;
    wordle.inputs.numPlayers = numPlayers;
    wordle.inputs.lobbyPort = lobbyPort;
    wordle.inputs.playPort = playPort;
    wordle.inputs.numRounds = numRounds;
    wordle.inputs.fileName = fileName;
    wordle.in_lobby = 1;
    char *word = word_to_guess();
    wordle.word = word;
    if(dbg) {
        printf("word is: %s\n", word);
    }
    pthread_mutex_unlock(&g_BigLock);
    Get_Clients(lobbyPort, 0);
    Get_Clients(playPort, 1);

    /*
    while(1) {
        pthread_mutex_lock(&g_BigLock);
        if(wordle.game_over) {
            pthread_mutex_unlock(&g_BigLock);
            break;
        }
        else {
            pthread_mutex_unlock(&g_BigLock);
        }
    }*/

    //keep running games
    while(1) {
        pthread_mutex_lock(&g_BigLock);
        if(wordle.game_over == 1) {
            char *word = word_to_guess();
            wordle.word = word;
            wordle.game_over = 0;
        }
        pthread_mutex_unlock(&g_BigLock);
        sleep(1);
    }
    printf("Ending game.....\n");
    sleep(15);
    return 0;
}   