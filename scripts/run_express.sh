#!/bin/sh
npassed=0
nfailed=0
nerror=0

script_dir=$(dirname $0)

for file in $script_dir/../bin/generators/*.so; do
    if [ -f "$file" ]; then
        echo "Processing file: $file"
        $script_dir/../bin/smokerand express $file --report-brief --threads
        case $? in
            0) npassed=$((npassed + 1)) ;;
            1) nfailed=$((nfailed + 1)) ;;
            2) nerror=$((nerror + 1)) ;;
        esac
    fi
done

echo Passed: $npassed
echo Failed: $nfailed
echo Error:  $nerror
