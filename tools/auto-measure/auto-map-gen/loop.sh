while true; do
  ./run-auto-map-gen.sh
  killall -9 dosbox_x86_64
  sleep 5
  killall -9 dosbox_x86_64
done