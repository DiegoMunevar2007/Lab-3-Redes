#!/bin/bash
set -e

# Absolute path for temporary clone
MSQUIC_SRC="/tmp/msquic"
MSQUIC_BUILD="$MSQUIC_SRC/build"

refresh_yarn_apt_key_if_needed() {
	# If Yarn repo exists, refresh its key to avoid NO_PUBKEY failures on apt update.
	if grep -Rqs "dl.yarnpkg.com/debian" /etc/apt/sources.list /etc/apt/sources.list.d 2>/dev/null; then
		echo "==> Refreshing Yarn APT key (if needed)..."
		if command -v curl >/dev/null 2>&1 && command -v gpg >/dev/null 2>&1; then
			sudo mkdir -p /usr/share/keyrings
			curl -fsSL https://dl.yarnpkg.com/debian/pubkey.gpg | \
				sudo gpg --dearmor -o /usr/share/keyrings/yarn-archive-keyring.gpg
		else
			echo "==> Warning: curl/gpg not available; cannot refresh Yarn key automatically"
		fi
	fi
}

echo "==> Updating packages and installing prerequisites..."
refresh_yarn_apt_key_if_needed
sudo apt update
# Added liblttng-ust-dev as it is a standard dependency for MSQuic on Linux
sudo apt install -y git cmake build-essential ninja-build clang libssl-dev liblttng-ust-dev

echo "==> Cloning MSQuic repository with submodules..."
# Removing old directory if it exists to avoid git errors
rm -rf "$MSQUIC_SRC"
git clone --recurse-submodules https://github.com/microsoft/msquic.git "$MSQUIC_SRC"

echo "==> Creating build directory..."
mkdir -p "$MSQUIC_BUILD"

echo "==> Configuring build with CMake..."
cd "$MSQUIC_BUILD"
# Using Release build and Ninja generator
cmake "$MSQUIC_SRC" -G Ninja -DCMAKE_BUILD_TYPE=Release

echo "==> Building MSQuic..."
# Using ninja to compile the project
ninja

echo "==> Installing headers and library..."
# Create a dedicated include directory to prevent header pollution
sudo mkdir -p /usr/local/include/msquic

# Copy ALL header files from the include source
# This solves the 'msquic_posix.h' missing error
sudo cp "$MSQUIC_SRC"/src/inc/*.h /usr/local/include/msquic/

# Install the shared object library
# Path usually follows the pattern: bin/Release/libmsquic.so
sudo cp "$MSQUIC_BUILD"/bin/Release/libmsquic.so /usr/local/lib/
sudo ldconfig

echo "==> Cleaning up..."
rm -rf "$MSQUIC_SRC"

echo "==> MSQuic installation complete!"
echo "---------------------------------------------------------"
echo "Headers installed in: /usr/local/include/msquic"
echo "Library installed in: /usr/local/lib/libmsquic.so"
echo ""
echo "To compile your project, use the following command:"
echo "gcc publisher.c -o publisher -I/usr/local/include/msquic -lmsquic -lssl -lcrypto"
echo "---------------------------------------------------------"