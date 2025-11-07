#!/bin/sh
seed=$(cat /dev/urandom | head -c 40 | base64)
./smokerand $1 $2 --seed=$seed --nthreads=4

echo Exit code: $?
echo The used seed: $seed

