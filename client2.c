#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdbool.h>
#include "../cJSON.h"
#include "sys/time.h"

bool debugFlag = false;

/* Global Defines */
#define BUFFER_MAX 1000
#define BACKLOG 10   // how many pending connections queue will hold

/* Global Variables */
char g_bKeepLooping = 1;
pthread_mutex_t     g_BigLock;

struct ClientInfo
{
    pthread_t threadClient;
    char szIdentifier[100];
    int  socketClient;
};

typedef struct Input {
    int numPlayers;
    char *lobbyPort;
    char *playPort;
    int numRounds;
    char *fileName;
} Input;

typedef struct Game_Player {
    char *name;
    char *client;
    int number;
    char *guess;
    char *score;
    char *correct;
    char *receipt_time;
    char *result;
    char *score_earned;
    char *winner;
} Game_Player;

typedef struct Lobby_Player {
    char *name;
    char *client;
} Lobby_Player;

typedef struct Wordle
{
    struct Lobby_Player lobbyPlayers[4];
    struct Game_Player players[4];
    char *word;
    int num_in_lobby;
    int num_players;
    int nonce;
    char *winner;
    struct Input inputs;
} Wordle;

char *chat_message;

Wordle wordle;

#define BACKLOG 10   // how many pending connections queue will hold
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
    char hostbuffer[256];
    char *IPbuffer;
    struct hostent *host_entry;
    int hostname;
  
    // To retrieve hostname
    hostname = gethostname(hostbuffer, sizeof(hostbuffer));
  
    // To retrieve host information
    host_entry = gethostbyname(hostbuffer);
  
    // To convert an Internet network
    // address into ASCII string
    IPbuffer = inet_ntoa(*((struct in_addr*)
                           host_entry->h_addr_list[0]));

    return IPbuffer;
}

void send_data(struct ClientInfo *pClient, char *response) {
    printf("sending: %s\n", response);
    if (send(pClient->socketClient, response, strlen(response), 0) == -1)
    {
        perror("send");     
    }
}

char *receive_data(struct ClientInfo *pClient) {
    int numBytes;
    char szBuffer[BUFFER_MAX];
    if ((numBytes = recv(pClient->socketClient, szBuffer, BUFFER_MAX-1, 0)) == -1) {
        perror("recv");
        exit(1);
    }
    szBuffer[numBytes] = '\0';

    char *return_val = malloc(sizeof(szBuffer));
    memcpy(return_val, szBuffer, numBytes);
    printf("received: %s\n", return_val);
    return return_val;
}

int get_nonce() {
    int r = rand() % 1000;
    wordle.nonce = r;
    return r;
}

// fucntion that selects random word from text file
char * word_to_guess() {
<<<<<<< HEAD
=======
    //return "wordle";
>>>>>>> 3ba660c477dcdb6eb5121bb09f2601f6786b0733
    int c;
    int wordCount = 0;
    FILE *file_handle = fopen (wordle.inputs.fileName, "r");
    while ((c = fgetc(file_handle)) != EOF){
       wordCount++; 
    }
    int i;
    char **words = (char**) calloc(wordCount, sizeof(char*));
    for (i = 0; i < wordCount; i++) {
        words[i] = (char*) calloc(11, sizeof(char));
        fscanf (file_handle, "%s", words[i]); 
    }
    int r = rand() % wordCount;
    char *result = NULL;
    strcpy(result, words[r]);
    for (i = 0; i < wordCount; ++i) {
        free (words[i]); 
    }
    free(words);
    return result;
}

char * word_guess_color_builder(char * guess, char * key) {
<<<<<<< HEAD
=======
    //return "";
>>>>>>> 3ba660c477dcdb6eb5121bb09f2601f6786b0733
    char *letters = (char*) malloc(strlen(guess)*sizeof(char));
    for (int l = 0; l < strlen(guess); l++) {
        letters[l] = 'B'; // grey
    }
    for (int i = 0; i < strlen(guess); i++) {
        for (int j = 0; j < strlen(key); j++) {
            if (guess[i] == key[j]) {
                letters[i] = 'Y'; // yellow 
            }
        }
    }
    for (int k = 0; k < strlen(guess); k++) {
        if (guess[k] == key[k]) {
            letters[k] = 'G'; // green 
        }
    }
    free(letters);
    return letters;
<<<<<<< HEAD
}

