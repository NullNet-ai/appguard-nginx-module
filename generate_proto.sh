#!/bin/bash

PWD=$(pwd)

cd proto || exit 1

python3 ../vendor/nanopb/generator/nanopb_generator.py appguard.proto

cd "$PWD"