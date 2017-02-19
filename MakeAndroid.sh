#!/bin/bash

# if the make file is unable to locate the location
# of the ndk, uncomment the line bellow and set
# the correct path to the android sdk, make sure ndk is installed
#PATH="<path_to_android_sdk>/ndk-bundle:$PATH"

if ! [ -x "$(command -v ndk-build -v)" ]; then
  echo 'Error: Unable to locate ndk-build, edit make file, or add to PATH' >&2
  exit 1
fi

echo "cleaning..."
rm -rf ndk-project
mkdir ndk-project
mkdir ndk-project/jni

echo "copying source to project..."
cp -r ./EmojicodeCompiler ./ndk-project/jni/EmojicodeCompiler
cp -r ./EmojicodeReal-TimeEngine ./ndk-project/jni/EmojicodeReal-TimeEngine
find . -maxdepth 1 -name \*.h -exec cp '{}' 'ndk-project/jni/' ';'

cp Android.mk ndk-project/jni

# when using the configuration of the make file it wont compile it correctly
cd ndk-project
cp ./jni/EmojicodeReal-TimeEngine/utf8.c ./jni/EmojicodeCompiler/utf8.cpp
cp ./jni/EmojicodeReal-TimeEngine/utf8.h ./jni/EmojicodeCompiler/utf8.h


echo "building..."
ndk-build NDK_PROJECT_PATH=. APP_STL=gnustl_static
