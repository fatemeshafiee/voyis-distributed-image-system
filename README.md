# Voyis Distributed Image System

## Overview
This repository contains a three-stage demo pipeline that streams images, enriches them with SIFT-based feature metadata, and archives the result in SQLite. Each stage is a standalone C++20 executable that communicates over ZeroMQ IPC sockets:

- `image_generator`: Scans a folder for images, encodes each frame as PNG, and publishes metadata + pixels over `ipc:///tmp/voyis-image-stream.ipc`.
- `feature_extractor`: Subscribes to the image stream, runs SIFT to generate keypoint metadata, and forwards both the metadata and image buffer to `ipc:///tmp/voyis-feature-stream.ipc`.
- `data_logger`: Consumes the feature stream and persists every frame inside `voyis_frames.db` (creating the database/table on demand).

Running all three binaries concurrently creates an end-to-end loop that mimics a distributed perception system.

## Requirements
- C++20 toolchain (tested with CMake 3.16+)
- [ZeroMQ](https://zeromq.org/) development headers (`libzmq`)
- [OpenCV](https://opencv.org/) (for image encode/decode + SIFT)
- [SQLite3](https://www.sqlite.org/)
- [nlohmann/json](https://github.com/nlohmann/json)

See the `Dockerfile` for exact package names on Ubuntu 22.04.

## Build
```bash
cmake -S . -B build
cmake --build build
```

The build produces the following binaries inside `build/`:

- `image_generator/image_generator`
- `feature_extractor/feature_extractor`
- `data_logger/data_logger`

## Build with Docker
The provided `Dockerfile` installs all dependencies on Ubuntu 22.04. Build an image and run the three apps inside separate containers or with `docker exec` shells:

```bash
docker build -t voyis-demo .
```

Start a development container that mounts the current repo and opens an interactive shell:

```bash
sudo docker run -it --rm \
  -v "$PWD":/workspace \
  -w /workspace \
  --name voyis-dev-cont \
  voyis-demo \
  bash
```

Keep the container running while you build the binaries. If you detach and need another shell, reattach with:

```bash
sudo docker exec -it voyis-dev-cont bash
```

Run the applications from within the container using the commands in the next section.

## Running the Pipeline
1. **Start the logger** (first consumer, so upstream senders do not block):
   ```bash
   ./build/data_logger/data_logger
   ```
2. **Start the feature extractor**:
   ```bash
   ./build/feature_extractor/feature_extractor
   ```
3. **Feed images** by pointing the generator to a folder that contains `.png`, `.jpg`, `.jpeg`, or `.bmp` files:
   ```bash
   ./build/image_generator/image_generator /path/to/images
   ```

Use separate terminals for each binary. All IPC sockets are created under `/tmp`, and each binary unlinks its socket path before binding, so you normally do not need manual cleanup. If the applications exit unexpectedly, ensure `/tmp/voyis-image-stream.ipc` and `/tmp/voyis-feature-stream.ipc` are removed before restarting.

## Notes
- The IPC topology replaces the original TCP bindings, which avoids picking free ports and keeps all communication on the local machine by using ZeroMQ IPC sockets under `/tmp`.
- `voyis_frames.db` is created in the working directory of `data_logger`. Inspect it with the `sqlite3` CLI to validate captured rows.
