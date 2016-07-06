#!/usr/bin/env bash

b=$(tput bold)
n=$(tput sgr0)

if [[ ! -w "/usr/local/" ]] ; then
	tput setaf 5
	echo "${b}/usr/local/ is not writeable from this user. Carefully try using sudo."
	tput sgr0
	exit 1
fi

if [[ ! -w "/usr/local/bin" ]] ; then
	tput setaf 5
	echo "${b}/usr/local/bin is not writeable from this user. Carefully try using sudo."
	tput sgr0
	exit 1
fi

echo "${b}Copying builds to /usr/local/bin/${n}"

cp emojicode /usr/local/bin/emojicode
cp emojicodec /usr/local/bin/emojicodec

chmod 755 /usr/local/bin/emojicode /usr/local/bin/emojicodec

echo "${b}Setting up packages directory in /usr/local/EmojicodePackages${n}"

mkdir -p /usr/local/EmojicodePackages

echo "${b}Copying s Package header${n}"

mkdir -p /usr/local/EmojicodePackages/s-v1
rm -f /usr/local/EmojicodePackages/s
ln -s /usr/local/EmojicodePackages/s-v1 /usr/local/EmojicodePackages/s
cp -f headers/s.emojic /usr/local/EmojicodePackages/s-v1/header.emojic

function copyPackage {
  echo "${b}Copying Package $1${n}"
  mkdir -p /usr/local/EmojicodePackages/$1-v$2
  rm -f /usr/local/EmojicodePackages/$1
  ln -s /usr/local/EmojicodePackages/$1-v$2 /usr/local/EmojicodePackages/$1
  cp -f headers/$1.emojic /usr/local/EmojicodePackages/$1-v$2/header.emojic
  cp -f $1.so /usr/local/EmojicodePackages/$1-v$2/$1.so
}

copyPackage files 0
copyPackage SDL 0
copyPackage sockets 0

chmod -R 755 /usr/local/EmojicodePackages

tput setaf 2
echo "${b}If there are no errors above Emojicode was successfully installed."
tput sgr0
