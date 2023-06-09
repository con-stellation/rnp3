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
        printf("IPv4\n"); 
        return &(((struct sockaddr_in*)sa)->sin_addr);
        
    } else if(sa->sa_family == AF_INET6){
        printf("IPv6\n");
        return &(((struct sockaddr_in6*)sa)->sin6_addr);
    }
    return NULL;
}

int main(int argc, char** argv) {
    printf("starting\n");
    (void)argc;
    const char *restrict SRV_ADDRESS = argv[1];  // TODO: Remove cast and parse arguments.
    const char *restrict SRV_PORT = argv[2];
    printf("%s\n", SRV_ADDRESS);
    printf("%s\n", SRV_PORT);
    char buffer1[INET6_ADDRSTRLEN];
    int s_tcp;

    struct addrinfo hints, *servinfo, *p;
    int rv;

    memset(&hints, 0, sizeof hints);    //make sure struct is empty
    hints.ai_family = AF_UNSPEC;        // dont care Ipv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;    //TCP stream socket
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

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

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), buffer1, sizeof buffer1);
    printf("client connection with: %s\n", buffer1);

    freeaddrinfo(servinfo);

    printf("into superloop\n");
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
    printf("\nEnter Command:\n");
    printf("0) List\n");
    printf("1) Files\n");
    printf("2) Get\n");
    printf("3) Put\n");
    printf("4) Quit\n");
    char command[2];
    if(fgets(command, sizeof(command), stdin) == NULL) {
        printf("cant read command\n");
        exit(1);
    }
    //flush the newline character from the console
    char unused[2]; 
    if(fgets(unused, sizeof(unused), stdin) == NULL) {
        printf("cant read command\n");
        exit(1);
    }
    printf("Command: %c\n", command[0]);
    return (int) command[0] - (int) '0';
}

bool handle_get(int stream) {
    printf("enter filename: \n");
    char filename[MAX_FILE_NAME] = {0};
    if(fgets(filename, sizeof(filename), stdin) == NULL) {
        printf("cant read filename\n");
        exit(1);
    }
    char buffer[2] = {0};
    int bytes = 1;
    FILE* f = fopen(filename, "w");
    if(f == NULL){
        printf ("error in client: %d\n", __LINE__);
        return false;
    }
    if(send(stream, "Get ", 4, 0) == -1) {
        printf("cant send command\n");
        exit(1);
    }
    if(send(stream, filename, strlen(filename), 0) == -1) {
        printf("cant send filename\n");
        exit(1);
    }
    char n = '\0';
    if(send(stream, &n, 1, 0) == -1) {
        printf("cant send filename nullchar\n");
        exit(1);
    }
    do {
        bytes = recv(stream, buffer, 1, 0);
        if(buffer[0] == EOF)
            break;
        fprintf(f, "%c", stream);
        printf("%s", buffer);
    } while(bytes > 0);
    fclose(f);
    return false;
}

bool handle_put(int stream) {
    char* source = NULL;
    char filename[MAX_FILE_NAME] = {0};
    printf("enter filename: \n");
    handle_error(fgets(filename, sizeof(filename), stdin));

    for(unsigned long i = 0; i < strlen(filename); i++){
        if(filename[i] == '\n') {
            filename[i] = '\0';
            break;
        }
    }
    for(unsigned long int i = 0; i <  strlen(filename); i++)
        printf("??? %c\n", filename[i]);

    FILE* file = fopen(filename, "r");
    if(file == NULL){
        perror("Fileopen\n");
        printf("error in client: %d", __LINE__);
        return false;
    }
    char *command = "Put ";
    if(send(stream, command, strlen(command), 0) == -1) {
        perror("sending command\n");
        printf("error in client: %d", __LINE__);
        return false;
    }
    if(send(stream, filename, strlen(filename), 0) == -1) {
        perror("sending filename\n");
        printf("error in client: %d", __LINE__);
        return false;
    }
    char n = '\0';
    if(send(stream, &n, 1, 0) == -1) {
        perror("sending filename nullchar\n");
        printf("error in client: %d", __LINE__);
        return false;
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
    if(source != NULL){
        printf("Source: %s\n\n", source);
    }

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
    fclose(file);
    free(source); /* Don't forget to call free() later! */
    printf("\n");
    char byte;
    do {
        recv(stream, &byte, 1, 0);
        printf("%c", byte);
    } while(byte != '\0');
    printf("\n");
    return false;
}

bool handle_list(int stream) {
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

bool read_request(int stream) {
    int command = read_command();
    switch(command) {
        case GET:
            return handle_get(stream);
        case LIST:
            return handle_list(stream);
        case PUT: 
            return handle_put(stream);
        case QUIT:
            return true;
        default:
            printf("unknown command");
            return false;
    }
}
