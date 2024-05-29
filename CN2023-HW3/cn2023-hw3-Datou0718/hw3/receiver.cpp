#include <iostream>
#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <set>
#include <openssl/evp.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <zlib.h>

#include "def.h"

using namespace std;

int fin, sockFd, base = 1, turn = -1, fileFd, length;
struct sockaddr_in recvAddr;
socklen_t recvAddrSz = sizeof(recvAddr);
segment recvSgmt;
segment segBuffer[MAX_SEG_BUF_SIZE];
set<int> unacked;
// sha256
unsigned char hashStr[EVP_MAX_MD_SIZE];
unsigned int hashLen;
EVP_MD_CTX *sha256;

string hexDigest(const void *buf, int len)
{
    const unsigned char *cbuf = static_cast<const unsigned char *>(buf);
    ostringstream hx{};

    for (int i = 0; i != len; ++i)
        hx << hex << setfill('0') << setw(2) << (unsigned int)cbuf[i];

    return hx.str();
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

void flushBuffer()
{
    printf("flush\n");
    for (int i = turn * MAX_SEG_BUF_SIZE + 1; i < base; i++)
    {
        // sha256
        EVP_DigestUpdate(sha256, segBuffer[i % MAX_SEG_BUF_SIZE].data, segBuffer[i % MAX_SEG_BUF_SIZE].head.length);
        EVP_MD_CTX *tmp_sha256 = EVP_MD_CTX_new();
        EVP_MD_CTX_copy_ex(tmp_sha256, sha256);
        EVP_DigestFinal_ex(tmp_sha256, hashStr, &hashLen);
        EVP_MD_CTX_free(tmp_sha256);
        write(fileFd, segBuffer[i % MAX_SEG_BUF_SIZE].data, segBuffer[i % MAX_SEG_BUF_SIZE].head.length);
        length += segBuffer[i % MAX_SEG_BUF_SIZE].head.length;
    }
    printf("sha256\t%d\t%s\n", length, hexDigest(hashStr, hashLen).c_str());
}

int isAllReceived()
{
    return fin == 1;
}

int isCorrupted()
{
    return (recvSgmt.head.checksum != crc32(0, (Bytef *)recvSgmt.data, recvSgmt.head.length));
}

int isBufferFull()
{
    return unacked.empty();
}

int isOverBuffer()
{
    return (recvSgmt.head.seqNumber > (turn + 1) * MAX_SEG_BUF_SIZE);
}

void printRecv(segment *sgmt)
{
    if (sgmt->head.fin == 1)
    {
        printf("recv\tfin\n");
    }
    else if (isCorrupted())
    {
        printf("drop\tdata\t#%d\t(corrupted)\n", sgmt->head.seqNumber);
    }
    else if (sgmt->head.seqNumber == base)
    {
        printf("recv\tdata\t#%d\t(in order)\n", sgmt->head.seqNumber);
    }
    else if (isOverBuffer())
    {
        printf("drop\tdata\t#%d\t(buffer overflow)\n", sgmt->head.seqNumber);
    }
    else
    {
        printf("recv\tdata\t#%d\t(out of order, sack-ed)\n", sgmt->head.seqNumber);
    }
}

void printSend(segment *sgmt)
{
    if (sgmt->head.fin == 1)
    {
        printf("send\tfinack\n");
    }
    else
    {
        printf("send\tack\t#%d,\tsack\t#%d\n", sgmt->head.ackNumber, sgmt->head.sackNumber);
    }
}

void sendACK(int ACKSeqNum, int SACKSeqNum, int isFin)
{
    segment sgmt{};
    sgmt.head.ack = 1;
    sgmt.head.ackNumber = ACKSeqNum;
    sgmt.head.sackNumber = SACKSeqNum;
    sgmt.head.fin = isFin;
    sgmt.head.syn = 0;
    sgmt.head.checksum = 0;
    sgmt.head.length = 0;
    sendto(sockFd, &sgmt, sizeof(sgmt), 0, (struct sockaddr *)&recvAddr, sizeof(sockaddr));
    printSend(&sgmt);
}

void refillUnacked()
{
    turn++;
    for (int i = 1; i <= MAX_SEG_BUF_SIZE; i++)
    {
        unacked.insert(turn * MAX_SEG_BUF_SIZE + i);
    }
}

// ./receiver <recv_ip> <recv_port> <agent_ip> <agent_port> <dst_filepath>
int main(int argc, char *argv[])
{
    // parse arguments
    if (argc != 6)
    {
        cerr << "Usage: " << argv[0] << " <recv_ip> <recv_port> <agent_ip> <agent_port> <dst_filepath>" << endl;
        exit(1);
    }

    int recv_port, agent_port;
    char recv_ip[50], agent_ip[50];

    // read argument
    setIP(recv_ip, argv[1]);
    sscanf(argv[2], "%d", &recv_port);

    setIP(agent_ip, argv[3]);
    sscanf(argv[4], "%d", &agent_port);

    char *filepath = argv[5];
    fileFd = open(filepath, O_CREAT | O_RDWR | O_APPEND, 0777);

    // make socket related stuff
    sockFd = socket(PF_INET, SOCK_DGRAM, 0);

    recvAddr.sin_family = AF_INET;
    recvAddr.sin_port = htons(agent_port);
    recvAddr.sin_addr.s_addr = inet_addr(agent_ip);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(recv_port);
    addr.sin_addr.s_addr = inet_addr(recv_ip);
    memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));
    bind(sockFd, (struct sockaddr *)&addr, sizeof(addr));

    // initialize sha256
    sha256 = EVP_MD_CTX_new();
    EVP_DigestInit_ex(sha256, EVP_sha256(), NULL);

    // initialize unacked
    refillUnacked();

    while (1)
    {
        recvfrom(sockFd, &recvSgmt, sizeof(recvSgmt), 0, (struct sockaddr *)&recvAddr, &recvAddrSz);
        printRecv(&recvSgmt);
        // corrupted
        if (isCorrupted())
        {
            sendACK(base - 1, base - 1, 0);
            continue;
        }
        // in order
        if (recvSgmt.head.fin == 1)
        {
            fin = 1;
        }
        if (recvSgmt.head.seqNumber == base)
        {
            // save to buffer
            memcpy(&segBuffer[base % MAX_SEG_BUF_SIZE], &recvSgmt, sizeof(segment));
            unacked.erase(base);
            base = unacked.empty() ? (turn + 1) * MAX_SEG_BUF_SIZE + 1 : *unacked.begin();
            sendACK(base - 1, recvSgmt.head.seqNumber, fin);
            if (isAllReceived())
            {
                flushBuffer();
                printf("finsha\t%s\n", hexDigest(hashStr, hashLen).c_str());
                return 0;
            }
            else if (isBufferFull())
            {
                flushBuffer();
                refillUnacked();
            }
        }
        // out of order
        else
        {
            if (isOverBuffer())
            {
                sendACK(base - 1, base - 1, 0);
            }
            else
            {
                // markSACK
                memcpy(&segBuffer[recvSgmt.head.seqNumber % MAX_SEG_BUF_SIZE], &recvSgmt, sizeof(segment));
                unacked.erase(recvSgmt.head.seqNumber);
                sendACK(base - 1, recvSgmt.head.seqNumber, fin);
            }
        }
    }
}