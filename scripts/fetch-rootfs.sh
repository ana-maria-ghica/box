#!/bin/bash

curl -O https://dl-cdn.alpinelinux.org/alpine/v3.24/releases/x86_64/alpine-minirootfs-3.24.2-x86_64.tar.gz
mkdir -p alpine
tar -xzf alpine-minirootfs-3.24.2-x86_64.tar.gz -C alpine 
