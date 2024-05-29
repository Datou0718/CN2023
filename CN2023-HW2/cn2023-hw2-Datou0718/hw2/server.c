#include "server.h"

void MIMEType(char *fileName, char *fileType)
{
    if (strstr(fileName, ".mp4") != NULL || strstr(fileName, ".m4v") != NULL)
        strcpy(fileType, "video/mp4");
    else if (strstr(fileName, ".m4s") != NULL)
        strcpy(fileType, "video/iso.segment");
    else if (strstr(fileName, ".m4a") != NULL)
        strcpy(fileType, "audio/mp4");
    else if (strstr(fileName, ".mpd") != NULL)
        strcpy(fileType, "application/dash+xml");
}

int isUnreserved(char ch)
{
    return (isalnum(ch) || ch == '-' || ch == '_' || ch == '.' || ch == '~');
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

void percentDecode(char *input)
{
    size_t inputLength = strlen(input);
    char *decoded = (char *)malloc(inputLength + 1); // +1 for the null terminator

    size_t j = 0;
    for (size_t i = 0; i < inputLength; ++i)
    {
        char currentChar = input[i];

        if (currentChar == '%')
        {
            char hex[3];
            hex[0] = input[++i];
            hex[1] = input[++i];
            hex[2] = '\0';

            decoded[j++] = (char)strtol(hex, NULL, 16);
        }
        else
        {
            decoded[j++] = currentChar;
        }
    }
    decoded[j] = '\0';
    strcpy(input, decoded);
    free(decoded);
}

void getFileName(char *ptr, char *fileName)
{
    int idx = 0;
    while (ptr[idx] != '"')
    {
        fileName[idx] = ptr[idx];
        idx++;
    }
    fileName[idx] = '\0';
    return;
}

char *strnstr(const char *haystack, const char *needle, size_t len)
{
    if (*needle == '\0')
    {
        return (char *)haystack;
    }

    while (len > 0)
    {
        size_t i = 0;
        while (haystack[i] == needle[i] && needle[i] != '\0' && i < len)
        {
            i++;
            if (haystack[i] == '\0')
                continue;
        }

        if (needle[i] == '\0')
        {
            return (char *)haystack;
        }

        haystack++;
        len--;
    }

    return NULL;
}

ssize_t getFileSize(FILE *fp)
{
    fseek(fp, 0L, SEEK_END);
    ssize_t res = (ssize_t)ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    return res;
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

ssize_t modifyContentLength(ssize_t cur, int commandType, char *fileName, char *url)
{
    int len;
    char dirPath[BUFFER_SIZE];
    if (commandType == 2)
    {
        cur -= strlen("<?FILE_LIST?>\n");
        len = strlen("<tr><td><a href=\"/api/file/\"></a></td></tr>\n");
        strcpy(dirPath, "./web/files");
    }
    else if (commandType == 3)
    {
        cur -= strlen("<?VIDEO_LIST?>\n");
        len = strlen("<tr><td><a href=\"/video/\"></a></td></tr>\n");
        strcpy(dirPath, "./web/videos");
    }
    else if (commandType == 4)
    {
        cur -= strlen("<?VIDEO_NAME?>");
        cur += strlen(fileName);
        cur -= strlen("<?MPD_PATH?>");
        cur += strlen(url);
        return cur;
    }
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(dirPath)) != NULL)
    {
        while ((ent = readdir(dir)) != NULL)
        {
            char *encoded = percentEncode(ent->d_name);
            if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0)
                cur += len + strlen(encoded) + strlen(ent->d_name);
            free(encoded);
        }
        closedir(dir);
        cur += 1;
    }
    return cur;
}

ssize_t checkAuthentication(char *buffer)
{
    size_t decoded_length = 0;
    unsigned char *decoded = base64_decode(buffer, strlen(buffer), &decoded_length);
    decoded
        printf("%s %d\n", (char *)decoded, strlen((char *)decoded));
    FILE *secretFile = fopen("./secret", "rb");
    char secretLine[BUFFER_SIZE];
    while (fgets(secretLine, BUFFER_SIZE, secretFile) != NULL)
    {
        ssize_t len = strlen(secretLine);
        if (len > 0 && secretLine[len - 1] == '\n')
        {
            secretLine[len - 1] = '\0';
        }
        if (strcmp((char *)decoded, secretLine) == 0)
        {
            fclose(secretFile);
            return 1;
        }
    }
    fclose(secretFile);
    return 0;
}

