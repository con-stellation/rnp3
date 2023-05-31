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
  ssize_t n = 0;

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
    char buffer[256];
    printf("enter msg\n");
    fgets(buffer, sizeof(buffer), stdin);
    if(buffer[0] == '\\' && buffer[1] == '0') {
      buffer[0] = (char) 0;
      send(s_tcp, buffer, 1, 0);
      break;
    }

    if ((n = send(s_tcp, buffer, strlen(buffer), 0)) > 0) {
      printf("Message %s sent (%zi Bytes).\n", buffer, n);
    }
  }

  close(s_tcp);
}