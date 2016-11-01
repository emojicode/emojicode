#!/usr/bin/env bash

b=$(tput bold)
n=$(tput sgr0)
cyan=$(tput setaf 6)
magenta=$(tput setaf 5)
r=$(tput setaf 1)

binaries=${1:-"/usr/local/bin"}
packages=${2:-"/usr/local/EmojicodePackages"}

echo "${b}🙋  Hi, I’m the Emojicode installer!${n}"

echo "${b}I’ll copy the ${cyan}Emojicode Compiler${n}${b} and ${magenta}Real-Time Engine${n}${b} to ${binaries}.${n}"
echo "${b}Then I’ll copy the packages to ${packages}.${n}"

read -p "${b}If you want to proceed type Y. ➡️ ${n}" -n 1 -r
if [[ $REPLY =~ ^[Yy]$ ]]
then
  echo
  if [[ ! -w $binaries ]] ; then
    echo "${b}${r}${binaries} is not writeable from this user. Carefully try using sudo.${n}"
    exit 1
  fi

  if [[ ! -w $packages ]] ; then
    pp=$(dirname "$packages")
    if [[ ! -w $pp ]] ; then
      echo "${b}${r}${pp} is not writeable from this user. Carefully try using sudo.${n}"
      exit 1
    else
      if [[ ! -d $packages ]] ; then
        echo "${b}Setting up packages directory in ${packages}${n}"

        mkdir -p ${packages}
      else
        echo "${b}${r}${packages} is not writeable from this user. Carefully try using sudo.${n}"
        exit 1
      fi
    fi
  fi

  (
    set -e
    echo "${b}Copying builds${n}"

    cp emojicode ${binaries}/emojicode
    cp emojicodec ${binaries}/emojicodec

    chmod 755 ${binaries}/emojicode ${binaries}/emojicodec

    echo "${b}Copying packages${n}"

    rsync -rl --copy-dirlinks packages/ ${packages}
    chmod -R 755 ${packages}
  )
  if [ $? = 0 ]
  then
    tput setaf 2
    echo "${b}✅  Emojicode was successfully installed.${n}"
  else
    echo "${b}${r}Installation failed. Please refer to the error above.${n}"
    exit 1
  fi
fi


