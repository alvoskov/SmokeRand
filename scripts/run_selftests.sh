#!/bin/sh
npassed=0
nfailed=0
nunknown=0

for file in generators/*.so; do
    if [ -f "$file" ]; then
        echo "Processing file: $file"
        ./smokerand selftest $file
        if [ $? -eq 0 ]; then
            npassed=$((npassed + 1))
        elif [ $? -eq 1 ]; then
            nfailed=$((nfailed + 1))
        elif [ $? -eq 2 ]; then
            nunknown=$((nunknown + 1))
        fi
    fi
done

echo Passed:          $npassed
echo Failed:          $nfailed
echo Not implemented: $nunknown
