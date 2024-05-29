#include <iostream>
#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <cstring>
#include <unistd.h>
#include <cmath>
#include <sys/time.h>
#include <set>
#include <fcntl.h>
#include <iterator>
#include <zlib.h>

#include "def.h"

using namespace std;

#define slowStart 0
#define congestionAvoidance 1

int state = slowStart;
int threshold, dupACK, base, sockFd, lastInQueue, dataNum;
double cwnd;
struct sockaddr_in recvAddr;
socklen_t recvAddrSz = sizeof(recvAddr);
segment *sgmt, finSgmt, recvSgmt;
clock_t timer, currentTime;
set<int> transmitQueue, haveSend, sendButNotAck;

void printSendorResend(int seqNumber)
{
    if (haveSend.find(seqNumber) != haveSend.end())
        printf("resnd\tdata\t#%d,\twinSize = %d\n", seqNumber, int(cwnd));
    else
        printf("send\tdata\t#%d,\twinSize = %d\n", seqNumber, int(cwnd));
}

void setupSegment(segment *sgmt, int seqNumber, int fin, int checksum, int length)
{
    sgmt->head.seqNumber = seqNumber;
    sgmt->head.ackNumber = 0;
    sgmt->head.sackNumber = 0;
    sgmt->head.fin = fin;
    sgmt->head.syn = 0;
    sgmt->head.ack = 0;
    sgmt->head.checksum = checksum;
    sgmt->head.length = length;
}

void resetTimer()
{
    timer = clock();
}

void setState(int toSet)
{
    state = toSet;
}

int isAtState(int toCheck)
{
    return state == toCheck;
}

void setIP(char *dst, char *src)
{
    if (strcmp(src, "0.0.0.0") == 0 || strcmp(src, "local") == 0 || strcmp(src, "localhost") == 0)
    {
        sscanf("127.0.0.1", "%s", dst);
    }
    else
    {
        sscanf(src, "%s", dst);
    }
    return;
}

void transmitNew()
{
    int len = cwnd - transmitQueue.size(), toSend;
    for (int i = 0; i < len; i++)
    {
        if (!sendButNotAck.empty()) // resend
        {
            toSend = *sendButNotAck.begin();
            sendButNotAck.erase(toSend);
            transmitQueue.insert(toSend);
            printSendorResend(toSend);
        }
        else
        {
            if (lastInQueue == dataNum)
                return;
            lastInQueue++;
            printSendorResend(lastInQueue);
            transmitQueue.insert(lastInQueue);
            haveSend.insert(lastInQueue);
            toSend = lastInQueue;
        }
        sendto(sockFd, &sgmt[toSend], sizeof(sgmt[toSend]), 0, (struct sockaddr *)&recvAddr, sizeof(recvAddr));
    }
}

void transmitMissing()
{
    int toSend = base + 1;
    sendButNotAck.erase(toSend);
    transmitQueue.insert(toSend);
    printSendorResend(toSend);
    sendto(sockFd, &sgmt[toSend], sizeof(sgmt[toSend]), 0, (struct sockaddr *)&recvAddr, sizeof(recvAddr));
}

