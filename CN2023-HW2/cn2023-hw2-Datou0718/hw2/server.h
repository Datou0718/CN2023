#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <poll.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <errno.h>
#include "utils/base64.h"

#define MAX_CLIENTS 150
#define BUFFER_SIZE 4096

#define ERR_EXIT(a) \
    {               \
        perror(a);  \
        exit(1);    \
    }

void MIMEType(char *fileName, char *fileType);
int isUnreserved(char ch);
void percentDecode(char *input);
char *percentEncode(const char *input);
void getFileName(char *ptr, char *fileName);
char *strnstr(const char *haystack, const char *needle, size_t len);
ssize_t getFileSize(FILE *fp);
ssize_t readLine(int socket, char *buffer, ssize_t maxSize);
ssize_t modifyContentLength(ssize_t cur, int commandType, char *fileName, char *url);
FILE *sendResponseHeader(int clientSocket, const char *statusCode, const char *statusMessage, const char *contentType, const char *filePath, int commandType, char *fileName, char *url);
ssize_t checkAuthentication(char *buffer);
void sendResponseBody(int clientSocket, FILE *fp, ssize_t fileSize);
void modifyHTML(int clientSocket, FILE *fp, ssize_t fileSize, int commandType, char *buffer, char *replacement);
void sendHttpResponse(int clientSocket, const char *statusCode, const char *statusMessage, const char *contentType, const char *filePath, int commandType, char *fileName, char *url);
bool handleHttpRequest(int clientSocket);
void readRequestBody(int clientSocket, ssize_t contentLength, char *boundary, int requestType, char *fileName);
void cleanUp(char *filePath, char *boundary);