#!/bin/bash

gcc -pthread main.c -o main

ROWS=2000
COLS=2000
KSIZE=5
ITERATIONS=10

echo "Starting tests with ROWS=$ROWS, COLS=$COLS, KSIZE=$KSIZE, ITERATIONS=$ITERATIONS"
echo "--------------------------------------------------"

for threads in 1 2 3 4 5 6 7 8 9 10
do
  echo "Thread №: $threads"
  echo "--------------------------------------------------"

  ./main $ROWS $COLS $KSIZE $ITERATIONS $threads &
  CURRENT_PROG_PID=$!
  echo "PID: $CURRENT_PROG_PID"

  while kill -0 $CURRENT_PROG_PID 2>/dev/null; do
    if [ -d "/proc/$CURRENT_PROG_PID/task" ]; then
      LIVE_THREADS=$(ls /proc/$CURRENT_PROG_PID/task | wc -l)
      echo "$(date +%T%3N) - PID $CURRENT_PROG_PID - Live threads: $LIVE_THREADS"
    else
      echo "$(date +%T%3N) - PID $CURRENT_PROG_PID"
      break
    fi
    sleep 0.1
  done

  wait $CURRENT_PROG_PID
  echo "Process $CURRENT_PROG_PID completed."

  echo "--------------------------------------------------"
done

echo "The end."