// ./sender <send_ip> <send_port> <agent_ip> <agent_port> <src_filepath>
int main(int argc, char *argv[])
{
    // parse arguments
    if (argc != 6)
    {
        cerr << "Usage: " << argv[0] << " <send_ip> <send_port> <agent_ip> <agent_port> <src_filepath>" << endl;
        exit(1);
    }

    int send_port, agent_port;
    char send_ip[50], agent_ip[50];

    // read argument
    setIP(send_ip, argv[1]);
    sscanf(argv[2], "%d", &send_port);

    setIP(agent_ip, argv[3]);
    sscanf(argv[4], "%d", &agent_port);

    char *filepath = argv[5];

    // make socket related stuff
    sockFd = socket(PF_INET, SOCK_DGRAM, 0);

    recvAddr.sin_family = AF_INET;
    recvAddr.sin_port = htons(agent_port);
    recvAddr.sin_addr.s_addr = inet_addr(agent_ip);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(send_port);
    addr.sin_addr.s_addr = inet_addr(send_ip);
    memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));
    bind(sockFd, (struct sockaddr *)&addr, sizeof(addr));

    // change socket to non-blocking
    int flags = fcntl(sockFd, F_GETFL, 0);
    fcntl(sockFd, F_SETFL, flags | O_NONBLOCK);

    // read file
    FILE *fp = fopen(filepath, "rb");
    fseek(fp, 0, SEEK_END);
    int file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *data = new char[file_size];
    fread(data, 1, file_size, fp);
    fclose(fp);

    // setup segments
    dataNum = ceil(double(file_size) / double(MAX_SEG_SIZE));
    int lastSegSize = file_size % MAX_SEG_SIZE;
    if (lastSegSize == 0)
        lastSegSize = MAX_SEG_SIZE;

    sgmt = new segment[dataNum + 1];
    for (int i = 1; i <= dataNum; i++)
    {
        int segSize = (i == dataNum) ? lastSegSize : MAX_SEG_SIZE;
        memcpy(sgmt[i].data, data + (i - 1) * MAX_SEG_SIZE, segSize);
        int checksum = crc32(0, (Bytef *)sgmt[i].data, segSize);
        setupSegment(&sgmt[i], i, 0, checksum, segSize);
    }

    // setup fin segment
    setupSegment(&finSgmt, dataNum + 1, 1, 0, 0);

    // init
    cwnd = 1;
    threshold = 16;
    dupACK = 0;
    base = 0;

    transmitNew();
    resetTimer();
    setState(slowStart);

    // receive segments
    while (1)
    {
        // timeout
        currentTime = clock();
        if (currentTime - timer > TIMEOUT_MILLISECONDS * CLOCKS_PER_SEC / 1000)
        {
            threshold = max(1, int(cwnd / 2));
            cwnd = 1;
            // move transmitQueue to sendButNotAck
            for (set<int>::iterator it = transmitQueue.begin(); it != transmitQueue.end(); it++)
                sendButNotAck.insert(*it);
            transmitQueue.clear();
            dupACK = 0;
            // logging
            printf("time\tout,\tthreshold = %d,\twinSize = %d\n", threshold, int(cwnd));
            transmitMissing();
            resetTimer();
            setState(slowStart);
        }

        if (recvfrom(sockFd, &recvSgmt, sizeof(recvSgmt), 0, (struct sockaddr *)&recvAddr, &recvAddrSz) > 0)
        {
            printf("recv\tack\t#%d,\tsack\t#%d\n", recvSgmt.head.ackNumber, recvSgmt.head.sackNumber);
            // dupACK
            if (recvSgmt.head.ackNumber <= base)
            {
                dupACK++;
                // markSACK and transmitNew
                transmitQueue.erase(recvSgmt.head.sackNumber);
                transmitNew();
                if (dupACK == 3)
                    transmitMissing();
            }

            // newACK
            else if (recvSgmt.head.ackNumber > base)
            {
                dupACK = 0;
                // markSACK
                transmitQueue.erase(recvSgmt.head.sackNumber);

                // update base and cwnd
                base = recvSgmt.head.ackNumber;
                if (base == dataNum)
                {
                    // finish transmission, send fin
                    sendto(sockFd, &finSgmt, sizeof(finSgmt), 0, (struct sockaddr *)&recvAddr, sizeof(recvAddr));
                    printf("send\tfin\n");
                    break;
                }
                if (isAtState(slowStart))
                {
                    cwnd++;
                    if (cwnd >= threshold)
                        setState(congestionAvoidance);
                }
                else if (isAtState(congestionAvoidance))
                    cwnd += double(1) / int(cwnd);
                // transmitNew
                transmitNew();
                resetTimer();
            }
        }
    }
    while (1)
    {
        recvfrom(sockFd, &recvSgmt, sizeof(recvSgmt), 0, (struct sockaddr *)&recvAddr, &recvAddrSz);
        if (recvSgmt.head.fin == 1)
        {
            printf("recv\tfinack\n");
            return 0;
        }
    }
}