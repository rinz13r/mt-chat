#!/bin/bash

nc=$1
np=$2

eval "bin/test bin/server $nc $np | python3 test/calc.py"
