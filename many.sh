#/bin/bash
set -m

./bubble-demo		--split=0 $* &
./chart-demo		--split=1 $* &
./chart-demo --vertical	--split=2 $* &
./dragon-demo 		--split=3 $* &
./fish-demo 		--split=4 $* &
./flowers-demo		--split=5 $* &
./gears-demo		--split=6 $* &
./gradient-demo		--split=7 $* &
./maze-demo		--split=8 $* &
./slideshow-demo	--split=9 $* &
./spinner-demo		--split=10 $* &
./spiral-demo		--split=11 $* &
./tiger-demo		--split=12 $* &
./waterfall-demo	--split=13 $* &
./wave-demo		--split=14 $* &
./flowers-demo --naive	--split=15 $* &

sleep 60
kill -HUP %1 %2 %3 %4 %5 %6 %7 %8 %9 %10 %11 %12 %13 %14 %15 %16
sleep 10
kill %1 %2 %3 %4 %5 %6 %7 %8 %9 %10 %11 %12 %13 %14 %15 %16
