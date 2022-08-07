#!/bin/bash

cd bin
valgrind --leak-check=full --track-origins=yes ./game 2>&1 | tee valgrind.txt
echo "log saved to valgrind.txt"