void modifyHTML(int clientSocket, FILE *fp, ssize_t fileSize, int commandType, char *buffer, char *replacement)
{
    char dirPath[BUFFER_SIZE], prefixTag[BUFFER_SIZE], prefix[BUFFER_SIZE], suffix[BUFFER_SIZE];
    int toReplaceLength = 0;
    if (commandType == 2)
    {
        strcpy(dirPath, "./web/files");
        strcpy(prefixTag, "<?FILE_LIST?>");
    }
    else if (commandType == 3)
    {
        strcpy(dirPath, "./web/videos");
        strcpy(prefixTag, "<?VIDEO_LIST?>");
    }
    else if (commandType == 4)
    {
        strcpy(prefixTag, "<?VIDEO_NAME?>");
    }
    else if (commandType == -4)
    {
        strcpy(prefixTag, "<?MPD_PATH?>");
    }
    toReplaceLength = strlen(prefixTag);

    char *prefixPtr = strnstr(buffer, prefixTag, BUFFER_SIZE);
    char *suffixPtr = prefixPtr + toReplaceLength;

    // store the prefix and suffix
    strncpy(prefix, buffer, prefixPtr - buffer);
    prefix[prefixPtr - buffer] = '\0';
    strncpy(suffix, suffixPtr, fileSize + buffer - suffixPtr);
    suffix[fileSize + buffer - suffixPtr] = '\0';
    strcpy(buffer, suffix);

    send(clientSocket, prefix, strlen(prefix), MSG_NOSIGNAL);

    if (commandType == 2 || commandType == 3)
    {
        DIR *dir;
        struct dirent *ent;
        char replace[BUFFER_SIZE];
        if ((dir = opendir(dirPath)) != NULL)
        {
            while ((ent = readdir(dir)) != NULL)
            {
                if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0)
                {
                    char *encoded = percentEncode(ent->d_name);
                    if (commandType == 2)
                        sprintf(replace, "<tr><td><a href=\"/api/file/%s\">%s</a></td></tr>\n", encoded, ent->d_name);
                    else
                        sprintf(replace, "<tr><td><a href=\"/video/%s\">%s</a></td></tr>\n", encoded, ent->d_name);
                    free(encoded);
                    send(clientSocket, replace, strlen(replace), MSG_NOSIGNAL);
                }
            }
            closedir(dir);
        }
    }
    else if (commandType == 4 || commandType == -4)
    {
        send(clientSocket, replacement, strlen(replacement), MSG_NOSIGNAL);
    }
}

void sendHttpResponse(int clientSocket, const char *statusCode, const char *statusMessage, const char *contentType, const char *filePath, int commandType, char *fileName, char *url)
{
    char buffer[BUFFER_SIZE];
    FILE *fp = sendResponseHeader(clientSocket, statusCode, statusMessage, contentType, filePath, commandType, fileName, url);
    // plain text
    if (commandType == 0)
    {
        strcpy(buffer, filePath);
        send(clientSocket, buffer, strlen(buffer), MSG_NOSIGNAL);
        return;
    }
    // method not allowed
    else if (commandType == 5)
    {
        return;
    }
    // file not found
    else if (fp == NULL)
    {
        sendHttpResponse(clientSocket, "404", "Not Found", "text/plain", "Not Found", 0, NULL, NULL);
        return;
    }

    ssize_t fileSize = getFileSize(fp);

    // simply send the file
    if (commandType == 1)
        sendResponseBody(clientSocket, fp, fileSize);
    // modify the html(listf or listv)
    else if (commandType == 2 || commandType == 3)
    {
        fread(buffer, BUFFER_SIZE, 1, fp);
        modifyHTML(clientSocket, fp, fileSize, commandType, buffer, NULL);
        send(clientSocket, buffer, strlen(buffer), MSG_NOSIGNAL);
    }
    // modify the html(player)
    else if (commandType == 4)
    {
        fread(buffer, BUFFER_SIZE, 1, fp);
        modifyHTML(clientSocket, fp, fileSize, commandType, buffer, fileName);
        modifyHTML(clientSocket, fp, fileSize, -commandType, buffer, url);
        send(clientSocket, buffer, strlen(buffer), MSG_NOSIGNAL);
    }
}

