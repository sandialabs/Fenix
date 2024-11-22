#!/bin/bash


set -e

echo "Installing apt packages"
sudo apt-get update >/dev/null
sudo apt-get install -y wget git cmake graphviz >/dev/null

echo "Installing Doxygen"
wget https://www.doxygen.nl/files/doxygen-1.12.0.linux.bin.tar.gz >/dev/null
tar -xzf doxygen-1.12.0.linux.bin.tar.gz >/dev/null
export PATH="$PWD/doxygen-1.12.0/bin:$PATH"

#List of branches to build docs for
#TODO: Remove doxygen branch once tested
BRANCHES="doxygen master develop"

build-docs() (
    git checkout $1

    #The CMake Doxygen stuff is weird, and doesn't 
    #properly clean up and/or overwrite old outputs.
    #So to make sure we get the correct doc configs,
    #we need to delete everything
    #We put the docs themselves into a hidden directory
    #so they don't get included in this glob
    rm -rf ./*

    cmake ../ -DBUILD_DOCS=ON -DDOCS_ONLY=ON \
        -DFENIX_DOCS_MAN=OFF -DFENIX_BRANCH=$1 \
        -DFENIX_DOCS_OUTPUT=$PWD/.docs
    make docs
)

git clone https://www.github.com/sandialabs/Fenix.git
mkdir Fenix/build
cd Fenix/build

for branch in $BRANCHES; do
    echo "Building docs for $branch"
    
    #TODO: Fail if any branch fails to build,
    #  once the develop and master branches have doxygen
    #  merged in
    build-docs $branch || true

    echo
    echo
done

if [ -n "$GITHUB_ENV" ]; then
    echo "DOCS_DIR=$PWD/.docs" >> $GITHUB_ENV
fi
