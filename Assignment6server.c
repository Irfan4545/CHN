
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080
#define SIZE 2048

struct Request
{
    int id;
    char data[SIZE];
};

int main()
{
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    struct Request req;
    char response[200];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0)
    {
        perror("Socket error");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0)
    {
        perror("Bind error");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 5) < 0)
    {
        perror("Listen error");
        close(server_fd);
        return 1;
    }

    signal(SIGCHLD, SIG_IGN);

    printf("Server is waiting for clients...\n");

    while (1)
    {
        client_fd = accept(server_fd,
                           (struct sockaddr *)&client_addr,
                           &addr_len);

        if (client_fd < 0)
        {
            perror("Accept error");
            continue;
        }

        if (fork() == 0)
        {
            close(server_fd);

            size_t total = 0;
            ssize_t n;

            while (total < sizeof(req))
            {
                n = read(client_fd, (char *)&req + total,
                         sizeof(req) - total);

                if (n <= 0)
                {
                    close(client_fd);
                    exit(1);
                }

                total += n;
            }

            int found = 0;
            char *line = strtok(req.data, "\n");

            while (line != NULL)
            {
                int id;
                char name[100], department[100];

                if (sscanf(line, "%d %99s %99s",
                           &id, name, department) == 3)
                {
                    if (id == req.id)
                    {
                        snprintf(response, sizeof(response),
                                 "Record Found : %s", line);
                        found = 1;
                        break;
                    }
                }

                line = strtok(NULL, "\n");
            }

            if (!found)
                strcpy(response, "Record Not Found");

            write(client_fd, response, strlen(response) + 1);

            printf("Client request processed: ID %d\n", req.id);

            close(client_fd);
            exit(0);
        }

        close(client_fd);
    }

    close(server_fd);
    return 0;
}

