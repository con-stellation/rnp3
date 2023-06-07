#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

// TODO: Remove this block.
#define SRV_ADDRESS "127.0.0.1"
#define SRV_PORT 7777
#define LIST 0
#define FILES 1
#define GET 2
#define PUT 3
#define QUIT 4
#define MAX_FILE_NAME 255

void read_request(int);

void *get_in_addr(struct sockaddr *sa){
  if (sa->sa_family == AF_INET) {
  return &(((struct sockaddr_in*)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char** argv) {
  (void)argc;  // TODO: Remove cast and parse arguments.
  (void)argv;  // TODO: Remove cast and parse arguments.
  int s_tcp;
  struct sockaddr_in sa;
  unsigned int sa_len = sizeof(struct sockaddr_in);
  // ssize_t n = 0;

  sa.sin_family = AF_INET;
  sa.sin_port = htons(SRV_PORT);

  if (inet_pton(sa.sin_family, SRV_ADDRESS, &sa.sin_addr.s_addr) <= 0) {
    perror("Address Conversion");
    return 1;
  }

  if ((s_tcp = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    perror("TCP Socket");
    return 1;
  }

  if (connect(s_tcp, (struct sockaddr*)&sa, sa_len) < 0) {
    perror("Connect");
    return 1;
  }

  while(1) {
    read_request(s_tcp);
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
  return (int) command[0] - (int) '0';
}

int read_and_send_command(int stream) {
  int c = read_command();
  char* command = NULL;
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
  printf("%s: %d\n", command, size);
  send(stream, command, size, 0);
  return c;
}

void read_request(int stream) {
  int command = read_and_send_command(stream);

  if(command == 3 || command == 2) {
    printf("enter filename: \n");
    char buffer[MAX_FILE_NAME] = {0};
    char unused[2]; //flush the newline character from the console
    handle_error(
      fgets(unused, sizeof(unused), stdin));
    handle_error(
      fgets(buffer, sizeof(buffer), stdin));
    send(stream, buffer, strlen(buffer), 0);
  }
  if(command == 2) {
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
}
