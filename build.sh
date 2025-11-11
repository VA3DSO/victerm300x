#!/bin/bash
set -e
cl65 -t vic20 --config vic20-8k.cfg -Cl -O -o victerm.prg -DEXP8K victerm.c
#c1541 victerm.d64 < build.txt

