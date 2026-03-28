#!/usr/bin/env bash
mkdir -p build
clang code/main.c -o build/handmade -std=c11 -lSDL3 -ggdb -Wall -Werror
