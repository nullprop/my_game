#!/bin/bash

# Clean
rm -rf ./bin/assets

# Copy assets
cp ./assets/ ./bin/ -r
cp ./src/shaders/standard ./assets/shaders -r
