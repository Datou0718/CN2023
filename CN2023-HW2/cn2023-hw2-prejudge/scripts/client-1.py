from pwn import * 
from assets.utils import *
import sys

TIMEOUT = 2
PORT = 2023
REPO = '..'
SERV = 'nodejs'

def sendCMD(p, cmd, expect, timeout=TIMEOUT, expect_aliases=[]):
    recv = p.sendlineafter('> '.encode(), cmd.encode(), timeout)
    print(f'send = {cmd}', end='')
    assert(recv.decode() == '> ')

    recv = p.recvline()
    print(f' recv = {recv}')
    
    if recv.decode() != expect:
        for alias in expect_aliases:
            if recv.decode() == alias:
                return
        assert(0)

if __name__ == '__main__':
    if len(sys.argv) >= 2:
        PORT = sys.argv[1]
    if len(sys.argv) >= 3:
        REPO = sys.argv[2]
    if len(sys.argv) >= 4:
        SERV = sys.argv[3]

    delPath(f'{REPO}/hw2/files')

    pty = process.PTY
    c = process(f'./client 127.0.0.1 {PORT} demo:123', shell=True, cwd=f'{REPO}/hw2', stdin=pty, stdout=pty) 
    # argv=['./hw2/client', '127.0.0.1', '8080', 'username:password']
    # r = process(argv=argv)

    genFile(f'{REPO}/hw2/L4RGebUtNoT7o01ArgE', size='100M')
    genFile(f'{REPO}/hw2/a 5 a 5 a a 5 5 5 o o 1 a 1 a a 5 5 5 o o', size='1M')
    genFile(f'{REPO}/hw2/iS7H@TL3g4L?', size='5M')

    sendCMD(c, 'put L4RGebUtNoT7o01ArgE', 'Command succeeded.\n', timeout=10)
    cmpFile(f'{REPO}/hw2/L4RGebUtNoT7o01ArgE', f'assets/pseudo-server/files-{SERV}/L4RGebUtNoT7o01ArgE')
    sendCMD(c, 'put a 5 a 5 a a 5 5 5 o o 1 a 1 a a 5 5 5 o o', 'Command succeeded.\n', timeout=TIMEOUT)
    cmpFile(f'{REPO}/hw2/a 5 a 5 a a 5 5 5 o o 1 a 1 a a 5 5 5 o o', f'assets/pseudo-server/files-{SERV}/a 5 a 5 a a 5 5 5 o o 1 a 1 a a 5 5 5 o o')
    
    sendCMD(c, 'get L4RGebUtNoT7o01ArgE', 'Command succeeded.\n', timeout=10)
    cmpFile(f'assets/pseudo-server/files-{SERV}/L4RGebUtNoT7o01ArgE', f'{REPO}/hw2/files/L4RGebUtNoT7o01ArgE')
    sendCMD(c, 'get a 5 a 5 a a 5 5 5 o o 1 a 1 a a 5 5 5 o o', 'Command succeeded.\n', timeout=TIMEOUT)
    cmpFile(f'assets/pseudo-server/files-{SERV}/a 5 a 5 a a 5 5 5 o o 1 a 1 a a 5 5 5 o o', f'{REPO}/hw2/files/a 5 a 5 a a 5 5 5 o o 1 a 1 a a 5 5 5 o o')

    sendCMD(c, 'put iS7H@TL3g4L?', 'Command succeeded.\n', timeout=TIMEOUT)
    cmpFile(f'{REPO}/hw2/iS7H@TL3g4L?', f'assets/pseudo-server/files-{SERV}/iS7H@TL3g4L?')
    sendCMD(c, 'get iS7H@TL3g4L?', 'Command succeeded.\n', timeout=TIMEOUT)
    cmpFile(f'assets/pseudo-server/files-{SERV}/iS7H@TL3g4L?', f'{REPO}/hw2/files/iS7H@TL3g4L?')

    sendCMD(c, 'quit', 'Bye.\n', timeout=TIMEOUT)

    try:
        sendCMD(c, 'quit', 'Bye.\n', timeout=TIMEOUT)
        exit(1)
    except EOFError:
        print('Client close.')

    delFile(f'{REPO}/hw2/L4RGebUtNoT7o01ArgE')
    delFile(f'{REPO}/hw2/a 5 a 5 a a 5 5 5 o o 1 a 1 a a 5 5 5 o o')
    delFile(f'{REPO}/hw2/iS7H@TL3g4L?')
    delPath(f'{REPO}/hw2/files')