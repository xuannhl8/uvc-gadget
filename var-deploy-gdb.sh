#!/bin/bash
readonly TARGET_IP="$1"
readonly PROGRAM="$2"
readonly TARGET_DIR="/home/root"
# kill gdbserver on target and delete old binary
ssh -i /home/dinh/.ssh/id_rsa root@${TARGET_IP} "sh -c '/usr/bin/killall -q gdbserver; exit 0'"

# Must match endsPattern in tasks.json
echo "Starting GDB Server on Target"

# start gdbserver on target
ssh -t -i /home/dinh/.ssh/id_rsa root@${TARGET_IP} "sh -c 'cd ${TARGET_DIR}; gdbserver localhost:3000 ${PROGRAM} -i 10s.mjpeg.avi'"