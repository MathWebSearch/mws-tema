#!/bin/bash -e

if [ "$#" -eq 0 ]; then
    cat << EOF
Usage: $0 <json_file> [ <json_file> ... ]

This script loads data into elastic search.

EOF
    exit 1
fi

JSON_DIR="$1"
HOST="localhost:9200"
INDEX="tema-search"

for json_doc in "$@"; do
    doc_id="$(basename $json_doc)"
    echo curl -XPOST "$HOST/$INDEX/doc/$json_doc" -d "@$json_doc" ...
    curl -XPOST "$HOST/$INDEX/doc/$doc_id" -d "@$json_doc"
    echo
done
