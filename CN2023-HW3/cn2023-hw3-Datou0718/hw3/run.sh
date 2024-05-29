make clean
make kill
make

MY_PORT=$1
if [[ -z "$MY_PORT" ]]; then
    echo "No port specified, using 8000"
    MY_PORT=8000
fi

FILE_SIZE=$2
if [[ -z "$FILE_SIZE" ]]; then
    echo "No file size specified, using 1000"
    FILE_SIZE=1M
fi

ERR=0.1

AGENT_PORT=$(($MY_PORT + 0))
RECV_PORT=$(($MY_PORT + 1))
SEND_PORT=$(($MY_PORT + 2))

# dd if=/dev/urandom bs=$FILE_SIZE count=1 | base64 > input.txt
# truncate -s $FILE_SIZE input.txt
# echo "file generated"

SECONDS=0
./agent $AGENT_PORT local $SEND_PORT local $RECV_PORT $ERR > agent_log.txt &
echo "agent started"
./receiver local $RECV_PORT local $AGENT_PORT output.txt > receiver_log.txt &
echo "receiver started"
./sender local $SEND_PORT local $AGENT_PORT input.txt > sender_log.txt &
echo "sender started"

wait $(jobs -p)

duration=$SECONDS
echo "all done!"
echo "$(($duration / 60)) minutes and $(($duration % 60)) seconds elapsed."
diff input.txt output.txt

