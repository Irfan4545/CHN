
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int getPort(char roll[])
{
    int len = strlen(roll);
    return 30000 + atoi(roll + len - 3);
}

int main(int argc, char *argv[])
{
    int sock, port;
    struct sockaddr_in server;
    socklen_t server_len = sizeof(server);
    char input[1024], message[1200], response[2048];

    if (argc != 3)
    {
        printf("Usage: %s <server_ip> <roll>\n", argv[0]);
        return 1;
    }

    if (strlen(argv[2]) < 3)
    {
        printf("Invalid roll number\n");
        return 1;
    }

    port = getPort(argv[2]);

    sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock < 0)
    {
        perror("Socket error");
        return 1;
    }

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    if (inet_pton(AF_INET, argv[1], &server.sin_addr) != 1)
    {
        printf("Invalid server IP address\n");
        close(sock);
        return 1;
    }

    printf("Connected to UDP server at port %d\n", port);
    printf("Type EXIT to quit.\n");

    while (1)
    {
        printf("\nEnter request: ");

        if (fgets(input, sizeof(input), stdin) == NULL)
            strcpy(input, "EXIT");
        else
            input[strcspn(input, "\n")] = '\0';

        if (snprintf(message, sizeof(message), "%s#%s",
                     argv[2], input) >= sizeof(message))
        {
            printf("Request too long\n");
            continue;
        }

        if (sendto(sock, message, strlen(message), 0,
                   (struct sockaddr *)&server,
                   sizeof(server)) < 0)
        {
            perror("Send error");
            break;
        }

        int n = recvfrom(sock, response, sizeof(response) - 1, 0,
                         (struct sockaddr *)&server, &server_len);

        if (n < 0)
        {
            perror("Receive error");
            break;
        }

        response[n] = '\0';

        printf("Server Response: %s\n", response);

        if (strcmp(input, "EXIT") == 0)
            break;
    }

    close(sock);
    return 0;
}
