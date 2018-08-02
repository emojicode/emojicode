#!/usr/bin/env bash

n=$(tput sgr0)
cyan=$(tput setaf 6)
bold=$(tput bold)
r=$(tput setaf 1)

binaries=${1:-"/usr/local/bin"}
packages=${2:-"/usr/local/EmojicodePackages"}
include=${3:-"/usr/local/include/emojicode"}

self=$0
magicsudod=$4

if [[ "$magicsudod" == "magicsudod" ]]; then
  echo "I’ve super user privileges now and will try to perform the installation."
else
  echo "👨‍💻  Hi, I’m the Emojicode Installer!"
  echo

  echo "${bold}I’ll copy the ${cyan}Emojicode Compiler${n}${bold} to ${binaries}.${n}"
  echo "${bold}Then I’ll copy the default packages to ${packages}.${n}"
  echo "${bold}Finally, I’ll copy the Emojicode API headers to ${include}.${n}"
  echo "If you prefer different locations you can rerun me and provide me with other locations like so:"
  echo -e "\t ${0} [binary location] [packages location] [include location]\n"
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
    sudo "$self" "$binaries" "$packages" "$include" magicsudod
    exit $?
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

    cp emojicodec "$binaries/emojicodec"

    chmod 755 "$binaries/emojicodec"

    echo "Copying packages${n}"

    rsync -rl packages/ "$packages"
    rsync -rl include/ "$include"
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
