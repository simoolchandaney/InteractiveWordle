 
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
#include "../cJSON.h"

#define BUFFER_MAX 1000


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void send_data(int sockfd, char *data) {
    if ((send(sockfd, data, strlen(data), 0)) == -1) {
        perror("recv");
        exit(1);  
    }
}

char *receive_data(int sockfd) {
    char szBuffer[BUFFER_MAX];
    int numBytes;
    if ((numBytes = recv(sockfd, szBuffer, BUFFER_MAX - 1, 0)) == -1) {
			perror("recv");
			exit(1);
		}
    szBuffer[numBytes] = '\0';

    return szBuffer;
}

cJSON *get_message(char *message_type, char *contents[], char *fields[]) {
    cJSON *message = cJSON_CreateObject();
    cJSON_AddStringToObject(message, "MessageType", message_type);
    cJSON *data = cJSON_CreateArray();
    
    for(int i = 0; i < sizeof(contents); i++) {
        cJSON *field = cJSON_CreateString(fields[i]);
        cJSON_AddItemToObject(data, contents[i], field);
    }

    cJSON_AddItemToObject(message, "Data", data);

    return message;
}

void interpret_message(cJSON *message) {
    char *message_type = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(message, "MessageType"));
    
    if(!strcmp(message_type, "JoinResult")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        if(cJSON_GetArraySize(data) == 2) {
            char *name = cJSON_GetStringValue(cJSON_GetArrayItem(data, 0));
            char *result = cJSON_GetStringValue(cJSON_GetArrayItem(data, 1));
            //TODO JOINRESULT
        }
        else {
            //TODO ERROR
        }
    }
    else if(!strcmp(message_type, "Chat")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        if(cJSON_GetArraySize(data) == 2) {
            char *name = cJSON_GetStringValue(cJSON_GetArrayItem(data, 0));
            char *text = cJSON_GetStringValue(cJSON_GetArrayItem(data, 1));
            //TODO CHAT
        }
        else {
            //TODO ERROR
        }
    
    }
    else if(!strcmp(message_type, "StartInstance")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        if(cJSON_GetArraySize(data) == 3) {
            char *server = cJSON_GetStringValue(cJSON_GetArrayItem(data, 0));
            char *port = cJSON_GetStringValue(cJSON_GetArrayItem(data, 1));
            char *nonce = cJSON_GetStringValue(cJSON_GetArrayItem(data, 2));
			//TODO STARTINSTANCE
        }
        else {
            //TODO ERROR
        }
    }
    else if(!strcmp(message_type, "JoinInstanceResult")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        if(cJSON_GetArraySize(data) == 3) {
            char *name = cJSON_GetStringValue(cJSON_GetArrayItem(data, 0));
            char *number = cJSON_GetStringValue(cJSON_GetArrayItem(data, 1));
            char *result = cJSON_GetStringValue(cJSON_GetArrayItem(data, 2));
		    //TODO JOININSTANCERESULT
        }
        else {
            //TODO ERROR
        }
    
    }
	else if(!strcmp(message_type, "StartGame")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        if(cJSON_GetArraySize(data) == 2) {
            char *rounds = cJSON_GetStringValue(cJSON_GetArrayItem(data, 0));
            int array_size = cJSON_GetArraySize(cJSON_GetArrayItem(data, 1));
			char *names[array_size];
			char *numbers[array_size];

			cJSON *entry;
			int i = 0;
			cJSON_ArrayForEach(entry, cJSON_GetArrayItem(data, 1)) {
				strcpy(names[i], cJSON_GetStringValue(cJSON_GetObjectItem(entry, "Name")));
				strcpy(numbers[i], cJSON_GetStringValue(cJSON_GetObjectItem(entry, "Number")));
				i++;
			}

		    //TODO JOININSTANCERESULT
        }
        else {
            //TODO ERROR
        }
    
    }
	else if(!strcmp(message_type, "StartRound")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        if(cJSON_GetArraySize(data) == 4) {
            char *word_length = cJSON_GetStringValue(cJSON_GetArrayItem(data, 0));
            char *round = cJSON_GetStringValue(cJSON_GetArrayItem(data, 1));
            char *rounds_remaining = cJSON_GetStringValue(cJSON_GetArrayItem(data, 2));
			int array_size = cJSON_GetArraySize(cJSON_GetArrayItem(data, 3));
			char *names[array_size];
			char *numbers[array_size];
			char *scores[array_size];
			cJSON *entry;
			int i = 0;
			cJSON_ArrayForEach(entry, cJSON_GetArrayItem(data, 3)) {
				strcpy(names[i], cJSON_GetStringValue(cJSON_GetObjectItem(entry, "Name")));
				strcpy(numbers[i], cJSON_GetStringValue(cJSON_GetObjectItem(entry, "Number")));
				strcpy(scores[i], cJSON_GetStringValue(cJSON_GetObjectItem(entry, "Score")));
				i++;
			}
		    //TODO STARTROUND
        }
        else {
            //TODO ERROR
        }
    
    }
	else if(!strcmp(message_type, "PromptForGuess")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        if(cJSON_GetArraySize(data) == 3) {
            char *word_length = cJSON_GetStringValue(cJSON_GetArrayItem(data, 0));
            char *name = cJSON_GetStringValue(cJSON_GetArrayItem(data, 1));
            char *guess_number = cJSON_GetStringValue(cJSON_GetArrayItem(data, 2));
		    //TODO PROMPTFORGUESS
        }
        else {
            //TODO ERROR
        }
    
    }
	else if(!strcmp(message_type, "GuessResponse")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        if(cJSON_GetArraySize(data) == 3) {
            char *name = cJSON_GetStringValue(cJSON_GetArrayItem(data, 0));
            char *guess = cJSON_GetStringValue(cJSON_GetArrayItem(data, 1));
            char *accepted = cJSON_GetStringValue(cJSON_GetArrayItem(data, 2));
		    //TODO GUESSRESPONSE
        }
        else {
            //TODO ERROR
        }
    
    }
	else if(!strcmp(message_type, "GuessResult")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        if(cJSON_GetArraySize(data) == 2) {
            char *name = cJSON_GetStringValue(cJSON_GetArrayItem(data, 0));
            int array_size = cJSON_GetArraySize(cJSON_GetArrayItem(data, 1));
			char *names[array_size];
			char *numbers[array_size];
			char *corrects[array_size];
			char *receipt_times[array_size];
			char *results[array_size];
			cJSON *entry;
			int i = 0;
			cJSON_ArrayForEach(entry, cJSON_GetArrayItem(data, 1)) {
				strcpy(names[i], cJSON_GetStringValue(cJSON_GetObjectItem(entry, "Name")));
				strcpy(numbers[i], cJSON_GetStringValue(cJSON_GetObjectItem(entry, "Number")));
				strcpy(corrects[i], cJSON_GetStringValue(cJSON_GetObjectItem(entry, "Correct")));
				strcpy(receipt_times[i], cJSON_GetStringValue(cJSON_GetObjectItem(entry, "ReceiptTime")));
				strcpy(results[i], cJSON_GetStringValue(cJSON_GetObjectItem(entry, "Result")));
				i++;
			}
		    //TODO GUESSRESULT
        }
        else {
            //TODO ERROR
        }
    
    }

	else if(!strcmp(message_type, "EndRound")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        if(cJSON_GetArraySize(data) == 2) {
            char *rounds_remaining = cJSON_GetStringValue(cJSON_GetArrayItem(data, 0));
            int array_size = cJSON_GetArraySize(cJSON_GetArrayItem(data, 1));
			char *names[array_size];
			char *numbers[array_size];
			char *scores_earned[array_size];
			char *winners[array_size];

			cJSON *entry;
			int i = 0;
			cJSON_ArrayForEach(entry, cJSON_GetArrayItem(data, 1)) {
				strcpy(names[i], cJSON_GetStringValue(cJSON_GetObjectItem(entry, "Name")));
				strcpy(numbers[i], cJSON_GetStringValue(cJSON_GetObjectItem(entry, "Number")));
				strcpy(scores_earned[i], cJSON_GetStringValue(cJSON_GetObjectItem(entry, "ScoreEarned")));
				strcpy(winners[i], cJSON_GetStringValue(cJSON_GetObjectItem(entry, "Winner")));
				i++;
			}
		    //TODO ENDROUND
        }
        else {
            //TODO ERROR
        }
    
    }
	//TODO
	else if(!strcmp(message_type, "EndGame")) {
        cJSON *data = cJSON_GetObjectItemCaseSensitive(message, "Data");
        if(cJSON_GetArraySize(data) == 2) {
            char *winner_name = cJSON_GetStringValue(cJSON_GetArrayItem(data, 0));
            int array_size = cJSON_GetArraySize(cJSON_GetArrayItem(data, 1));
			char *names[array_size];
			char *numbers[array_size];
			char *scores[array_size];

			cJSON *entry;
			int i = 0;
			cJSON_ArrayForEach(entry, cJSON_GetArrayItem(data, 1)) {
				strcpy(names[i], cJSON_GetStringValue(cJSON_GetObjectItem(entry, "Name")));
				strcpy(numbers[i], cJSON_GetStringValue(cJSON_GetObjectItem(entry, "Number")));
				strcpy(scores[i], cJSON_GetStringValue(cJSON_GetObjectItem(entry, "Score")));
				i++;
			}
		    //TODO ENDGAME
        }
        else {
            //TODO ERROR
        }
    
    }
    else {
        //TODO ERROR
    }
}