FILE *sendResponseHeader(int clientSocket, const char *statusCode, const char *statusMessage, const char *contentType, const char *filePath, int commandType, char *fileName, char *url)
{
    char response[BUFFER_SIZE];
    ssize_t contentLength = 0;
    FILE *fp = NULL;
    if (commandType == 0)
        contentLength = strlen(filePath);
    else if (commandType > 0 && commandType < 5)
    {
        fp = fopen(filePath, "rb");
        if (fp == NULL)
        {
            sendHttpResponse(clientSocket, "404", "Not Found", "text/plain", "Not Found", 0, NULL, NULL);
            return NULL;
        }
        if (commandType == 1)
            contentLength = getFileSize(fp);
        else if (commandType == 2 || commandType == 3 || commandType == 4)
            contentLength = modifyContentLength(getFileSize(fp), commandType, fileName, url);
    }
    char temp[BUFFER_SIZE];
    sprintf(response, "HTTP/1.1 %s %s\r\nServer: CN2023Server/1.0\r\n", statusCode, statusMessage);
    if (strcmp(statusCode, "401") == 0)
        strcat(response, "WWW-Authenticate: Basic realm=\"B10902034\"\r\n");
    else if (strcmp(statusCode, "405") == 0)
    {
        char temp[BUFFER_SIZE];
        sprintf(temp, "Allow: %s\r\nContent-Length: %d\r\n\r\n", filePath, 0);
        strcat(response, temp);
        send(clientSocket, response, strlen(response), MSG_NOSIGNAL);
        return NULL;
    }
    sprintf(temp, "Content-Type: %s\r\nContent-Length: %ld\r\n\r\n", contentType, contentLength);
    strcat(response, temp);
    send(clientSocket, response, strlen(response), MSG_NOSIGNAL);
    return fp;
}

void sendResponseBody(int clientSocket, FILE *fp, ssize_t fileSize)
{
    ssize_t bytesRead = 0, sizeCnt = 0;
    char buffer[BUFFER_SIZE];
    while (fileSize > sizeCnt)
    {
        if ((bytesRead = fread(buffer, 1, sizeof(buffer), fp)) > 0)
        {
            sizeCnt += bytesRead;
            send(clientSocket, buffer, bytesRead, MSG_NOSIGNAL);
        }
    }
    fclose(fp);
}

void cleanUp(char *filePath, char *boundary)
{
    FILE *file = fopen(filePath, "rb+");
    char *line = NULL, *pos;
    int foundBoundary = 0;
    size_t bytesRead = 0, len = 0, totalBytesRead = 0;
    while ((bytesRead = getline(&line, &len, file)) != -1)
    {
        totalBytesRead += bytesRead;
        pos = strnstr(line, boundary, bytesRead);
        if (pos != NULL)
        {
            foundBoundary = 1;
            break;
        }
    }
    if (foundBoundary)
        ftruncate(fileno(file), totalBytesRead - (bytesRead - (pos - line)));
    fclose(file);
    if (line != NULL)
        free(line);
}

void readRequestBody(int clientSocket, ssize_t contentLength, char *boundary, int requestType, char *fileName)
{
    ssize_t bytesRead = 0, boundaryCnt = 0, totalRead = 0, finish = 0, skip = (requestType == 3 ? 1 : 0);
    FILE *fp = NULL;
    char filePath[BUFFER_SIZE];
    while (totalRead < contentLength)
    {
        char buffer[BUFFER_SIZE] = {'\0'};
        bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        totalRead += bytesRead;
        if (boundaryCnt == 0 && !skip)
        {
            char *ptr = strnstr(buffer, boundary, BUFFER_SIZE), *ptr2 = NULL;
            if (ptr != NULL)
            {
                boundaryCnt++;
                ptr = strstr(ptr, "Content-Disposition: form-data; name=\"upfile\"; filename=\"");
                ptr += strlen("Content-Disposition: form-data; name=\"upfile\"; filename=\"");
                getFileName(ptr, fileName);
                if (requestType == 1)
                    sprintf(filePath, "./web/files/%s", fileName);
                else if (requestType == 2)
                    sprintf(filePath, "./web/tmp/%s", fileName);
                fp = fopen(filePath, "wb");
                ptr = strnstr(ptr, "\r\n\r\n", BUFFER_SIZE - (ptr - buffer));
                ptr += 4;

                ptr2 = strnstr(ptr, boundary, BUFFER_SIZE - (ptr - buffer));
                if (ptr2 != NULL)
                {
                    boundaryCnt++;
                    fwrite(ptr, 1, ptr2 - ptr - 2, fp);
                    finish = 1;
                }
                else
                {
                    fwrite(ptr, 1, bytesRead - (ptr - buffer), fp);
                }
            }
        }
        else if (boundaryCnt == 1 && !skip)
        {
            char *ptr = strnstr(buffer, boundary, BUFFER_SIZE);
            if (ptr != NULL)
            {
                boundaryCnt++;
                fwrite(buffer, 1, ptr - buffer - 2, fp);
                finish = 1;
            }
            else
            {
                fwrite(buffer, 1, bytesRead, fp);
            }
        }
    }
    if (!skip)
    {
        fclose(fp);
        if (!finish)
            cleanUp(filePath, boundary);
    }
}

