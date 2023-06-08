#include <errno.h>
#include <stddef.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    int connfd;
    struct sockaddr_in addr;
} client_t;

volatile sig_atomic_t wasSignal = 0;

void signalHandler(int r)
{
    wasSignal = 1;
}

void setupSignalHandler(sigset_t *origMask) {
    struct sigaction sa;
    sigaction(SIGHUP, NULL, &sa);
    sa.sa_handler = signalHandler;
    sa.sa_flags |= SA_RESTART;
    sigaction(SIGHUP, &sa, NULL);

    sigset_t blockedMask;
    sigemptyset(&blockedMask);
    sigaddset (&blockedMask, SIGHUP);
    sigprocmask(SIG_BLOCK, &blockedMask, origMask);
}

int startServer(int port) {
    int sockfd;
    struct sockaddr_in servaddr;
  
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(-1);
    }
    bzero(&servaddr, sizeof(servaddr));
  
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
  
    if ((bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr))) != 0) {
        printf("socket bind failed...\n");
        exit(-1);
    }
  
    if ((listen(sockfd, 5)) != 0) {
        printf("Listen failed...\n");
        exit(-1);
    }

    return sockfd;
}

void printClient(client_t* client) {
    printf("[%s:%d]", inet_ntoa(client->addr.sin_addr), htons(client->addr.sin_port));
}

int main() {
    printf("Process ID:%d\n",getpid());
    int sockfd = startServer(5000);
    puts("Listening...\n");
    client_t clients[3];
    int active_clients = 0;
    char buffer[1024] = {0};

    sigset_t origSigMask;
    setupSignalHandler(&origSigMask);

    while (1) {
        if (wasSignal) {
            wasSignal = 0;
            printf("Clients list: ");
            for (int i = 0; i < active_clients; i++) {
                printClient(&clients[i]);
                printf(" ");
            }
            printf("\n");
        }

        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(sockfd, &fds);
        int maxFd = sockfd;

        for (int i = 0; i < active_clients; i++) {
            FD_SET(clients[i].connfd, &fds);
            if(clients[i].connfd > maxFd) {
                maxFd = clients[i].connfd;
            }
        }

        if (pselect (maxFd + 1, &fds, NULL, NULL, NULL, &origSigMask) < 0 && errno != EINTR) {
            puts("pselect failed\n");
            return -1;
        }
        if (wasSignal)
        {
            wasSignal = 0;
            puts("yEAH U GET SIGHUP\n");
            break;
        }
        
        if (FD_ISSET(sockfd, &fds) && active_clients < 3) {
            client_t *client = &clients[active_clients];
            int len = sizeof(client->addr);
            int connfd = accept(sockfd, (struct sockaddr*)&client->addr, &len);
            if (connfd >= 0) {
                client->connfd = connfd;
                printClient(client);
                printf(" Connected!\n");
                active_clients++;
            }
            else {
                printf("err: %s\n", strerror(errno));
            }
        }

        for (int i = 0; i < active_clients; i++) {
            client_t *client = &clients[i];
            if (FD_ISSET(client->connfd, &fds)) {
                int read_len = read(client->connfd, &buffer, 1023);
                if (read_len > 0) {
                    buffer[read_len - 1] = 0;
                    printClient(client);                    
                    printf(" %s\n", &buffer);
                }
                else {
                    close(client->connfd);
                    printClient(client);
                    printf(" Connection closed\n");
                    clients[i] = clients[active_clients - 1];
                    active_clients--;
                    i--;
                }
            }
        }
    }
    close(sockfd);

    return 0;
}
