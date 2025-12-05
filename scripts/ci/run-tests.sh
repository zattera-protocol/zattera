#!/bin/bash
set -e
docker build --build-arg BUILD_STEP=1 -t=zatterahub/zattera:tests .
docker run -v $WORKSPACE:/var/workspace zatterahub/zattera:tests cp -r /var/cobertura /var/workspace
# make docker cleanup after itself and delete all exited containers
docker rm -v $(docker ps -a -q -f status=exited) || true