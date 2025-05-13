git submodule update --init --recursive

BOOST_DIR=./deps/boost_1_82_0
BOOST_ARCHIVE=./archive/boost_1_82_0.tar.bz2
BOOST_URL=https://archives.boost.io/release/1.82.0/source/boost_1_82_0.tar.bz2

if [ ! -f "$BOOST_ARCHIVE" ]; then
	echo "Boost archive not found, downloading..."
	mkdir archive
	wget -O "$BOOST_ARCHIVE" "$BOOST_URL"
fi

if [ ! -d "$BOOST_DIR" ]; then
	echo "Boost directory not found, extracting archive..."
	tar --bzip2 -xf "$BOOST_ARCHIVE" -C ./deps/
else
	echo "Boost directory already exists."
fi

cmake -B build -DCGAL_DIR=./deps/cgal -DBOOST_ROOT=$BOOST_DIR
cd build
make -j6
cd ..
