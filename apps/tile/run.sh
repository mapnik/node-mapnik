#!/bin/sh

# https://github.com/senchalabs/spark
spark -p 8000 -n `sysctl hw.ncpu | awk '{print $2}'`