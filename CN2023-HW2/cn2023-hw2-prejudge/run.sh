#!/bin/bash

# Regular Colors
BLACK='\033[0;30m'
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
WHITE='\033[0;37m'
# Reset
COLOROFF='\033[0m'

function checkResult (){
  if [ $2 -eq 0 ]; then
    echo -e "${GREEN}${1}:   pass${COLOROFF}"
  else
    echo -e "${RED}${1}: failed${COLOROFF}"
  fi
}

if [ -z "$1" ]; then
    echo "You need to pass the path to 'repository' (not 'hw2') folder to start the script."
    exit 1
fi

SCRIPT_PATH=`pwd`
REPO_PATH=$1
cd ${REPO_PATH}
if [ $? -ne 0 ]; then
  exit 1
fi
REPO_PATH=`pwd`
echo ${REPO_PATH}

cd ${SCRIPT_PATH}
rm -rf .prejudge
mkdir .prejudge
/bin/cp -rf scripts/* .prejudge
cd .prejudge
TEST_PATH=`pwd`

echo -e "${YELLOW}-----startup-----${COLOROFF}"
killall -9 server > /dev/null 2>&1
killall -9 node > /dev/null 2>&1
killall -9 python > /dev/null 2>&1

cd ${REPO_PATH}/hw2
make clean

echo "done."
echo -e "${YELLOW}-----structure-check-----${COLOROFF}"
cd ${TEST_PATH}
python structure-checker.py ${REPO_PATH}
SC_RET=$?

echo -e "${YELLOW}-----build-----${COLOROFF}"
cd ${REPO_PATH}/hw2
make

echo "done."
echo -e "${YELLOW}-----server-test-----${COLOROFF}"
/bin/cp -f ${TEST_PATH}/assets/secret ${REPO_PATH}/hw2
cd ${REPO_PATH}/hw2 && ./server 8080 > /dev/null 2>&1 &
SERVER_8080=$!
cd ${TEST_PATH} && python server-0.py 8080 ${REPO_PATH}
SVR0_RET=$?
cd ${REPO_PATH}/hw2 && ./server 7777 > /dev/null 2>&1 &
SERVER_7777=$!
cd ${TEST_PATH} && python server-1.py 7777 ${REPO_PATH}
SVR1_RET=$?

rm -f ${REPO_PATH}/hw2/secret
echo -e "${YELLOW}-----client-test-----${COLOROFF}"
# echo "${TEST_PATH}/assets/pseudo-server"
cd ${TEST_PATH}/assets/pseudo-server
npm ci > /dev/null 2>&1
node app.js > /dev/null 2>&1 &
echo "Wait for server to wake up..."
sleep 5
cd ${TEST_PATH} && python client-0.py 4500 ${REPO_PATH} nodejs
CLI0_RET=$?
cd ${TEST_PATH} && python client-1.py 4500 ${REPO_PATH} nodejs
CLI1_RET=$?

echo -e "${YELLOW}-----client-test-with-odd-server-----${COLOROFF}"
cd ${TEST_PATH}/assets/pseudo-server
python web.py > /dev/null 2>&1 &
echo "Wait for server to wake up..."
sleep 5
cd ${TEST_PATH} && python client-0.py 2023 ${REPO_PATH} flask
CLI2_RET=$?

echo -e "${YELLOW}-----clean-up-----${COLOROFF}"
killall -9 server > /dev/null 2>&1
killall -9 node > /dev/null 2>&1
killall -9 python > /dev/null 2>&1
echo "done."
echo -e "${YELLOW}-----summary-----${COLOROFF}"

checkResult "structure-check                          " ${SC_RET}
checkResult "server-test                 (server-0.py)" ${SVR0_RET}
checkResult "server-test                 (server-1.py)" ${SVR1_RET}
checkResult "client-test                 (client-0.py)" ${CLI0_RET}
checkResult "client-test                 (client-1.py)" ${CLI1_RET}
checkResult "client-test-with-odd-server (client-0.py)" ${CLI2_RET}