bool handleHttpRequest(int clientSocket)
{
    char buffer[BUFFER_SIZE];
    ssize_t bytesRead, contentLength = 0, isAuth = 0;
    char method[BUFFER_SIZE], route[BUFFER_SIZE], fileName[BUFFER_SIZE], filePath[BUFFER_SIZE];
    char auth[BUFFER_SIZE], boundary[BUFFER_SIZE], temp[BUFFER_SIZE];
    bool toClose = false;

    // Read start-line
    if ((bytesRead = readLine(clientSocket, buffer, BUFFER_SIZE)) <= 0)
    {
        // broken
        return true;
    }
    sscanf(buffer, "%s %s", method, route);
    // Read HTTP header
    while ((bytesRead = readLine(clientSocket, buffer, BUFFER_SIZE)) > 0)
    {
        if (strcmp(buffer, "\r\n") == 0)
            break;
        else if (sscanf(buffer, "Content-Length: %ld\r\n", &contentLength) == 1)
            continue;
        else if (sscanf(buffer, "Authorization: Basic %s\r\n", auth) == 1)
        {
            printf("start authenticating\n");
            isAuth = checkAuthentication(auth);
            printf("finish authenticating\n");
        }
        else if (sscanf(buffer, "Connection: %s\r\n", temp) == 1 && (strcasecmp(temp, "close") == 0))
            toClose = true;
        else if (sscanf(buffer, "Content-Type: multipart/form-data; boundary=%s\r\n", temp) == 1)
            sprintf(boundary, "--%s", temp);
    }

    if (strcmp(route, "/") == 0)
    {
        readRequestBody(clientSocket, contentLength, boundary, 3, fileName);
        if (strcmp(method, "GET") == 0)
        {
            sendHttpResponse(clientSocket, "200", "OK", "text/html", "./web/index.html", 1, NULL, NULL);
        }
        else
        {
            sendHttpResponse(clientSocket, "405", "Method Not Allowed", "text/plain", "GET", 5, NULL, NULL);
        }
    }
    else if (strcmp(route, "/upload/file") == 0)
    {
        readRequestBody(clientSocket, contentLength, boundary, 3, fileName);
        if (strcmp(method, "GET") == 0)
        {
            if (isAuth)
                sendHttpResponse(clientSocket, "200", "OK", "text/html", "./web/uploadf.html", 1, NULL, NULL);
            else
                sendHttpResponse(clientSocket, "401", "Unauthorized", "text/plain", "Unauthorized\n", 0, NULL, NULL);
        }
        else
            sendHttpResponse(clientSocket, "405", "Method Not Allowed", "text/plain", "GET", 5, NULL, NULL);
    }
    else if (strcmp(route, "/upload/video") == 0)
    {
        readRequestBody(clientSocket, contentLength, boundary, 3, fileName);
        if (strcmp(method, "GET") == 0)
        {
            if (isAuth)
                sendHttpResponse(clientSocket, "200", "OK", "text/html", "./web/uploadv.html", 1, NULL, NULL);
            else
                sendHttpResponse(clientSocket, "401", "Unauthorized", "text/plain", "Unauthorized\n", 0, NULL, NULL);
        }
        else
            sendHttpResponse(clientSocket, "405", "Method Not Allowed", "text/plain", "GET", 5, NULL, NULL);
    }
    else if (strcmp(route, "/file/") == 0)
    {
        readRequestBody(clientSocket, contentLength, boundary, 3, fileName);
        if (strcmp(method, "GET") == 0)
            sendHttpResponse(clientSocket, "200", "OK", "text/html", "./web/listf.rhtml", 2, NULL, NULL);
        else
            sendHttpResponse(clientSocket, "405", "Method Not Allowed", "text/plain", "GET", 5, NULL, NULL);
    }
    else if (strcmp(route, "/video/") == 0)
    {
        readRequestBody(clientSocket, contentLength, boundary, 3, fileName);
        if (strcmp(method, "GET") == 0)
            sendHttpResponse(clientSocket, "200", "OK", "text/html", "./web/listv.rhtml", 3, NULL, NULL);
        else
            sendHttpResponse(clientSocket, "405", "Method Not Allowed", "text/plain", "GET", 5, NULL, NULL);
    }
    else if (sscanf(route, "/video/%s", fileName) == 1)
    {
        readRequestBody(clientSocket, contentLength, boundary, 3, fileName);
        if (strcmp(method, "GET") == 0)
        {
            char url[BUFFER_SIZE];
            percentDecode(fileName);
            int len = strlen(fileName);
            for (int i = len - 1; i >= 0; i--)
            {
                if (fileName[i] == '.')
                {
                    fileName[i] = '\0';
                    break;
                }
            }
            char *encoded = percentEncode(fileName);
            sprintf(url, "\"/api/video/%s/dash.mpd\"", encoded);
            free(encoded);
            sendHttpResponse(clientSocket, "200", "OK", "text/html", "./web/player.rhtml", 4, fileName, url);
        }
        else
            sendHttpResponse(clientSocket, "405", "Method Not Allowed", "text/plain", "GET", 5, NULL, NULL);
    }
    else if (sscanf(route, "/api/file/%s", fileName) == 1)
    {
        readRequestBody(clientSocket, contentLength, boundary, 3, fileName);
        if (strcmp(method, "GET") == 0)
        {
            percentDecode(fileName);
            sprintf(filePath, "./web/files/%s", fileName);
            sendHttpResponse(clientSocket, "200", "OK", "text/plain", filePath, 1, NULL, NULL);
        }
        else
            sendHttpResponse(clientSocket, "405", "Method Not Allowed", "text/plain", "GET", 5, NULL, NULL);
    }
    else if (sscanf(route, "/api/video/%s", fileName) == 1)
    {
        readRequestBody(clientSocket, contentLength, boundary, 3, fileName);
        if (strcmp(method, "GET") == 0)
        {
            char fileType[BUFFER_SIZE];
            percentDecode(fileName);
            sprintf(filePath, "./web/videos/%s", fileName);
            MIMEType(fileName, fileType);
            sendHttpResponse(clientSocket, "200", "OK", fileType, filePath, 1, NULL, NULL);
        }
        else
            sendHttpResponse(clientSocket, "405", "Method Not Allowed", "text/plain", "GET", 5, NULL, NULL);
    }
    else if (strcmp(route, "/api/file") == 0)
    {
        if (strcmp(method, "POST") == 0)
        {
            if (isAuth)
            {
                readRequestBody(clientSocket, contentLength, boundary, 1, fileName);
                sendHttpResponse(clientSocket, "200", "OK", "text/plain", "File Uploaded\n", 0, NULL, NULL);
            }
            else
            {
                readRequestBody(clientSocket, contentLength, boundary, 3, fileName);
                sendHttpResponse(clientSocket, "401", "Unauthorized", "text/plain", "Unauthorized\n", 0, NULL, NULL);
            }
        }
        else
        {
            readRequestBody(clientSocket, contentLength, boundary, 3, fileName);
            sendHttpResponse(clientSocket, "405", "Method Not Allowed", "text/plain", "POST", 5, NULL, NULL);
        }
    }
    else if (strcmp(route, "/api/video") == 0)
    {
        if (strcmp(method, "POST") == 0)
        {
            if (isAuth)
            {
                char dirPath[BUFFER_SIZE], filePath[BUFFER_SIZE], comm[BUFFER_SIZE];
                readRequestBody(clientSocket, contentLength, boundary, 2, fileName);
                sendHttpResponse(clientSocket, "200", "OK", "text/plain", "File Uploaded\n", 0, NULL, NULL);
                sprintf(filePath, "./web/tmp/%s", fileName);
                sprintf(dirPath, "./web/videos/%s", fileName);
                int len = strlen(dirPath);
                for (int i = len - 1; i >= 0; i--)
                {
                    if (dirPath[i] == '.')
                    {
                        dirPath[i] = '\0';
                        break;
                    }
                }
                int pid = fork();
                if (pid == 0)
                {
                    int pid2 = fork();
                    if (pid2 == 0)
                    {
                        sprintf(comm, "mkdir -p \"%s\"", dirPath);
                        system(comm);
                        sprintf(comm, "ffmpeg -re -i \"%s\" -c:a aac -c:v libx264 \
                            -map 0 -b:v:1 6M -s:v:1 1920x1080 -profile:v:1 high \
                            -map 0 -b:v:0 144k -s:v:0 256x144 -profile:v:0 baseline \
                            -bf 1 -keyint_min 120 -g 120 -sc_threshold 0 -b_strategy 0 \
                            -ar:a:1 22050 -use_timeline 1 -use_template 1 \
                            -adaptation_sets \"id=0,streams=v id=1,streams=a\" -f dash \
                            \"%s/dash.mpd\"",
                                filePath, dirPath);
                        system(comm);
                        exit(0);
                    }
                    else
                        exit(0);
                }
                else
                    wait(NULL);
            }
            else
            {
                readRequestBody(clientSocket, contentLength, boundary, 3, fileName);
                sendHttpResponse(clientSocket, "401", "Unauthorized", "text/plain", "Unauthorized\n", 0, NULL, NULL);
            }
        }
        else
        {
            readRequestBody(clientSocket, contentLength, boundary, 3, fileName);
            sendHttpResponse(clientSocket, "405", "Method Not Allowed", "text/plain", "POST", 5, NULL, NULL);
        }
    }
    else
    {
        readRequestBody(clientSocket, contentLength, boundary, 3, fileName);
        sendHttpResponse(clientSocket, "404", "Not Found", "text/plain", "Not Found", 0, NULL, NULL);
    }
    return toClose;
}

