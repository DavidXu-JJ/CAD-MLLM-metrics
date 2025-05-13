#!/bin/bash

if [ -z "$1" ]; then
	echo "Usage: ./eval.sh /path/to/your/folder"
	exit 1
fi

FOLDER_PATH="$1"

python3 ./scripts/ply2stl.py "$FOLDER_PATH"
./build/bin/mesh_segment "$FOLDER_PATH"
./build/bin/dangling_edge "$FOLDER_PATH"
./build/bin/flux_enclosure_error "$FOLDER_PATH"
./build/bin/self_intersection "$FOLDER_PATH"
