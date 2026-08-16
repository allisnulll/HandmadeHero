#!/usr/bin/env bash

prod=$1

set -euo pipefail
mkdir -p build

cflags=(
    -std=c11 -lSDL3 -lm -ggdb -O2
    \
    -Wall -Wextra -Wpedantic
    -Werror=format-security -Werror=implicit
    -Werror=incompatible-pointer-types -Werror=int-conversion
    -Wconversion -Wsign-conversion -Wshadow -Wundef -Wimplicit-fallthrough
    -Wstrict-prototypes -Wmissing-prototypes -Wold-style-definition
    -Wformat -Wformat=2 -Wformat-overflow=2 -Wformat-truncation=2
    -Wnull-dereference -Wnonnull-compare
    -Wuninitialized -Winit-self -Wmaybe-uninitialized
    -Wfloat-equal -Wwrite-strings
    -Wjump-misses-init -Wlogical-op -Waggregate-return
    -Wstack-protector -Wtrampolines -Wbidi-chars=any
    \
    -fno-delete-null-pointer-checks
    -fno-strict-overflow
    -fno-strict-aliasing
    -fstrict-flex-arrays=3
    -fstack-clash-protection -fstack-protector-strong
    -fzero-init-padding-bits=all
    -fcf-protection=full
    -fPIE -pie
    -fanalyzer
    \
    -U_FORTIFY_SOURCE
    \
    "-Wl,-z,nodlopen" "-Wl,-z,noexecstack"
    "-Wl,-z,relro" "-Wl,-z,now"
    "-Wl,--as-needed" "-Wl,--no-copy-dt-needed-entries"
    \
    src/platform.c src/handmade.c
)

if [ "$prod" ]; then
    printf "\e[38;5;52mPRODUCTION\n\e[0m"
    cflags+=(
        -D_FORTIFY_SOURCE=3
        -ftrivial-auto-var-init=zero
    )
else
    cflags+=(
        -Werror
        "-fsanitize=address,undefined"
        -fno-omit-frame-pointer
    )
fi

gcc -o build/handmade "${cflags[@]}"
