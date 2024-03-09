#!/bin/bash

#cl65 -t cx16 -L ../zsound/lib/ -I ../zsound/include -o X16MAZE.PRG x16maze.c mazeutil.o zsound.lib

rm -rf mazeutil.o zsmckit.o X16MAZE.PRG
ca65 -t cx16 -o mazeutil.o x16maze.asm
ca65 -t cx16 -I ../zsmkit/src/ -o zsmckit.o zsmckit.inc
cl65 -t cx16 -C x16maze.cfg \
     -m x16maze.map -Ln x16maze.sym -o X16MAZE.PRG \
     -L ../zsmkit/lib/ \
     x16maze.c mazeutil.o zsmkit.lib zsmckit.o
