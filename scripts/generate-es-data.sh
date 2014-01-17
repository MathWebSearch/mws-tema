#!/bin/bash -e

if [ "$#" -ne 2 ]; then
    cat << EOF
Usage: $0 <json_out_dir> <xhtml_dir>

This script generates elastic search json data.

EOF
    exit 1
fi

ELASTIC_SEARCH_OUTPUT="$1"
INCLUDE_PATH="$2"

MWSD="$(dirname $0)/../build/mwsd"
$MWSD                                                   \
    --include-harvest-path $INCLUDE_PATH                \
    --recursive                                         \
    --elastic-search-outdir $ELASTIC_SEARCH_OUTPUT      \
    --exit-after-load

