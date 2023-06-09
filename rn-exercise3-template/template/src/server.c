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
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>

#define BUFFER_SIZE 256
#define SRV_PORT 7777
#define LIST 0
#define FILES 1
#define GET 2
#define PUT 3
#define QUIT 4
#define MAX_FILE_NAME 255
#define MAX_PATH_LENGTH 4096

struct clientinformation{
    char hostname[NI_MAXHOST];
    int socket;
    char port[NI_MAXHOST];
};
void handle_request(int, struct sockaddr_storage, socklen_t);
void rearrangeArray(int);
void handle_quit(int);
int client_count, maxfd;
struct clientinformation connected_clients[5 * sizeof(struct clientinformation)];
fd_set all_fds, copy_fds;

void *get_in_addr(struct sockaddr *sa){
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    } else if(sa->sa_family == AF_INET6){
        return &(((struct sockaddr_in6*)sa)->sin6_addr);
    }
  return NULL;    
}

int main(void) {
    
    int s_tcp, news, selectRetVal;
    char host[NI_MAXHOST];
    char service[NI_MAXHOST];

    //IP-neutralität gewähren
    int status, yes = 1;
    struct addrinfo hints, *info, *p;
    struct sockaddr_storage sa_client;
    socklen_t  sa_len;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = IPPROTO_TCP;

    char port[6];
    sprintf(port, "%d", SRV_PORT);

    if ((status = getaddrinfo(NULL, port, &hints, &info)) != 0) {
        perror("getaddrinfo");
        exit(1);
    }

    char temp[INET6_ADDRSTRLEN];

    // loop through all the results and bind to the first we can
    for(p = info; p != NULL; p = p->ai_next) {
        if ((s_tcp = socket(p->ai_family, p->ai_socktype,
        p->ai_protocol)) == -1) {
        perror("server: socket");
        continue;
        }

        if (setsockopt(s_tcp, SOL_SOCKET, SO_REUSEADDR, &yes,sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
        }

        if (bind(s_tcp, p->ai_addr, p->ai_addrlen) == -1) {
        close(s_tcp);
        perror("server: bind");
        continue;
        }

        break;
    }

    freeaddrinfo(info);

    if(p == NULL){
        fprintf(stderr, "Server: failed to bind.\n");
        return 1;
    }

    if (listen(s_tcp, 5) < 0) {
        perror("listen");
        close(s_tcp);
        return 1;
    }

    //initialize fd-sets
    FD_ZERO(&all_fds);
    FD_ZERO(&copy_fds);
    FD_SET(s_tcp, &all_fds);

    //set biggest filedesciptor known until now
    maxfd = s_tcp;

    printf("Waiting for TCP connections on s_tcp = %d...\n", s_tcp);

    //superloop
    while (1) {
        //get next event
        copy_fds = all_fds;
        selectRetVal = select(maxfd + 1, &copy_fds, NULL, NULL, NULL);

        //error handling
        if (selectRetVal < 0) {
            perror("select");
            close(s_tcp);
            return 1;
        } else if (selectRetVal == 0) {
            printf("Nothing...\n");
        }

        //see which fd is ready
        for (int i = 0; i <= maxfd; i++) {
            if (FD_ISSET(i, &copy_fds)) {
                //new connection
                if (i == s_tcp) {
                    sa_len = sizeof sa_client;
                    if ((news = accept(s_tcp, (struct sockaddr*)&sa_client, &sa_len)) < 0) {
                        perror("accept");
                        close(s_tcp);
                        return 1;
                    }
                    //add new fd to original set
                    FD_SET(news, &all_fds);
                    //set new known maximum
                    if (maxfd < news) {
                        maxfd = news;
                    }
                    printf("selectserver: new connection from %s on socket %d\n",
                           inet_ntop(sa_client.ss_family, get_in_addr((struct sockaddr*)&sa_client), temp, INET6_ADDRSTRLEN), news);

                    //collect information on new client and store it
                    getnameinfo((struct sockaddr*)&sa_client, sizeof sa_client, host, sizeof host, service, sizeof service, 0);
                    struct clientinformation ci;
                    for(int i=0; i<NI_MAXHOST; i++){
                      ci.hostname[i] = host[i];
                    }
                    
                    ci.socket = news;
                    for(int i=0; i<NI_MAXHOST; i++){
                      ci.port[i] = service[i];
                    }
                    
                    connected_clients[client_count] = ci;
                    client_count++;
                    printf("New socket = %d, host = %s, port = %s\n", ci.socket, ci.hostname, ci.port);
                } else {
                    handle_request(i, sa_client, sa_len);
                }
            }
        }
    }
    freeaddrinfo(info);
    close(s_tcp);
    return 0;
}

void get_time(char *buffer, int size) {
  time_t t;
  struct tm *info;
  time(&t);
  info = localtime(&t);
  strftime(buffer, size, "%Y-%m-%d %H:%M:%S", info);
}

int read_command(int stream) {
  printf("reading command\n");
  char buffer[7] = {0};

  for(int i = 0; i < 6; i++) {
    ssize_t bytes = recv(stream, &(buffer[i]), 1, 0);
    if(bytes <= 0){
        handle_quit(stream);
        printf("Disconnected\n");
        break;
    } else {
        if(buffer[i] == ' ' || buffer[i] == '\n')
        break;
    }

  }
  int command = -1;
  if(strcmp("List ", buffer) == 0) {
    command = LIST;
  }else if(strcmp("Files ", buffer) == 0) {
    command = FILES;
  }else if(strcmp("Get ", buffer) == 0) {
    command = GET;
  }else if(strcmp("Put ", buffer) == 0) {
    command = PUT;
  }else if(strcmp("Quit ", buffer) == 0) {
    command = QUIT;
  }
  return command;
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
    size++;
  }
}

