#!/bin/bash
rm -rf egl-demo
gcc -o egl-demo  -lX11 -lEGL -lGLESv2  -lm   egl.c