int main(int argc, char *argv[]) 
{

    char player_name[BUFSIZ];
    char server_name[BUFSIZ];
    char lobby_port[BUFSIZ];
    char game_port[BUFSIZ];
    char nonce[BUFSIZ];


    for(int i = 1; i < argc; i++) {

        if(!strcmp(argv[i], "-name")) {
            strcpy(player_name, argv[i+1]);
        }
        else if(!strcmp(argv[i], "-server")) {
            strcpy(server_name, argv[i+1]);
        }
        else if(!strcmp(argv[i], "-port")) {
            strcpy(lobby_port, argv[i+1]);
        }
        else if(!strcmp(argv[i], "-game")) {
            strcpy(game_port, argv[i+1]);
        }
        else if(!strcmp(argv[i], "-nonce")) {
            strcpy(nonce, argv[i+1]);
        }
        else continue;
    }
	
    
    char *contents[2] = {"Name", "Client"};
    char *fields[2] = {player_name, "ISJ-C"};
    char *join = cJSON_Print(get_message("Join", contents, fields));
    uint16_t join_size = strlen(join);

    int sockfd;
    struct addrinfo hints, *servinfo, *p;
	struct sockaddr_in *h;
    int rv;
    char s[INET6_ADDRSTRLEN];
	char ip[100];


    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(server_name, lobby_port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {

        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
    printf("client: connecting to %s\n", s);
    freeaddrinfo(servinfo); // all done with this structure


    send_data(sockfd, join);

    char *data = receive_data(sockfd);

    
    printf("received: %s\n", data);
    
	close(sockfd);

	return 0;
}	


