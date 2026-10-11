
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT 8080
#define SIZE 2048

struct Request
{
    int id;
    char data[SIZE];
};

int main()
{
    int sock;
    struct sockaddr_in server_addr;
    struct Request req;
    char response[200];
    FILE *fp;
    size_t total;
    ssize_t n;

    fp = fopen("data.txt", "r");

    if (fp == NULL)
    {
        perror("File error");
        return 1;
    }

    total = fread(req.data, 1, SIZE - 1, fp);
    req.data[total] = '\0';
    fclose(fp);

    printf("Enter Student ID to search : ");
    scanf("%d", &req.id);

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0)
    {
        perror("Socket error");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1",
              &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0)
    {
        perror("Connection error");
        close(sock);
        return 1;
    }

    total = 0;

    while (total < sizeof(req))
    {
        n = write(sock, (char *)&req + total,
                  sizeof(req) - total);

        if (n <= 0)
        {
            perror("Write error");
            close(sock);
            return 1;
        }

        total += n;
    }

    total = 0;

    while (total < sizeof(response) - 1)
    {
        n = read(sock, response + total,
                 sizeof(response) - 1 - total);

        if (n <= 0)
            break;

        total += n;

        if (memchr(response, '\0', total) != NULL)
            break;
    }

    response[total] = '\0';

    if (total > 0)
        printf("Server Response : %s\n", response);
    else
        printf("No response from server\n");

    close(sock);

    return 0;
}
