#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/select.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>

// TODO: Remove me.
#define SRV_PORT 7777

void *get_in_addr(struct sockaddr *sa)
{
 if (sa->sa_family == AF_INET) {
 return &(((struct sockaddr_in*)sa)->sin_addr);
}

 return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char** argv) {
  (void)argc;  // TODO: Remove cast and parse arguments.
  (void)argv;  // TODO: Remove cast and parse arguments.
  //int clientCount = 0;
  fd_set totalFDs, copyFDs;
  int clientSockets[5] = {0};
  int s_tcp = 0, news = 0, rv, temp;
  int yes = 1;
  struct sockaddr_storage sa_client;
  struct addrinfo hints, *ai, *p;
  unsigned int sa_len = sizeof(struct sockaddr_in);
  char info[256];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, (const char *) SRV_PORT, &hints, &ai)) != 0) {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }

    for(p = ai; p != NULL; p = p->ai_next) {
        s_tcp = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (s_tcp < 0) {
            continue;
        }
        setsockopt(s_tcp, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(s_tcp, p->ai_addr, p->ai_addrlen) < 0) {
            close(s_tcp);
            continue;
        }

        break;
    }
//freeaddrinfo
        //---------------------------------------------------------
  /*if ((s_tcp = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    perror("TCP Socket");
    return 1;
  }

  if (bind(s_tcp, (struct sockaddr*)&sa, sa_len) < 0) {
    perror("bind");
    return 1;
  }*/

  if (listen(s_tcp, 5) < 0) {
    perror("listen");
    close(s_tcp);
    return 1;
  }
  // TODO: Check port in use and print it.
  printf("Waiting for TCP connections ... \n");
  FD_ZERO(&totalFDs);
  FD_ZERO(&copyFDs);
  FD_SET(s_tcp, &totalFDs);

  while (1) {

    copyFDs = totalFDs;
    printf("connection success, socket fd is %d\n", news);
    temp = select(5, &copyFDs, NULL, NULL, NULL);
    printf("Done with select!\n");
    if(temp>0){

      for(int i=0; i<5; i++)
      {
        if(FD_ISSET(i, &totalFDs)){
          if(i == s_tcp){
            if ((news = accept(s_tcp, (struct sockaddr*)&sa_client, &sa_len)) < 0) {
            perror("accept");
            close(s_tcp);
            return 1;
          }
          FD_SET(news, &totalFDs);
        }  else {
            news = i;
          }
            if (clientSockets[i] == 0) {
              clientSockets[i] = news;
              //printf("New connection, socket fd is %d, ip is : %s, port : %d\n", news, inet_ntoa(hints.ai_addr), ntohs(hints.ai_addrlen));
                printf("New connection, socket fd is %d\n", news);
              break;
            } else {
              //printf("Old connection, socket fd is %d, ip is : %s, port : %d\n", news, inet_ntoa(sa.sin_addr), ntohs(sa.sin_port));
                printf("Old connection, socket fd is %d\n", news);
            }
            if (clientSockets[i] != 0 && recv(clientSockets[i], info, sizeof(info), 0)) {
              printf("Message received: %s \n", info);

            }
          }
        
      }
    }
     else if(temp == 0){
      //keine neuen Verbindungen
    } 
    else {
      perror("select");
      close(s_tcp);
      return 1;
    };
    

    
  }

  close(s_tcp);
}
