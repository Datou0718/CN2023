from pathlib import Path
import sys
import os

def checkFile(file, expect=True, aliases=[]):

    if Path(file).is_file() is not expect:
        for alias in aliases:
            if Path(alias).is_file() is expect:
                return 0
        print("missing:   " if expect else "unexpected:", file)
        return 1
    return 0

def checkDir(dir, expect=True):
    if Path(dir).is_dir() is not expect:
        print("missing:   " if expect else "unexpected:", dir)
        return 1
    return 0

def checkFileR(file, root='.', expect=True):
    files = list(Path(root).rglob(f'{file}'))
    if bool(len(files)) is not expect:
        print("missing:   " if expect else "unexpected:", files)
        return 1
    return 0

def checkFileExtR(ext, root='.', expect=True):
    files = list(Path(root).rglob(f'*{ext}'))
    if bool(len(files)) is not expect:
        print("missing:   " if expect else "unexpected:", files)
        return 1
    return 0

if __name__ == '__main__':
    if len(sys.argv) == 2:
        os.chdir(sys.argv[1])
        print('Change to', sys.argv[1])

    retval = 0

    retval += checkFile('hw2/makefile', expect=True, aliases=['hw2/Makefile'])
    retval += checkFile('hw2/server.c', expect=True, aliases=['hw2/server.cpp'])
    retval += checkFile('hw2/client.c', expect=True, aliases=['hw2/client.cpp'])
    
    retval += checkFile('hw2/client', expect=False)
    retval += checkFile('hw2/server', expect=False)
    retval += checkFile('hw2/secret', expect=False)

    retval += checkDir('hw2/web/files', expect=False)
    retval += checkDir('hw2/web/tmp', expect=False)
    retval += checkDir('hw2/web/videos', expect=False)

    retval += checkFileR('.vscode', expect=False)
    retval += checkFileR('__pycache__', expect=False)
    retval += checkFileR('node_modules', expect=False)
    retval += checkFileR('__MACOSX', expect=False)
    retval += checkFileR('Thumbs.db', expect=False)

    retval += checkFileExtR('.DS_Store', expect=False)

    retval += checkFileExtR('.d', expect=False)
    retval += checkFileExtR('.slo', expect=False)
    retval += checkFileExtR('.lo', expect=False)
    retval += checkFileExtR('.o', expect=False)
    retval += checkFileExtR('.ko', expect=False)
    retval += checkFileExtR('.obj', expect=False)
    retval += checkFileExtR('.elf', expect=False)
    retval += checkFileExtR('.ilk', expect=False)
    retval += checkFileExtR('.map', expect=False)
    retval += checkFileExtR('.exp', expect=False)
    retval += checkFileExtR('.gch', expect=False)
    retval += checkFileExtR('.pch', expect=False)
    retval += checkFileExtR('.so', expect=False)
    retval += checkFileExtR('.so.*', expect=False)
    retval += checkFileExtR('.dylib', expect=False)
    retval += checkFileExtR('.dll', expect=False)
    retval += checkFileExtR('.lai', expect=False)
    retval += checkFileExtR('.la', expect=False)
    retval += checkFileExtR('.a', expect=False)
    retval += checkFileExtR('.lib', expect=False)
    retval += checkFileExtR('.exe', expect=False)
    retval += checkFileExtR('.out', expect=False)
    retval += checkFileExtR('.app', expect=False)
    retval += checkFileExtR('.su', expect=False)
    retval += checkFileExtR('.idb', expect=False)
    retval += checkFileExtR('.pdb', expect=False)

    retval += checkFileExtR('~', expect=False)
    retval += checkFileExtR('.sw*', expect=False)

    retval += checkFileExtR('.mp4', expect=False)
    retval += checkFileExtR('.m4s', expect=False)
    retval += checkFileExtR('.mpd', expect=False)

    retval += checkFileExtR('.tmp', expect=False)
    
    assert(retval == 0)
    print('all is well')