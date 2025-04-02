#!/bin/bash
SERVICE_PATH=/usr/lib/systemd/system
SERVICE_NAME=sysDetector.service

if [[ $1 == "install" ]]; then
  cp -af $SERVICE_NAME" "$SERVICE_PATH || exit 1
elif [[ $1 == "remove" ]]
  rm -f $SERVICE_PATH/$SERVICE_NAME || exit 1
fi