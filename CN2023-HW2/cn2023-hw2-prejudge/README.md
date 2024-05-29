# cn2023-hw2-prejudge

## Usage

將這個 repository clone 到你的 container 裡面：

```shell
# in hw2 container
$ mkdir mytest
$ cd mytest
$ git clone https://github.com/ntucn/cn2023-hw2-prejudge.git
$ git clone <your_repository> # ensure it is clean
$ cd cn2023-hw2-prejudge
$ ./install_packages.sh # only need to do it once; chmod u+x *.sh  if needed
$ ./run.sh ../<your_repository> # e.g. ../cn2023-hw2-yourgithubid (without "/" in the end)
```

## Sample Output

```
-----summary-----
structure-check                          :   pass
server-test                 (server-0.py):   pass
server-test                 (server-1.py):   pass
client-test                 (client-0.py):   pass
client-test                 (client-1.py):   pass
client-test-with-odd-server (client-0.py):   pass
```

有任何問題可以寄信到 `ntu.cnta@gmail.com`