int check_profanity(char *guess) {
    char *profanity[] = {"fuck", "shit", "bitch", "damn", "pussy", "penis", "dick", "cock", "bastard", "asshole"};
    int i = 0;

    while(profanity[i]) {

        if(!strcmp(profanity[i], guess)) {
            return 1;
        }

        i++;
    }
    return 0;
=======
>>>>>>> 3ba660c477dcdb6eb5121bb09f2601f6786b0733
}

cJSON *get_message(char *message_type, char *contents[], char *fields[], int content_length) {
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
                cJSON_AddStringToObject(item, "Score", wordle.players[j].score);
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
                cJSON_AddStringToObject(item, "ScoreEarned", wordle.players[j].score_earned);
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
                cJSON_AddStringToObject(item, "Score", wordle.players[j].score);
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

void join(char *name, char *client, struct ClientInfo *pClient) {
    //make sure name is unique
    int unique  = 1;
    for(int i = 0; i < wordle.num_in_lobby; i++) {
        if (!strcmp(name, wordle.lobbyPlayers[i].name)) {
            unique = 0;
        }
    }
    char *fields[2] = {name, ""};
    if(!unique) {
        fields[1] = "No";
    }
    else {
        fields[1] = "Yes";
        Lobby_Player player;
        player.name = name;
        player.client = client;
        wordle.lobbyPlayers[wordle.num_in_lobby] = player;
        wordle.num_in_lobby += 1;
    }

    char *contents[2] = {"Name", "Result"};
    char *response = cJSON_Print(get_message("JoinResult", contents, fields, 2));
    send_data(pClient, response);
}

void chat(char *name, char *text, struct ClientInfo *pClient) {
}

void joinInstance(char *name, char *nonce, struct ClientInfo *pClient) {
    //check if name and nonce is valid
    int unique = 1;
    for(int i =0; i < wordle.num_players; i++) {
        if(!strcmp(name, wordle.players[i].name)) {
            unique = 0;
        }
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
        wordle.players[wordle.num_players] = player;
        wordle.num_players += 1;
    }

    char *contents[3] = {"Name", "Number", "Result"};
    char *response = cJSON_Print(get_message("JoinInstanceResult", contents, fields, 3));
    send_data(pClient, response);
}

void checkGuess(char *name, char *guess, struct ClientInfo *pClient) {
    printf("%s guessed %s\n", name, guess);
    //TODO send guessresponse

    //todo check if word is correct and update correct
    for(int i = 0; i < wordle.num_players; i++) {
        if(!strcmp(wordle.players[i].name, name)) {

            wordle.players[i].guess = guess;

            // if profanity is found, print warning
            check_profanity(guess);

            //if(correct word) -> set correct field
            //if(correct word) -> set wordle.winner
            if(!strcmp(guess, wordle.word)) {

                wordle.players[i].correct = "yes";
                wordle.winner = wordle.players[i].name;
            }

            // set receipt time
            struct timeval tv;
            gettimeofday(&tv, NULL);
            wordle.players[i].receipt_time = (char *) tv.tv_sec;

            // set result string - string containing BYG
            char *colored_word = word_guess_color_builder(guess, wordle.word);
            printf(colored_word);

        }
    }

    //todo change accepted based on something?
    char *contents[3] = {"Name", "Guess", "Accepted"};
    char *fields[3] = {name, guess, "Yes"};
    char *response = cJSON_Print(get_message("GuessResponse", contents, fields, 3));
    send_data(pClient, response);


}

char *interpret_message(cJSON *message, struct ClientInfo *pClient) {

    char * message_type = cJSON_GetStringValue(cJSON_GetObjectItem(message, "MessageType"));

    if(!strcmp(message_type, "Join")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        char *name = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Name"));
        char *client = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Client"));
        join(name, client, pClient); 

    }
    else if(!strcmp(message_type, "Chat")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        char *name = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Name"));
        char *text = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Text"));
        chat(name, text, pClient);
    }

    else if(!strcmp(message_type, "JoinInstance")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        char *name = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Name"));
        char *nonce = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Nonce"));
        joinInstance(name, nonce, pClient);
        return name;
    }

    else if(!strcmp(message_type, "Guess")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        char *name = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Name"));
        char *guess = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(data, "Guess"));
        checkGuess(name, guess, pClient);
    }
    else {
        //TODO ERROR
    }
    return "";
}

