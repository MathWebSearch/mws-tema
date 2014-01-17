#!/bin/bash -e

if [ "$#" -ne 1 ]; then
    cat << EOF
Usage: $0 <json_dir>

This script loads data intro elastic search.

EOF
    exit 1
fi

JSON_DIR="$1"
HOST="localhost:9200"
INDEX="tema-search"

for json_doc in $JSON_DIR/*; do
    doc_id="$(basename $json_doc)"
    echo curl -XPOST "$HOST/$INDEX/doc/$json_doc" -d "@$json_doc" ...
    curl -XPOST "$HOST/$INDEX/doc/$doc_id" -d "@$json_doc"
    echo
done
