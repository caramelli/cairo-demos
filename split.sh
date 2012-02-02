#/bin/bash
set -m

#$* --split=top-left --glx &
#$* --split=bottom-left --xlib &
#$* --split=bottom-right --ximage &

$* --split=top --xlib &
$* --split=bottom --ximage &

sleep 60
kill -HUP %1 %2
sleep 10
kill %1 %2
