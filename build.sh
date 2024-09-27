#!/bin/bash

#cl65 -t cx16 -L ../zsound/lib/ -I ../zsound/include -o X16MAZE.PRG x16maze.c mazeutil.o zsound.lib

rm -rf mazeutil.o zsmckit.o X16MAZE.PRG
#Build assembly helper-functions
ca65 -t cx16 -o mazeutil.o x16maze.asm
#Build ZSM C library
ca65 -t cx16 -I ../zsmkit/src/ -o zsmckit.o zsmckit.inc
#Build and link main program
cl65 -t cx16 -C x16maze.cfg \
     -m x16maze.map -Ln x16maze.sym -o X16MAZE.PRG \
     -L ../zsmkit/lib/ \
     x16maze.c mazeutil.o zsmkit.lib zsmckit.o