void * Thread_Client_Game (void * pData)
{
    
    struct ClientInfo * pClient;
    struct ClientInfo   threadClient;
    
    char *szBuffer;
    
    // Typecast to what we need 
    pClient = (struct ClientInfo *) pData;
    
    // Copy it over to a local instance 
    threadClient = *pClient;

    //receive join instance
    szBuffer = receive_data(pClient);
    //pthread_mutex_lock(&g_BigLock);
    char *name = interpret_message(cJSON_Parse(szBuffer), pClient);
    printf("NAME IS %s\n", name);
    free(szBuffer);
    //pthread_mutex_unlock(&g_BigLock);

    //loop until all players have joined
    while(wordle.num_players < 1) {
        printf("looping");
    }

    //send start game message
    char *contents[2] = {"Rounds", "PlayerInfo"};
    char buffer[2];
    sprintf(buffer, "%d", wordle.inputs.numRounds);
    char *fields[2] = {buffer, NULL};

    char *response = cJSON_Print(get_message("StartGame", contents, fields, 2));
    send_data(pClient, response);

    //send start round messages
    int num_round = 1;
    while(wordle.inputs.numRounds > 0) {
        char *word = word_to_guess();
        printf("word: %s with length of %d\n", word, strlen(word));
        char *contents[4] = {"WordLength", "Round", "RoundsRemaining", "PlayerInfo"};
        char word_length_s[2];
        sprintf(word_length_s, "%d", strlen(word));
        char round_s[2];
        sprintf(round_s, "%d", num_round);
        char rounds_remaining_s[2];
        sprintf(rounds_remaining_s, "%d", wordle.inputs.numRounds - 1);
        char *fields[4] = {"", "", "", NULL};
        fields[0] = word_length_s;
        fields[1] = round_s;
        fields[2] = rounds_remaining_s;
        char *response = cJSON_Print(get_message("StartRound", contents, fields, 4));
        send_data(pClient, response);
        sleep(1);
        int no_winners = 1;
        int guess_number = 1;
        while(no_winners) {
            //prompt for guess
            char *contents[3] = {"WordLength", "Name", "GuessNumber"};
            char guess_number_s[2];
            sprintf(guess_number_s, "%d", guess_number);
            char *fields[3]  = {"", name, ""};
            fields[0] = word_length_s;
            fields[2] = guess_number_s;
            char *response = cJSON_Print(get_message("PromptForGuess", contents, fields, 3));
            send_data(pClient, response);

            //await guess
            char *data = receive_data(pClient);
            interpret_message(cJSON_Parse(data), pClient);
            free(data);

            sleep(1);
            //TODO check if all guesses are received
            //send guess result
            char *contents2[2] = {"Winner", "PlayerInfo"};
            char *fields2[2] = {wordle.winner, NULL};
            response = cJSON_Print(get_message("GuessResult", contents2, fields2, 2));
            send_data(pClient, response);

            //TODO if there is a winner end round
            sleep(1);
            char *contents3[2] = {"RoundsRemaining", "PlayerInfo"};
            char *fields3[2] = {rounds_remaining_s, NULL};
            response = cJSON_Print(get_message("EndRound", contents3, fields3, 2));
            send_data(pClient, response);

            guess_number++;
            no_winners = 0;
        }

        wordle.inputs.numRounds--;
        num_round++;
    }

    //send end game since done with every round
    sleep(1);
    char *contents4[2] = {"WinnerName", "PlayerInfo"};
    char *winner_name;
    //TODO get winner_name
    char *fields4[2] = {"Jacob", NULL};
    response = cJSON_Print(get_message("EndGame", contents4, fields4, 2));
    send_data(pClient, response);

    return NULL;
}

void Game_Lobby(char *nGamePort) {
    // Adapting this from Beej's Guide
    
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;
    
    struct ClientInfo theClientInfo;
    int       nClientCount = 0; 
    wordle.num_players = 0;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, nGamePort, &hints, &servinfo)) != 0) {
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

    g_bKeepLooping = 1;
    while(g_bKeepLooping) 
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
        sprintf(theClientInfo.szIdentifier, "%s-%d", s, nClientCount);
        theClientInfo.socketClient = new_fd;
        nClientCount++;

        // From OS: Three Easy Pieces 
        //   https://pages.cs.wisc.edu/~remzi/OSTEP/threads-api.pdf 
        pthread_create(&(theClientInfo.threadClient), NULL, Thread_Client_Game, &theClientInfo);
        
        /*
        // Bail out when the third client connects after sleeping a bit
        if(nClientCount == 1)
        {
            g_bKeepLooping = 0;
            sleep(15);
        }*/       
    }
    close(sockfd);
}

