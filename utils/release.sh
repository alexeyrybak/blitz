#!/bin/bash

cd ..

if [ ! -x ./release ]; then
    echo "ERROR: create ./release directory"
    exit
fi

cd release

if [ ! -x ./last ]; then
    echo "ERROR: create ./release/last directory"
    exit
fi

cd last

echo "Deleting old repository in release/last...";
rm -rf ./*
echo "Getting the last version from git..."
git clone git@git.mlan:blitz.git

echo "Extracting version..."
version=`php ../../utils/version.php`
echo "Version: " $version
read -p "Correct [Y/n]?" response

if [ $response=~"^(Y|y| )" ]; then
    rm -rf blitz/.cvsignore
    rm -rf blitz/.git
    if [-r blitz-$version]; then
        rm -rf blitz-$version
    fi
    mv blitz blitz-$version
    tar zcvf blitz-$version.tar.gz blitz-$version
    mv blitz-$version.tar.gz ../
    echo "created tarball: " blitz-$version.tar.gz
fi

cd ../..
