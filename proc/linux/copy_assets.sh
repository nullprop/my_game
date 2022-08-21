#!/bin/bash

# Clean
rm -rf ./bin/assets

# Copy assets
cp ./assets/ ./bin/ -r
cp ./src/shaders/standard ./bin/assets/shaders -r
