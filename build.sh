#!/usr/bin/env bash

set -euo pipefail

PROJECT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
cd "$PROJECT_DIR"

git submodule update --init --recursive --depth 1 deps/CGAL

BOOST_DIR="$PROJECT_DIR/deps/boost_1_82_0"
BOOST_ARCHIVE="$PROJECT_DIR/archive/boost_1_82_0.tar.bz2"
BOOST_URL=https://archives.boost.io/release/1.82.0/source/boost_1_82_0.tar.bz2
CGAL_SOURCE_DIR="$PROJECT_DIR/deps/CGAL"

if [ ! -f "$CGAL_SOURCE_DIR/CGALConfig.cmake" ]; then
	echo "CGAL submodule is missing or incomplete: $CGAL_SOURCE_DIR" >&2
	echo "Run: git submodule update --init --recursive --depth 1 deps/CGAL" >&2
	exit 1
fi

if [ ! -f "$BOOST_ARCHIVE" ]; then
	echo "Boost archive not found, downloading..."
	mkdir -p "$PROJECT_DIR/archive"
	wget -O "$BOOST_ARCHIVE" "$BOOST_URL"
fi

if [ ! -d "$BOOST_DIR" ]; then
	echo "Boost directory not found, extracting archive..."
	tar --bzip2 -xf "$BOOST_ARCHIVE" -C ./deps/
else
	echo "Boost directory already exists."
fi

cmake -S "$PROJECT_DIR" -B "$PROJECT_DIR/build" \
	-DCMAKE_BUILD_TYPE=Release \
	-DCGAL_DIR="$CGAL_SOURCE_DIR" \
	-DBOOST_ROOT="$BOOST_DIR"
cmake --build "$PROJECT_DIR/build" --parallel 6
