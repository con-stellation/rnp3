#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>

#define BUFFER_SIZE 256
#define SRV_PORT 7777

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

                    if (recv(i, info, sizeof(info), 0)) {
                        printf("Message received: %s\n", info);
                        memset(info, 0, sizeof(info));

                    }
                    printf("Nach receive\n");
                }
            }
        }
    }
    freeaddrinfo(servinfo);
    close(s_tcp);
    return 0;
}
