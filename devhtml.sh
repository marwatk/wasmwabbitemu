#!/bin/bash

set -ex

readonly PORT="${PORT:-8081}"
readonly CONTAINER_NAME="${CONTAINER_NAME:-wabbit}"
readonly IMAGE_NAME="${IMAGE_NAME:-wabbit}"

if ! [[ -v SKIP_BUILD ]]; then
  docker build -t "${IMAGE_NAME}" .
fi

docker kill "${CONTAINER_NAME}" || true
docker run -d --rm -v "$(pwd -P)/index.html:/opt/src/bin/index.html" -p "${PORT}:8080" --name "${CONTAINER_NAME}" "${IMAGE_NAME}"
