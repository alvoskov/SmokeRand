#!/bin/sh
npassed=0
nfailed=0
nerror=0

script_dir=$(dirname $0)

for file in $script_dir/../bin/generators/*.so; do
    if [ -f "$file" ]; then
        echo "Processing file: $file"
        $script_dir/../bin/smokerand express $file --report-brief --threads
        if [ $? -eq 0 ]; then
            npassed=$((npassed + 1))
        elif [ $? -eq 1 ]; then
            nfailed=$((nfailed + 1))
        elif [ $? -eq 2 ]; then
            nerror=$((nerror + 1))
        fi
    fi
done

echo Passed: $npassed
echo Failed: $nfailed
echo Error:  $nerror
