#!/usr/bin/env bash

n=$(tput sgr0)
cyan=$(tput setaf 6)
magenta=$(tput setaf 5)
r=$(tput setaf 1)

binaries=${1:-"/usr/local/bin"}
packages=${2:-"/usr/local/EmojicodePackages"}

self=$0
magicsudod=$3

if [[ "$magicsudod" == "magicsudod" ]]; then
  echo "I’ve super user privileges now and will try to perform the installation."
else
  echo "👨‍💻  Hi, I’m the Emojicode Installer!"

  echo "I’ll copy the ${cyan}Emojicode Compiler${n} and ${magenta}Real-Time Engine${n} to ${binaries}.${n}"
  echo "Then I’ll copy the packages to ${packages}.${n}"
fi

function offerSudo {
  if [[ "$magicsudod" == "magicsudod" ]]; then
    exit 1
  fi
  if [ "$EUID" -eq 0 ]; then
    exit 1
  fi
  echo "I can try to rerun myself with sudo."
  read -p "If you wish me to do so type y. ➡️  " -n 1 -r
  if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo ""
    sudo "$self" "$binaries" "$packages" magicsudod
  fi
  exit 1
}

read -p "If you want to proceed type y. ➡️  " -n 1 -r
if [[ $REPLY =~ ^[Yy]$ ]]; then
  echo
  if [[ ! -w $binaries ]] ; then
    echo "${r}${binaries} is not writeable from this user.${n}"
    offerSudo
  fi

  if [[ ! -w $packages ]] ; then
    pp=$(dirname "$packages")
    if [[ ! -w $pp ]] ; then
      echo "${r}${pp} is not writeable from this user.${n}"
      offerSudo
    else
      if [[ ! -d $packages ]] ; then
        echo "Setting up packages directory in ${packages}${n}"

        mkdir -p "$packages"
      else
        echo "${r}${packages} is not writeable from this user.${n}"
        offerSudo
      fi
    fi
  fi

  (
    set -e
    echo "Copying builds${n}"

    cp emojicode "$binaries/emojicode"
    cp emojicodec "$binaries/emojicodec"

    chmod 755 "$binaries/emojicode" "$binaries/emojicodec"

    echo "Copying packages${n}"

    rsync -rl --copy-dirlinks packages/ "$packages"
    chmod -R 755 "$packages"
  )
  if [ $? = 0 ]
  then
    tput setaf 2
    echo "✅  Emojicode was successfully installed.${n}"
  else
    echo "${r}Installation failed. Please refer to the error above.${n}"
    exit 1
  fi
fi
