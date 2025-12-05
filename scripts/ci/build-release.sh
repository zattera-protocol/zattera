#!/bin/bash
set -e
export IMAGE_NAME="zatterahub/zattera:$BRANCH_NAME"
if [[ $IMAGE_NAME == "zatterahub/zattera:stable" ]] ; then
  IMAGE_NAME="zatterahub/zattera:latest"
fi
docker build --build-arg BUILD_STEP=2 -t=$IMAGE_NAME .
echo "$DOCKER_TOKEN" | docker login --username=$DOCKER_USER --password-stdin
docker push $IMAGE_NAME
# make docker cleanup after itself and delete all exited containers
docker rm -v $(docker ps -a -q -f status=exited) || true