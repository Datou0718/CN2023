#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include "utils/base64.h"

#define BUFFER_SIZE 4096

#define ERR_EXIT(a) \
    {               \
        perror(a);  \
        exit(1);    \
    }

int serverPort, sockfd;
char *serverIp, *encoded = NULL;
char statusCode[BUFFER_SIZE], boundary[] = "------WebKitFormBoundarytcHXAhuOytmBIB0B";
struct sockaddr_in addr;

void handleConnection()
{
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        ERR_EXIT("socket()");
    }

    // Set server address
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(serverIp);
    addr.sin_port = htons(serverPort);

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        ERR_EXIT("connect()");
    }
}

int isUnreserved(char ch)
{
    return (isalnum(ch) || ch == '-' || ch == '_' || ch == '.' || ch == '~');
}

ssize_t readLine(int socket, char *buffer, ssize_t maxSize)
{
    ssize_t bytesRead = 0;
    char c = '\0';

    while (bytesRead < maxSize - 1 && c != '\n')
    {
        ssize_t result = recv(socket, &c, 1, 0);
        if (result > 0)
        {
            buffer[bytesRead++] = c;
        }
        else if (result == 0)
        {
            break;
        }
        else
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                continue;
            }
            else
            {
                return -1;
            }
        }
    }

    buffer[bytesRead] = '\0';
    return bytesRead;
}

char *percentEncode(const char *input)
{
    size_t inputLength = strlen(input);
    size_t encodedMaxLength = 3 * inputLength + 1; // +1 for the null terminator
    char *encoded = (char *)malloc(encodedMaxLength);

    size_t j = 0;
    for (size_t i = 0; i < inputLength; ++i)
    {
        char currentChar = input[i];

        if (isUnreserved(currentChar))
        {
            encoded[j++] = currentChar;
        }
        else
        {
            snprintf(&encoded[j], 4, "%%%02X", (unsigned char)currentChar);
            j += 3;
        }
    }

    encoded[j] = '\0';

    return encoded;
}

ssize_t getFileSize(FILE *fp)
{
    fseek(fp, 0L, SEEK_END);
    ssize_t res = (ssize_t)ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    return res;
}

ssize_t calculateContentLength(char *fileName, ssize_t fileSize)
{
    ssize_t contentLength = 0;
    contentLength += strlen(boundary) + 2;
    contentLength += strlen("Content-Disposition: form-data; name=\"upfile\"; filename=\"\"\r\n");
    contentLength += strlen(fileName);
    contentLength += strlen("Content-Type: application/octet-stream\r\n");
    contentLength += strlen("\r\n");
    contentLength += fileSize;
    contentLength += strlen("\r\n");
    contentLength += strlen(boundary) + 2;
    contentLength += strlen("Content-Disposition: form-data; name=\"submit\"\r\n");
    contentLength += strlen("\r\n");
    contentLength += strlen("Upload\r\n");
    contentLength += strlen(boundary) + 4;
    return contentLength;
}

void handleResponse(char *fileName)
{
    char buffer[BUFFER_SIZE], connection[BUFFER_SIZE];
    ssize_t contentLength = 0, reconnect = 0;
    while (readLine(sockfd, buffer, BUFFER_SIZE) > 0)
    {
        if (strcmp(buffer, "\r\n") == 0)
            break;
        else if (sscanf(buffer, "HTTP/1.1 %s\r\n", statusCode) == 1)
            continue;
        else if (sscanf(buffer, "Content-Length: %zd\r\n", &contentLength) == 1)
            continue;
        else if (sscanf(buffer, "Connection: %s\r\n", connection) == 1 && (strcasecmp(connection, "close") == 0))
        {
            reconnect = 1;
        }
    }
    // receive body
    FILE *fp = NULL;
    if (contentLength > 0)
    {
        if (fileName != NULL)
        {
            char path[BUFFER_SIZE] = {'\0'};
            sprintf(path, "./files/%s", fileName);
            fp = fopen(path, "wb");
        }
        ssize_t bytesRead, totalRead = 0;
        while (totalRead < contentLength)
        {
            bytesRead = recv(sockfd, buffer, BUFFER_SIZE, 0);
            totalRead += bytesRead;
            if (fp != NULL)
                fwrite(buffer, sizeof(char), bytesRead, fp);
        }
    }
    if (fp != NULL)
        fclose(fp);
    if (strcmp(statusCode, "200") == 0)
        printf("Command succeeded.\n");
    else
        fprintf(stderr, "Command failed.\n");
    if (reconnect == 1)
    {
        close(sockfd);
        handleConnection();
    }
}