void handle_list(int stream) {
    char temp[100] = {0};
    char end_of_file = EOF;
    sprintf(temp, "Clientcount: %d\n%c", client_count, end_of_file);
    char s[3*NI_MAXHOST] = {0};
    char str[7 * (sizeof(struct clientinformation))] = {0};

    for (int i = 0; i < client_count; i++) {
      sprintf(s, "%s : %s\n", connected_clients[i].hostname, connected_clients[i].socket);
      strcat(str, s);   
    }

    strcat(str, temp);
    send(stream, str, strlen(str), 0);
    memset(str, 0, sizeof str);
    memset(temp, 0, sizeof temp);
}


void handle_files(int stream) {
  char *path = "./";
  DIR *dir = opendir(path);
  if(dir == NULL) {
    char n = EOF;
    send(stream, &n, 1, 0);
    printf("cant open path");
    return;
  }

  int buffer_size = 1024; //should be enough memory
  char buffer[buffer_size]; 
  memset(buffer, 0, buffer_size);
  struct dirent* entry;
  struct stat attributes;
  int amount_of_files = 0;

  //iterate though the directory
  while ((entry = readdir(dir)) != NULL) {
    char entryPath[MAX_PATH_LENGTH];
    sprintf(entryPath, "%s/%s", path, entry->d_name);
    stat(entryPath, &attributes);

    if (S_ISREG(attributes.st_mode) /*is file*/) {
      char *name = entry->d_name;
      char *time = ctime(&attributes.st_mtime);
      long long size = (long long) attributes.st_size;
      sprintf(buffer, "Datei-Attribute: %s Last Modified, %s Size, %lld bytes\n", name, time, size);
      send(stream, buffer, strlen(buffer), 0);
      memset(buffer, 0, buffer_size);
      amount_of_files++;
    }
  }
  sprintf(buffer, "%d Dateien\n", amount_of_files);
  send(stream, buffer, strlen(buffer), 0);
  closedir(dir);
  char n = EOF;
  send(stream, &n, 1, 0);
}

void handle_get(int stream) {
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
    return;
  }
  int bytes;
  do {
    bytes = sendfile(stream, file, NULL, 1);
  } while(bytes > 0);
  if(bytes<0){
      printf("Server: Error sending file. Line: %d", __LINE__);
      return;
  }

  int cls = close(file);
  printf("closed file: %d\n", cls); //log if file was closed
  char n = '\0';
  send(stream, &n, 1, 0);
}

void handle_put(int stream, struct sockaddr_storage socket, socklen_t socket_length) {
    char filename[MAX_FILE_NAME] = {0};
    read_filename(filename, MAX_FILE_NAME, stream);
    printf("%s\n", filename);
    FILE * file = fopen(filename, "w");
    if(file == NULL) {
      printf("File could not be opened");
      return;
    }

    //read file content
    int bytes = 1;
    char buf[2] = {0};
    do {
        bytes = recv(stream, buf, 1, 0);
        if(bytes == -1) {
            printf("error\n");
            exit(1);
        }
        printf("buf %c\n", buf[0]);
        if(buf[0] == EOF){
            printf("End of file bekommen. \n");
            break;
        }
        if(fprintf(file,"%c", buf[0]) == -1) {
            printf("error\n");
            exit(1);
        }
        memset(buf, 0, 2);
    } while(bytes > 0);

    memset(filename, 0, sizeof filename);
    if(bytes <= -1) {
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

    //send response
    char hostname[NI_MAXHOST];
    if(gethostname(hostname, NI_MAXHOST) != 0) {
      printf("cant get hostname\n");
      exit(1);
    }
    char line[NI_MAXHOST + 4];
    char host[NI_MAXHOST];
    char port[NI_MAXHOST];
    if(getnameinfo((struct sockaddr *)&socket, socket_length, host, NI_MAXHOST, port, NI_MAXHOST, 0) != 0) {
      printf("error in getnameinfo\n");
      exit(1);
    }
    sprintf(line, "OK %s\n", hostname);
    send(stream, line, strlen(line), 0);
    send(stream, port, strlen(port), 0);
    printf("Host: %s, Port: %s", host, port);
    char new_line = '\n';
    send(stream, &new_line, 1, 0);
    char time[20] = {0};
    get_time(time, 20);
    send(stream, time, strlen(time), 0);
    char n = '\0';
    send(stream, &n, 1, 0);
}

void handle_quit(int stream) {
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
}

//move everything 1 to the left
void rearrangeArray(int index){
    for(int i=index; i<(client_count-1); i++){
        connected_clients[i] = connected_clients[i+1];
    }
}

void handle_request(int stream, struct sockaddr_storage socket, socklen_t socket_length) {
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
      handle_put(stream, socket, socket_length);
      break;
    case QUIT:
      handle_quit(stream);
      break;
    default:
      printf("not a valid command\n");
      break;
  }
  printf("done\n");
}
