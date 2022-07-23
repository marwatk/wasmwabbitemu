#!/bin/bash

set -ex

readonly PORT="${PORT:-8081}"
readonly CONTAINER_NAME="${CONTAINER_NAME:-wabbit}"
readonly IMAGE_NAME="${IMAGE_NAME:-wabbit}"

if ! [[ -v SKIP_BUILD ]]; then
  docker build -t "${IMAGE_NAME}" .
fi

ROM_MOUNT_ARGS=()
if [[ -n "${ROM_DIR}" ]]; then
  ROM_MOUNT_ARGS=(-v "${ROM_DIR}:/public/roms:ro,z")
fi

docker kill "${CONTAINER_NAME}" || true
docker run \
  --rm \
  "${ROM_MOUNT_ARGS[@]}" \
  -v "$(pwd -P)/docker/public/index.html:/public/index.html:z" \
  -p "${PORT}:8080" \
  --name "${CONTAINER_NAME}" \
  "${IMAGE_NAME}"
