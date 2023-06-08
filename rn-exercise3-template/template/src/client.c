#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <netdb.h>

// TODO: Remove this block.
#define LIST 0
#define FILES 1
#define GET 2
#define PUT 3
#define QUIT 4
#define MAX_FILE_NAME 255

bool read_request(int);

void *get_in_addr(struct sockaddr *sa){
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char** argv) {
    const char *restrict SRV_ADDRESS = argv[0];  // TODO: Remove cast and parse arguments.
    const char *restrict SRV_PORT = argv[1];
    char buffer[INET6_ADDRSTRLEN];
    int s_tcp;

    struct addrinfo hints, *servinfo, *p;
    int rv;

    memset(&hints, 0, sizeof hints);    //make sure struct is empty
    hints.ai_family = AF_UNSPEC;        // dont care Ipv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;    //TCP stream socket
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_TCP;

    if ((rv = getaddrinfo(SRV_ADDRESS, SRV_PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(rv)); //TODO im Buch gucken
        return 1;
    }

    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((s_tcp = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) < 0) {
            perror("socket");
            continue;
        }

        if (connect(s_tcp, p->ai_addr, p->ai_addrlen) < 0) {
            close(s_tcp);
            perror("connect");
            continue;
        }
        break;
    }

    if(p==NULL){
        fprintf(stderr, "Client: failed to connect.\n");
        return 1;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), buffer, sizeof buffer);
    printf("client connection with: %s\n", buffer);

    freeaddrinfo(servinfo);



    while(1) {
        if(read_request(s_tcp))
            break;
        // char buffer[256];
        // printf("enter msg\n");
        // fgets(buffer, sizeof(buffer), stdin);
        // if(buffer[0] == '\\' && buffer[1] == '0') {
        //   buffer[0] = (char) 0;
        //   send(s_tcp, buffer, 1, 0);
        //   break;
        // }

        // if ((n = send(s_tcp, buffer, strlen(buffer), 0)) > 0) {
        //   printf("Message %s sent (%zi Bytes).\n", buffer, n);
        // }
    }

    close(s_tcp);
}

void handle_error(char* return_value) {
    if(return_value == NULL) {
        printf("Error\n");
        exit(1);
    }
}

int read_command() {
    printf("Enter Command:\n");
    printf("0) List\n");
    printf("1) Files\n");
    printf("2) Get\n");
    printf("3) Put\n");
    printf("4) Quit\n");
    char command[2];
    handle_error(
        fgets(command, sizeof(command), stdin));
    char unused[2]; //flush the newline character from the console
    handle_error(
        fgets(unused, sizeof(unused), stdin));
    printf("Command: %c\n", command[0]);
    return (int) command[0] - (int) '0';
}

int read_and_send_command(int stream) {
    int c = read_command();
    char* command = NULL;
    printf("c= %d\n", c);
    int size;
    switch(c) {
        case LIST:
            command = "List ";
            size = 5;
            break;
        case FILES:
            command = "Files ";
            size = 6;
            break;
        case GET:
            command = "Get ";
            size = 4;
            break;
        case PUT:
            command = "Put ";
            size = 4;
            break;
        case QUIT:
        default:
            command = "Quit ";
            size = 5;
            break;
    }
    printf("Command: %s Size: %d\n", command, size);
    send(stream, command, size, 0);
    return c;
}

bool read_request(int stream) {
    int command = read_and_send_command(stream);
    char filename[MAX_FILE_NAME] = {0};

    if(command == GET || command == PUT) {
        printf("enter filename: \n");
        // char unused[2]; //flush the newline character from the console
        // handle_error(
        //     fgets(unused, sizeof(unused), stdin));
        handle_error(
            fgets(filename, sizeof(filename), stdin));
        send(stream, filename, strlen(filename), 0);
    }
    if(command == GET) {
        char buffer[2] = {0};
        int bytes = 1;
        //printf("bytes received: %d\n", bytes);
        do {
            bytes = recv(stream, buffer, 1, 0);
            //printf("bytes received: %d\n", bytes);
            if(buffer[0] == EOF){
                break;
            }
            printf("%s", buffer);
        } while(bytes > 0);
    }
    if(command == LIST){
        char buf[2] = {0};
        int bytes = 1;
        do{
            bytes = recv(stream, buf, 1, 0);
            if(buf[0] == EOF){
                break;
            }
            printf("%c", buf[0]);
            memset(buf, 0, 2);
        } while(bytes > 0);
    }
    char* source = NULL;
    if(command == PUT) {
        for(unsigned long i = 0; i < strlen(filename); i++)
            if(filename[i] == '\n') {
                filename[i] = '\0';
                break;
            }
        for(unsigned long int i = 0; i <  strlen(filename); i++)
            printf("??? %c\n", filename[i]);


        FILE* file = fopen(filename, "r");
        if(file == NULL){
            perror("Fileopen\n");
            printf("Error Filename: %s\n", filename);
            return -1;
        }
        printf("Filename: %s\n", filename);
        if (fseek(file, 0L, SEEK_END) == 0) {
            /* Get the size of the file. */
            printf("Made it to fseek. Filename: %s\n", filename);
            long bufsize = ftell(file);
            if (bufsize == -1) {
                /* Error */
                perror("ftell");
            }

            /* Allocate our buffer to that size. */
            source = malloc(sizeof(char) * (bufsize + 1));

            /* Go back to the start of the file. */
            if (fseek(file, 0L, SEEK_SET) != 0) { /* Error */ }

            /* Read the entire file into memory. */
            size_t newLen = fread(source, sizeof(char), bufsize, file);
            if ( ferror( file ) != 0 ) {
                fputs("Error reading file", stderr);
            } else {
                source[newLen++] = EOF; /* Just to be safe. */
            }
        }
        printf("Source: %s\n\n", source);
        char filelines[1] = {0};
        int index = 0;
        while(1){
            if(source[index] == EOF){
                filelines[0] = source[index];
                send(stream, filelines, 1, 0);
                break;
            }
            printf("%c", source[index]);
            filelines[0] = source[index];
            send(stream, filelines, 1, 0);
            memset(filelines, 0, 1);
            index++;

        };
        filelines[0] = EOF;
        send(stream, filelines, 1, 0);
       // char eof[1] = {EOF};
       // send(stream, eof, sizeof eof, 0);
        fclose(file);
        free(source); /* Don't forget to call free() later! */
    }



    //printf("closed file: %d\n", cls);


    return(command == QUIT);
}