int argCheck(int argc, char *argv[])
{
    if (argc > 4 || argc < 3)
    {
        fprintf(stderr, "Usage: %s [host] [port] [username:password]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int port = atoi(argv[2]);
    return port;
}

void sendHeader(char *requestType, char *endPoint, char *contentType, ssize_t contentLength)
{
    char header[BUFFER_SIZE] = {'\0'}, buffer[BUFFER_SIZE] = {'\0'};
    sprintf(header, "%s %s HTTP/1.1\r\nHost: %s:%d\r\nUser-Agent: CN2023Client/1.0\r\nConnection: keep-alive\r\n", requestType, endPoint, serverIp, serverPort);
    if (contentType != NULL)
    {
        sprintf(buffer, "Content-Type: %s; boundary=%s\r\n", contentType, boundary + 2);
        strcat(header, buffer);
    }
    if (contentLength != -1)
    {
        sprintf(buffer, "Content-Length: %ld\r\n", contentLength);
        strcat(header, buffer);
    }
    if (encoded != NULL)
    {
        sprintf(buffer, "Authorization: Basic %s\r\n", encoded);
        strcat(header, buffer);
    }
    strcat(header, "\r\n");
    send(sockfd, header, strlen(header), 0);
}

void sendBody(FILE *fp, char *fileName, ssize_t fileSize, ssize_t bodySize)
{
    char buffer[BUFFER_SIZE] = {'\0'};
    char disposition1[BUFFER_SIZE] = {'\0'};
    sprintf(disposition1, "Content-Disposition: form-data; name=\"upfile\"; filename=\"%s\"", fileName);
    char *contentType = "Content-Type: application/octet-stream";
    char *disposition2 = "Content-Disposition: form-data; name=\"submit\"";
    char *upload = "Upload";

    int bytes_read;
    sprintf(buffer, "%s\r\n%s\r\n%s\r\n\r\n", boundary, disposition1, contentType);
    send(sockfd, buffer, strlen(buffer), 0);
    memset(buffer, '\0', BUFFER_SIZE);
    while ((bytes_read = fread(buffer, sizeof(char), BUFFER_SIZE, fp)) > 0)
    {
        send(sockfd, buffer, bytes_read, 0);
        memset(buffer, '\0', BUFFER_SIZE);
    }
    sprintf(buffer, "\r\n%s\r\n%s\r\n\r\n%s\r\n%s--\r\n", boundary, disposition2, upload, boundary);
    send(sockfd, buffer, strlen(buffer), 0);
}

void sendHttpRequest()
{
    char input[BUFFER_SIZE], buffer[BUFFER_SIZE], statusCode[BUFFER_SIZE];
    printf("> ");
    while (fgets(input, BUFFER_SIZE, stdin) != NULL)
    {
        input[strcspn(input, "\n")] = '\0';
        char command[BUFFER_SIZE] = {'\0'}, file[BUFFER_SIZE] = {'\0'};
        sscanf(input, "%s %79[^\n]", command, file);

        if (strcmp(command, "quit") == 0)
        {
            printf("Bye.\n");
            return;
        }
        else if (strcmp(command, "put") == 0)
        {
            if (strcmp(file, "") != 0)
            {
                if (access(file, F_OK) != -1)
                {
                    FILE *fp = fopen(file, "rb");
                    if (fp == NULL)
                    {
                        fprintf(stderr, "Command failed.\n");
                        continue;
                    }
                    ssize_t fileSize = getFileSize(fp);
                    ssize_t bodySize = calculateContentLength(file, fileSize);
                    sendHeader("POST", "/api/file", "multipart/form-data", bodySize);
                    sendBody(fp, file, fileSize, bodySize);
                    fclose(fp);
                    handleResponse(NULL);
                }
                else
                    fprintf(stderr, "Command failed.\n");
            }
            else
                fprintf(stderr, "Usage: %s [file]\n", command);
        }
        else if (strcmp(command, "putv") == 0)
        {
            if (strcmp(file, "") != 0)
            {
                if (access(file, F_OK) != -1)
                {
                    FILE *fp = fopen(file, "rb");
                    if (fp == NULL)
                    {
                        fprintf(stderr, "Command failed.\n");
                        continue;
                    }
                    ssize_t fileSize = getFileSize(fp);
                    ssize_t bodySize = calculateContentLength(file, fileSize);
                    sendHeader("POST", "/api/video", "multipart/form-data", bodySize);
                    sendBody(fp, file, fileSize, bodySize);
                    fclose(fp);
                    handleResponse(NULL);
                }
                else
                    fprintf(stderr, "Command failed.\n");
            }
            else
                fprintf(stderr, "Usage: %s [file]\n", command);
        }
        else if (strcmp(command, "get") == 0)
        {
            if (strcmp(file, "") != 0)
            {
                char *encoded = percentEncode(file);
                sprintf(buffer, "/api/file/%s", encoded);
                free(encoded);
                sendHeader("GET", buffer, NULL, -1);
                handleResponse(file);
            }
            else
                fprintf(stderr, "Usage: %s [file]\n", command);
        }
        else
        {
            fprintf(stderr, "Command Not Found.\n");
        }
        printf("> ");
    }
}

int main(int argc, char *argv[])
{
    system("mkdir -p ./files");

    serverPort = argCheck(argc, argv);
    char hostname[BUFFER_SIZE];
    strcpy(hostname, argv[1]);
    size_t encodeLength;
    if (argc == 4)
        encoded = base64_encode(argv[3], strlen(argv[3]), &encodeLength);

    // change domain name to IP address
    struct hostent *host;
    if ((host = gethostbyname(hostname)) == NULL)
    {
        ERR_EXIT("gethostbyname()");
    }
    serverIp = inet_ntoa(*(struct in_addr *)host->h_addr_list[0]);

    handleConnection();

    // Send an HTTP request
    sendHttpRequest();

    close(sockfd);

    return 0;
}
