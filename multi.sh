#/bin/bash
set -m

for i in `seq 0 15`; do
	$* --split=$i &
done

sleep 60
kill -HUP %1 %2 %3 %4 %5 %6 %7 %8 %9 %10 %11 %12 %13 %14 %15 %16
sleep 10
kill %1 %2 %3 %4 %5 %6 %7 %8 %9 %10 %11 %12 %13 %14 %15 %16
