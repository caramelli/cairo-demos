#!/bin/sh

./tiger-demo --antialias=best --benchmark $*
./tiger-demo --antialias=fast --benchmark $*
./tiger-demo --antialias=none --benchmark $*
./chart-demo --benchmark $*
./gradient-demo --benchmark $*
./fish-demo --benchmark $*
./gears-demo --benchmark $*
