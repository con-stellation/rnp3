#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <stdbool.h>
#include <sys/sendfile.h>
#include <fcntl.h>

#define BUFFER_SIZE 256
#define SRV_PORT 7777

void handle_request(int);
int client_count;
int* connected_clients;

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    int s_tcp, news, maxfd, selectRet;
    fd_set all_fds, copy_fds;

    //IP-neutralität gewähren
    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo;
    struct sockaddr_storage sa_client;
    socklen_t  sa_len;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = IPPROTO_TCP;

    char port[6];
    sprintf(port, "%d", SRV_PORT);

    if ((status = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
        perror("getaddrinfo");
        return 1;
    }

    char info[BUFFER_SIZE], temp[INET6_ADDRSTRLEN];

/*    sa.sin_family = AF_INET;
    sa.sin_port = htons(SRV_PORT);
    sa.sin_addr.s_addr = INADDR_ANY;*/

    if ((s_tcp = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) < 0) {
        perror("TCP Socket");
        return 1;
    }

    if (bind(s_tcp, servinfo->ai_addr, servinfo->ai_addrlen) < 0) {
        perror("bind");
        return 1;
    }

    if (listen(s_tcp, 5) < 0) {
        perror("listen");
        close(s_tcp);
        return 1;
    }

    FD_ZERO(&all_fds);
    FD_ZERO(&copy_fds);
    FD_SET(s_tcp, &all_fds);
    maxfd = s_tcp;

    printf("Waiting for TCP connections on s_tcp = %d...\n", s_tcp);

    while (1) {
        copy_fds = all_fds;
        printf("Select blocking\n");
        selectRet = select(maxfd + 1, &copy_fds, NULL, NULL, NULL);
        printf("Select free\n");

        if (selectRet < 0) {
            perror("select");
            close(s_tcp);
            return 1;
        } else if (selectRet == 0) {
            printf("Nix neues...\n");
        }

        for (int i = 0; i <= maxfd; i++) {
            if (FD_ISSET(i, &copy_fds)) {
                printf("FD_ISSET i=%d\n", i);
                if (i == s_tcp) {
                    sa_len = sizeof sa_client;
                    if ((news = accept(s_tcp, (struct sockaddr*)&sa_client, &sa_len)) < 0) {
                        perror("accept");
                        close(s_tcp);
                        return 1;
                    }
                    connected_clients[client_count] = news;
                    client_count++;

                    FD_SET(news, &all_fds);
                    if (maxfd < news) {
                        printf("maxfd<news\n");
                        maxfd = news;
                    }
                    printf("selectserver: new connection from %s on socket %d\n",
                           inet_ntop(hints.ai_family, (struct sockaddr*)&sa_client, temp, INET6_ADDRSTRLEN), news);
                    printf("New socket = %d\n", news);
                } else {
                    printf("<else-block> i=%d\n", i);
                    handle_request(i);
                    /*if (recv(i, info, sizeof(info), 0)) {
                        printf("Message received: %s\n", info);
                        memset(info, 0, sizeof(info));

                    }*/
                    printf("Nach receive\n");
                }
            }
        }
    }
    freeaddrinfo(servinfo);
    close(s_tcp);
    return 0;
}

#define LIST 0
#define FILES 1
#define GET 2
#define PUT 3
#define QUIT 4
#define MAX_FILE_NAME 255

int read_command(int stream) {
  uint8_t buffer;
  recv(stream, &buffer, sizeof(buffer), 0);
  return (int )buffer;
}

void read_filename(char *buffer, int buffer_size, int stream) {
  int size = 0;
  bool done = false;
  while(size < buffer_size && !done) {
    recv(stream, buffer + size, 1,  0);
    if(buffer[size] == '\0' || buffer[size] == '\n') {
      buffer[size] = '\0';
      done = true;
    }
    //printf("%d:%s\n", size, buffer);
    size++;
  }
}

void handle_list(int stream) {
  printf("list\n");
}

void handle_files(int stream) {
  printf("files\n");
}

void handle_get(int stream) {
  printf("get\n");
  char filename[MAX_FILE_NAME] = {0};
  read_filename(filename, MAX_FILE_NAME, stream);
  printf("filename: %s\n", filename);
  int file = open(filename, O_RDONLY);
  int bytes = sendfile(stream, file, NULL, 1);
  //printf("%d bytes send\n", bytes);
  while(bytes > 0) {
    bytes = sendfile(stream, file, NULL, 1);
    //printf("%d bytes send\n", bytes);
  }
  char end_of_file = EOF;
  send(stream, &end_of_file, 1, 0);
  int cls = close(file);
  printf("closed file: %d\n", cls);
}

void handle_put(int stream) {
  printf("put\n");
  char filename[MAX_FILE_NAME] = {0};
  read_filename(filename, MAX_FILE_NAME, stream);
  printf("%s\n", filename);
}

void handle_quit(int stream) {
  for(int i=0; i<=client_count; i++){
      if(connected_clients[i] == stream){
          connected_clients[i] = -1;
      }
  } //nicht nötig durch FD_SET? S42 im Handbuch
  client_count--;
  close(stream);
  printf("quit\n");
}

void handle_error() {
  printf("error\n");
}

void handle_request(int stream) {
  int command = read_command(stream);
  switch(command) {
    case LIST:
      handle_list(stream);
      break;
    case FILES:
      handle_files(stream);
      break;
    case GET:
      handle_get(stream);
      break;
    case PUT:
      handle_put(stream);
      break;
    case QUIT:
      handle_quit(stream);
      break;
    default:
      handle_error();
      break;
  }
  printf("done\n");
}
