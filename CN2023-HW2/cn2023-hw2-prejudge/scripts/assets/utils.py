import filecmp
import shutil
import os

def genFile(filename, size='1M'):
    os.system(f'dd if=/dev/urandom of="{filename}" bs={size} count=1 > /dev/null 2>&1')

def cmpFile(fileA, fileB):
    assert(filecmp.cmp(fileA, fileB))

def delFile(filename):
    try:
        os.remove(filename)
    except:
        pass
   
def delPath(path): 
    shutil.rmtree(path, ignore_errors=True)