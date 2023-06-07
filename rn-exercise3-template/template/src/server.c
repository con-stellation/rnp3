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
#include <malloc.h>
#include <errno.h>
#include <stdlib.h>

#define BUFFER_SIZE 256
#define SRV_PORT 7777
#define LIST 0
#define FILES 1
#define GET 2
#define PUT 3
#define QUIT 4
#define MAX_FILE_NAME 255
#define MAX_HOST_NAME 255
struct clientinformation{
    char *hostname;
    int socket;
};
void handle_request(int);
void rearrangeArray(int);
int client_count, maxfd;
struct clientinformation * connected_clients;
fd_set all_fds, copy_fds;


int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    connected_clients = malloc(5 * sizeof(struct clientinformation));
    int s_tcp, news, selectRet;
    char host[1024];
    char service[20];


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

    // char info[BUFFER_SIZE];
    char temp[INET6_ADDRSTRLEN];

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


                    FD_SET(news, &all_fds);
                    if (maxfd < news) {
                        printf("maxfd<news\n");
                        maxfd = news;
                    }
                    printf("selectserver: new connection from %s on socket %d\n",
                           inet_ntop(hints.ai_family, (struct sockaddr*)&sa_client, temp, INET6_ADDRSTRLEN), news);

                    getnameinfo((struct sockaddr*)&sa_client, sizeof sa_client, host, sizeof host, service, sizeof service, 0);
                    struct clientinformation ci;
                    //muss noch der Fall abgefangen werden, dass mehr als 5 Clients angemeldet sind oder ist dies durch das listen() erledigt?
                    ci.hostname = host;
                    ci.socket = news;
                    connected_clients[client_count] = ci;
                    client_count++;
                    printf("New socket = %d (comp. %d), host = %s\n", ci.socket, news, ci.hostname);

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

int read_command(int stream) {

  printf("reading command\n");
  char buffer[7] = {0};
  for(int i = 0; i < 6; i++) {
    recv(stream, &(buffer[i]), 1, 0);
    printf("command: %s\n", buffer);
    if(buffer[i] == ' ' || buffer[i] == '\n')
      break;
  }
  int command = 0;
  if(strcmp("List ", buffer) == 0) {
    command = 0;
  }else if(strcmp("Files ", buffer) == 0) {
    command = 1;
  }else if(strcmp("Get ", buffer) == 0) {
    command = 2;
  }else if(strcmp("Put ", buffer) == 0) {
    command = 3;
  }else if(strcmp("Quit ", buffer) == 0) {
    command = 4;
  }
  return command;
}

void read_filename(char *buffer, int buffer_size, int stream) {
  int size = 0;
  bool done = false;
  while(size < buffer_size && !done) {
    recv(stream, buffer + size, 1,  0);
    printf("dbg: %d\n", (int) buffer[size]);
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
    char temp[17];
    char end_of_file = EOF;
    sprintf(temp, "Clientcount: %d\n%c", client_count, end_of_file);
    printf("Clientcount: %d\n", client_count);
    char str[6 * sizeof(struct clientinformation)] = {0};

    for (int i = 0; i < client_count; i++) {
        char s[2];
        sprintf(s, "%d", connected_clients[i].socket);

        char buf1[strlen(connected_clients[i].hostname) + strlen(s) + 1];
        sprintf(buf1, "%s : %d\n", connected_clients[i].hostname, connected_clients[i].socket);
        printf("%s", buf1);
        strcat(str, buf1);
        memset(buf1, 0, strlen(buf1));

    }

    printf("Clientinformationen versendet\n");
    strcat(str, temp);
    send(stream, str, strlen(str), 0);
    memset(str, 0, strlen(str));
}


void handle_files(int stream) {
  printf("files\n");
  send(stream, NULL, 0, 0);
}

void handle_get(int stream) {
  printf("get\n");
  char filename[MAX_FILE_NAME] = {0};
  read_filename(filename, MAX_FILE_NAME, stream);
  printf("filename: %s\n", filename);
  int file = open(filename, O_RDONLY);
  if(file == -1) {
    switch(errno) {
      case EACCES:
        printf("Access error\n");
        break;
      case EFAULT:
        printf("fault\n");
        break;
      case EINVAL:
        printf("unsupported characters in pathname\n");
        break;
      case ENOENT:
        printf("file does not exist\n");
        break;
    }
    printf("error\n");
    exit(1);
  }
  int bytes;
  do {
    bytes = sendfile(stream, file, NULL, 1);
    //printf("%d bytes send\n", bytes);
  } while(bytes > 0);
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
  FILE * file = fopen(filename, "w");
  if(file == NULL) {
    printf("File could not be opened");
    exit(1);
  }
  int bytes = 1;
  char buf[2] = {0};
  char* str = NULL;
    int str_size = 0;
    int str_capacity = 0;

    do {
        bytes = recv(stream, buf, 1, 0);
        if(bytes == -1) {
            printf("error\n");
            exit(1);
        }
        printf("%d\n", (int) buf[0]);
        if(buf[0] == EOF){
            break;
        }
        //printf("%c", buf[0]);
        if (str_size >= str_capacity - 1) {
            // Den Speicher für str erweitern
            str_capacity += 1;
            char* new_str = realloc(str, str_capacity);
            if (new_str == NULL) {
                printf("Fehler: Speicher konnte nicht erweitert werden");
                exit(1);
            }
            str = new_str;
            printf("Neuer Zuwachs: %s\n", str);
            strcat(str, &buf[0]);
        }
        //strcat(str, &buf[0]);

        if(fprintf(file,"%c", buf[0]) == -1) {
            printf("error\n");
            exit(1);
        }
        memset(buf, 0, 2);
    } while(bytes > 0);
    printf("Content: %s", str);
    fprintf(file,"%s", str);
    free(str);

    if(bytes == -1) {
        switch(errno) {
        case EAGAIN:
            printf("Nonblocking\n");
            break;
        case EBADF:
            printf("Missing flag\n");
            break;
        case EFAULT:
            printf("Bad address\n");
            break;
        case EINVAL:
            printf("Invalid file desciptor\n");
            break;
        case EIO:
            printf("some IO error");
            break;
        case ENOMEM:
            printf("out of memory\n");
            break;
        case EOVERFLOW:
            printf("overflow\n");
            break;
        default:
            printf("unknown error\n");
            break;
        }
        exit(1);
    }
    fclose(file);
    //printf("closed file: %d\n", cls);
    char hostname[MAX_HOST_NAME];
    if(gethostname(hostname, MAX_HOST_NAME) != 0) {
        printf("cant get hostname\n");
        exit(1);
    }
    char line[MAX_FILE_NAME + 4];
    sprintf(line, "OK %s\n", hostname);
    send(stream, line, strlen(line), 0);
    char* ip = "0.0.0.0\n";
    send(stream, ip, strlen(ip), 0);
    char* time = "1.1.1970:00:00";
    send(stream, time, strlen(time), 0);
    char n = '\0';
    send(stream, &n, 1, 0);
}

void handle_quit(int stream) {

  //nicht nötig durch FD_SET? S42 im Handbuch
  close(stream);
  FD_CLR(stream, &all_fds);
  FD_CLR(stream, &copy_fds);
  for(int i=0; i<client_count; i++){
      if(connected_clients[i].socket == stream){
          if(i!=(client_count-1)){
              rearrangeArray(i);
          }
      }
  }

  --client_count;
  printf("quit\n");
}

void rearrangeArray(int index){
    for(int i=index; i<(client_count-1); i++){
        connected_clients[i] = connected_clients[i+1];
    }
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
