
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

// Roll: 24105003 Service Type: A/B/C calculated from roll, d = calculated

#define ROLL "24105003"

int getPort(char roll[])
{
    int len = strlen(roll);
    return 30000 + atoi(roll + len - 3);
}

int getType(char roll[])
{
    int len = strlen(roll);
    int sum = 0, i;

    for (i = len - 3; i < len; i++)
        sum += roll[i] - '0';

    return sum % 3;
}

void process(char msg[], char result[], int type)
{
    char text[1024];
    int i;

    if (type == 0)
    {
        char *cmd = strtok(msg, "|");
        char *str = strtok(NULL, "");

        if (cmd == NULL || str == NULL)
        {
            strcpy(result, "Invalid request");
            return;
        }

        strcpy(text, str);

        if (strcmp(cmd, "UP") == 0)
        {
            for (i = 0; text[i]; i++)
                text[i] = toupper((unsigned char)text[i]);
        }
        else if (strcmp(cmd, "LOW") == 0)
        {
            for (i = 0; text[i]; i++)
                text[i] = tolower((unsigned char)text[i]);
        }
        else if (strcmp(cmd, "REV") == 0)
        {
            int j = strlen(text) - 1;
            for (i = 0; i < j; i++, j--)
            {
                char temp = text[i];
                text[i] = text[j];
                text[j] = temp;
            }
        }
        else
        {
            strcpy(result, "Invalid command");
            return;
        }

        strcpy(result, text);
    }
    else if (type == 1)
    {
        char *op = strtok(msg, "|");
        char *ns = strtok(NULL, "|");
        char *values = strtok(NULL, "|");

        if (op == NULL || ns == NULL || values == NULL)
        {
            strcpy(result, "Invalid request");
            return;
        }

        int n = atoi(ns), count = 0;
        char *p = values, *end;
        long long sum = 0, max = 0, min = 0;
        double avg;

        if (n <= 0 || n > 200)
        {
            strcpy(result, "Invalid number of integers");
            return;
        }

        while (*p && isspace((unsigned char)*p))
            p++;

        while (*p && count < n)
        {
            long long val = strtoll(p, &end, 10);

            if (end == p)
                break;

            if (count == 0 || val > max)
                max = val;

            if (count == 0 || val < min)
                min = val;

            sum += val;
            count++;
            p = end;

            while (*p && isspace((unsigned char)*p))
                p++;
        }

        if (count != n || *p != '\0')
        {
            strcpy(result, "Invalid integer list");
            return;
        }

        if (strcmp(op, "SUM") == 0)
            snprintf(result, 1024, "%lld", sum);
        else if (strcmp(op, "MAX") == 0)
            snprintf(result, 1024, "%lld", max);
        else if (strcmp(op, "MIN") == 0)
            snprintf(result, 1024, "%lld", min);
        else if (strcmp(op, "AVG") == 0)
        {
            avg = (double)sum / n;
            snprintf(result, 1024, "%.2f", avg);
        }
        else
            strcpy(result, "Invalid operation");
    }
    else
    {
        int len = strlen(msg);
        int vowels = 0, consonants = 0, digits = 0;

        for (i = 0; msg[i]; i++)
        {
            char c = tolower((unsigned char)msg[i]);

            if (isalpha((unsigned char)msg[i]))
            {
                if (c == 'a' || c == 'e' || c == 'i' ||
                    c == 'o' || c == 'u')
                    vowels++;
                else
                    consonants++;
            }
            else if (isdigit((unsigned char)msg[i]))
                digits++;
        }

        snprintf(result, 1024,
                 "LEN=%d;VOWELS=%d;CONSONANTS=%d;DIGITS=%d",
                 len, vowels, consonants, digits);
    }
}

int main()
{
    int sock, port, type;
    struct sockaddr_in server, client;
    socklen_t client_len;
    char buffer[2048], response[1024];
    char *message, *sep;

    port = getPort(ROLL);
    type = getType(ROLL);

    sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock < 0)
    {
        perror("Socket error");
        return 1;
    }

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);

    if (bind(sock, (struct sockaddr *)&server,
             sizeof(server)) < 0)
    {
        perror("Bind error");
        close(sock);
        return 1;
    }

    printf("Server running on port %d\n", port);
    printf("Service Type: %c (d = %d)\n", 'A' + type, type);

    while (1)
    {
        client_len = sizeof(client);

        int n = recvfrom(sock, buffer, sizeof(buffer) - 1, 0,
                         (struct sockaddr *)&client, &client_len);

        if (n < 0)
        {
            perror("Receive error");
            continue;
        }

        buffer[n] = '\0';

        printf("\nClient: %s:%d\n",
               inet_ntoa(client.sin_addr), ntohs(client.sin_port));

        sep = strchr(buffer, '#');

        if (sep == NULL)
        {
            strcpy(response, "ERROR: Unauthorized client");
        }
        else
        {
            *sep = '\0';
            message = sep + 1;

            if (strcmp(buffer, ROLL) != 0)
            {
                strcpy(response, "ERROR: Unauthorized client");
            }
            else if (strcmp(message, "EXIT") == 0)
            {
                strcpy(response, "Server shutting down");
                sendto(sock, response, strlen(response), 0,
                       (struct sockaddr *)&client, client_len);

                printf("Received message: EXIT\n");
                printf("Sent response: %s\n", response);
                break;
            }
            else
            {
                printf("Received message: %s\n", message);

                process(message, response, type);
            }
        }

        sendto(sock, response, strlen(response), 0,
               (struct sockaddr *)&client, client_len);

        printf("Sent response: %s\n", response);
    }

    close(sock);
    return 0;
}