int argCheck(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s [port]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int port = atoi(argv[1]);
    return port;
}

int main(int argc, char *argv[])
{
    int port = argCheck(argc, argv);
    system("mkdir -p ./web/files");
    system("mkdir -p ./web/videos");
    system("mkdir -p ./web/tmp");

    int listenfd, connfd;
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        ERR_EXIT("socket()");
    }
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
    {
        ERR_EXIT("setsockopt()");
    }

    struct sockaddr_in server_addr, client_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    if (bind(listenfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        ERR_EXIT("bind()");
    }

    if (listen(listenfd, 3) < 0)
    {
        ERR_EXIT("listen()");
    }

    // polling
    int clients[MAX_CLIENTS];
    memset(clients, 0, sizeof(clients));

    int num_clients = 0;
    int client_addr_len = sizeof(client_addr);
    struct pollfd fds[MAX_CLIENTS + 1]; // +1 for the listening socket
    memset(fds, 0, sizeof(fds));

    fds[0].fd = listenfd;
    fds[0].events = POLLIN;

    while (1)
    {
        int ret = poll(fds, num_clients + 1, -1);
        if (ret < 0)
        {
            ERR_EXIT("poll()");
        }

        // Check if a new client is trying to connect
        if (fds[0].revents & POLLIN)
        {
            if (num_clients < MAX_CLIENTS)
            {
                if ((connfd = accept(listenfd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_len)) < 0)
                {
                    ERR_EXIT("accept()");
                }

                clients[num_clients] = connfd;
                fds[num_clients + 1].fd = connfd;
                fds[num_clients + 1].events = POLLIN;

                num_clients++;
            }
        }

        // handle data from clients
        for (int i = 0; i < num_clients; i++)
        {
            if (fds[i + 1].revents & POLLIN)
            {
                if (handleHttpRequest(fds[i + 1].fd))
                {
                    close(fds[i + 1].fd);
                    memmove(clients + i, clients + i + 1, (num_clients - i - 1) * sizeof(int));
                    memset(&clients[num_clients - 1], 0, sizeof(int));
                    for (int j = i + 1; j <= num_clients; ++j)
                    {
                        fds[j] = fds[j + 1];
                    }
                    num_clients--;
                }
            }
        }
    }
    return 0;
}
