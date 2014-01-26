#!/bin/bash -e

if [ "$#" -lt 2 ]; then
    cat << EOF
Usage: $0 <json_out_dir> <xhtml_dir> [ <xhtml_dir> .. ]

This script generates elastic search json data.

EOF
    exit 1
fi

MWSD="$(dirname $0)/../build/mwsd"
ELASTIC_SEARCH_OUTPUT="$1"

shift # skip first argument
for xhtml_dir in $@; do
    INCLUDE_PATH_ARGS="$INCLUDE_PATH_ARGS --include-harvest-path $1"
done

$MWSD $INCLUDE_PATH_ARGS                                \
    --recursive                                         \
    --elastic-search-outdir $ELASTIC_SEARCH_OUTPUT      \
    --exit-after-load

