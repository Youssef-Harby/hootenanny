#!/bin/bash
set -e

# Really basic run script for the hoot core image

IMGNAME=hoot_core

TIMESTAMP=$( date +"%s" )
CONTNAME="hoot_${TIMESTAMP}"
VERSION=latest

#####
if [[ "$(docker images -q ${IMGNAME}:latest)" == "" ]]; then
  echo "ERROR: The ${IMGNAME}:latest docker image does not exist!"
else
  # Add more -v directories as needed
  docker run --rm -it \
   --name=${CONTNAME} \
   --shm-size=1g \
   -v ~:/home/dev \
   ${IMGNAME}:latest
fi

