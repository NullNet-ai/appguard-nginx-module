#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUT_DIR="$SCRIPT_DIR/../src/generated"
PROTOC_GEN_GRPC_CPP=$(which grpc_cpp_plugin)

PROTO_FILES=(
    "$SCRIPT_DIR/appguard.proto"
    "$SCRIPT_DIR/commands.proto"
)

protoc -I "$SCRIPT_DIR"                             \
    --cpp_out="$OUT_DIR"                            \
    --grpc_out="$OUT_DIR"                           \
    --plugin=protoc-gen-grpc="$PROTOC_GEN_GRPC_CPP" \
    --experimental_allow_proto3_optional            \
    "${PROTO_FILES[@]}"

echo "✅ C++ and gRPC code generated in: $OUT_DIR"