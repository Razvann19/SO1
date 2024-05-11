#!/bin/bash

file_path="$1"

# Afișează permisiunile fișierului pentru diagnosticare
ls -l "$file_path"

# Cuvinte cheie suspecte
keywords=("corrupted" "dangerous" "risk" "attack" "malware" "malicious")

for keyword in "${keywords[@]}"; do
    if grep -q "$keyword" "$file_path"; then
        echo "Malicious content found in $file_path"
        exit 1
    fi
done

echo "No malicious content found in $file_path"
exit 0