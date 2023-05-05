#!/bin/bash

rm -rf mazeutil.o X16MAZE.PRG
ca65 -t cx16 -o mazeutil.o x16maze.asm
cl65 -t cx16 -o X16MAZE.PRG x16maze.c mazeutil.o

