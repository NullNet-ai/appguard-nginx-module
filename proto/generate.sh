#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

PROTO_FILE="$SCRIPT_DIR/appguard.proto"
OUT_DIR="$SCRIPT_DIR/../src/"
PROTOC_GEN_GRPC_CPP=$(which grpc_cpp_plugin)

protoc -I "$SCRIPT_DIR" \
    --cpp_out="$OUT_DIR" \
    --grpc_out="$OUT_DIR" \
    --plugin=protoc-gen-grpc="$PROTOC_GEN_GRPC_CPP" \
    "$PROTO_FILE"

echo "✅ C++ and gRPC code generated in: $OUT_DIR"
