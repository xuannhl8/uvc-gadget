#!/bin/bash
readonly TARGET_IP="$1"
readonly PROGRAM="$2"
readonly TARGET_DIR="/home/root"
# current dircectory
# /home/dinh/imx-docker/Documents/uvc-gadget = ${PWD}
readonly LOCAL_PROGRAM_DIR="${PWD}/build/src" 

echo "Deploying to target"
# send lib
scp -i /home/dinh/.ssh/id_rsa /home/dinh/imx-docker/Documents/uvc-gadget/build/lib/libuvcgadget.so.0.4.0 root@${TARGET_IP}:/usr/lib
scp -i /home/dinh/.ssh/id_rsa /home/dinh/imx-docker/Documents/uvc-gadget/build/lib/libuvcgadget.so root@${TARGET_IP}:/usr/lib
scp -i /home/dinh/.ssh/id_rsa /home/dinh/imx-docker/Documents/uvc-gadget/build/lib/libuvcgadget.so.0 root@${TARGET_IP}:/usr/lib
# send the program to the target
scp -i /home/dinh/.ssh/id_rsa ${LOCAL_PROGRAM_DIR}/${PROGRAM} root@${TARGET_IP}:${TARGET_DIR}