void * Thread_Client (void * pData)
{
    
    struct ClientInfo * pClient;
    struct ClientInfo   threadClient;
    
    char *szBuffer;
    
    // Typecast to what we need 
    pClient = (struct ClientInfo *) pData;
    
    // Copy it over to a local instance 
    threadClient = *pClient;

    //get player info
    szBuffer = receive_data(pClient);
    pthread_mutex_lock(&g_BigLock);
    //interpret Join message and send JoinResult back
    interpret_message(cJSON_Parse(szBuffer), pClient);
    free(szBuffer);
    pthread_mutex_unlock(&g_BigLock);

    //once all players are in (1) for now, send startInstance to all
     char *contents[3] = {"Server", "Port", "Nonce"};
    char buffer[5];
    sprintf(buffer, "%d", get_nonce());
    char *IP = get_IP();
    char *fields[3] = {IP, wordle.inputs.playPort, buffer};

    cJSON *message = get_message("StartInstance", contents, fields, 3);
    sleep(1); //make sure message sends seperately
    send_data(pClient, cJSON_Print(message));
    Game_Lobby(wordle.inputs.playPort);

    /*
    while(g_bKeepLooping)
    {
        szBuffer = receive_data(pClient);

        // Debug / show what we got3
        //printf("Received a message of %d bytes from Client %s\n", numBytes, pClient->szIdentifier);
        //printf("   Message: %s\n", szBuffer);
        
        // Do something with it
                        
        
        // This is a pretty good time to lock a mutex
        pthread_mutex_lock(&g_BigLock);
        
        // Do something dangerous here that impacts shared information
        interpret_message(cJSON_Parse(szBuffer), pClient);
        free(szBuffer);
                
        // This is a pretty good time to unlock a mutex
        pthread_mutex_unlock(&g_BigLock);
    }

    char *contents[3] = {"Server", "Port", "Nonce"};
    //TODO get these fields as variables
    char *fields[3] = {"129.74.152.124", "41334", "1234"};

    char *message = get_message("StartInstance", contents, fields, 3);
    send_data(pClient, cJSON_Print(message));
    */
    return NULL;
}

void Server_Lobby (char *nLobbyPort)
{
    
    // Adapting this from Beej's Guide
    
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;
    
    struct ClientInfo theClientInfo;
    int       nClientCount = 0; 

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, nLobbyPort, &hints, &servinfo)) != 0) {
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

    while(g_bKeepLooping) 
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
        sprintf(theClientInfo.szIdentifier, "%s-%d", s, nClientCount);
        theClientInfo.socketClient = new_fd;
        nClientCount++;

        // From OS: Three Easy Pieces 
        //   https://pages.cs.wisc.edu/~remzi/OSTEP/threads-api.pdf 
        pthread_create(&(theClientInfo.threadClient), NULL, Thread_Client, &theClientInfo);
        
        // Bail out when the third client connects after sleeping a bit
        if(nClientCount == 1)
        {
            g_bKeepLooping = 0;
            sleep(15);
        }       
    }
    close(sockfd);
    
}

int main(int argc, char *argv[]) 
{   
    srand(time(NULL));
    int numPlayers = 1;
    char *lobbyPort = "41333";
    char *playPort = "41334";
    int numRounds = 1;
    FILE *DFile;
    DFile = fopen("../terms.txt", "r+");

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
            fclose(DFile);
            char fileName[BUFSIZ];
            sprintf(fileName, "../%s", argv[i+1]);
            wordle.inputs.fileName = fileName;

        }  
        else if (!strcmp(argv[i], "-dbg")) {
            debugFlag = true;
        }
        else if (!strcmp(argv[i], "-gameonly")) {
            // TODO: act as a game instance server only awaiting clients on port X (also ignore the provided nonce)
        }
    }
    pthread_mutex_init(&g_BigLock, NULL);
    wordle.num_in_lobby = 0;
    wordle.inputs.numPlayers = numPlayers;
    wordle.inputs.lobbyPort = lobbyPort;
    wordle.inputs.playPort = playPort;
    wordle.inputs.numRounds = numRounds;
    Server_Lobby(wordle.inputs.lobbyPort);

    printf("Sleeping before exiting\n");
    sleep(15);
    
    printf("And we are done\n");
    fclose(DFile);
    return 0;
}   