#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// TODO: Remove me.
#define SRV_PORT 7777

int main(int argc, char** argv) {
    (void)argc;  // TODO: Remove cast and parse arguments.
    (void)argv;  // TODO: Remove cast and parse arguments.
    int s_tcp, news, maxfd, selectRet;
    fd_set all_fds, copy_fds;
    struct sockaddr_in sa, sa_client;
    unsigned int sa_len = sizeof(struct sockaddr_in);
    char info[256], temp[256];

    sa.sin_family = AF_INET;
    sa.sin_port = htons(SRV_PORT);
    sa.sin_addr.s_addr = INADDR_ANY;


    if ((s_tcp = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("TCP Socket");
        return 1;
    }

    if (bind(s_tcp, (struct sockaddr*)&sa, sa_len) < 0) {
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
    // TODO: Check port in use and print it.
    printf("Waiting for TCP connections on s_tcp = %d... \n", s_tcp);

    while (1) {
        copy_fds = all_fds;
        printf("Select blocking\n");
        selectRet = select(5, &copy_fds, NULL, NULL, NULL);
        printf("Select free\n");
        if(selectRet<0){
            perror("select");
            close(s_tcp);
        } else if(selectRet==0){
            //keine neuen Verbindungen oder Anfragen
            printf("Nix neues...\n");
        }
        for(int i=0; i<=maxfd; i++){
            if(FD_ISSET(i, &copy_fds)){
                printf("FD_ISSET i=%d\n", i);
                if(i==s_tcp){
                    if ((news = accept(s_tcp, (struct sockaddr*)&sa_client, &sa_len)) < 0) {
                        perror("accept");
                        close(s_tcp);
                        return 1;
                    }
                    FD_SET(news, &all_fds);
                    if(maxfd<news){
                        printf("maxfd<news\n");
                        maxfd = news;
                    }
                    printf("selectserver: new connection from %s on socket %d\n",
                           inet_ntop(sa.sin_family, (struct sockaddr*)&sa.sin_addr,
                                     temp, INET6_ADDRSTRLEN), news);
                    printf("New socket = %d\n", news);
                } else {
                    printf("<else-block> i=%d\n", i);

                    if (recv(news, info, sizeof(info), 0)) {
                        printf("Message received: %s \n", info);
                    }
                    printf("Nach receive\n");
                }
            }


        }


    }





    close(s_tcp);
}