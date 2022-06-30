#!/bin/bash

FAILCOUNT=0

function genrnd() {
  local FILE=$1
  local SIZE=$2
  if ! test -f "$FILE"; then
    dd bs=1000 count=$SIZE iflag=count_bytes </dev/urandom >$FILE
  else
    echo "File $FILE exists, skipping"
  fi
}

function genbad() {
  local FILE=$1
  if ! test -f "$FILE"; then
    ./makebad $1 $2 $3
  else
    echo "File $FILE exists, skipping"
  fi
}

function makefiles() {
  local SUFFIX=$1
  local SIZE=$2
  genrnd "input_$SUFFIX" $SIZE
  genbad "input_${SUFFIX}_bad" $SIZE 
  genbad "input_${SUFFIX}_good" $SIZE "good"
}

function test1() {
# $1 = custom SUFFIX
# $2 = custom MEMLIMIT

  if [ -z $1 ]; then 
    INFILE=""
    OUTFILE=""
    LOGFILE="output.log"
  else
    INFILE="input_$1"
    MEMLIMIT=$2
    if [ -z ${MEMLIMIT} ]; then 
      OUTFILE="output_$1"
    else
      OUTFILE="output_$1_$MEMLIMIT"
    fi
    LOGFILE="${OUTFILE}.log"
  fi

  echo ""
  echo ========================================================================
  echo $INFILE $OUTFILE $MEMLIMIT
  echo ========================================================================
 
  rm -f $OUTFILE
  rm -f $LOGFILE

  # to catch original exit code after tee 
  set -o pipefail
  ./bigsort $INFILE $OUTFILE $MEMLIMIT 2>&1 | tee $LOGFILE
  if [ $? -eq 0 ]; then
    ./bigsort_test $OUTFILE 2>&1 | tee -a $LOGFILE
    if [ $? -ne 0 ]; then
      echo !!!!!!!!!!!!!!!!!!!!!!!!!!!!!! TEST FAILED: bigsort_test
      let FAILCOUNT++
    fi
  else
    echo !!!!!!!!!!!!!!!!!!!!!!!!!!!!!! TEST FAILED: bigsort
    let FAILCOUNT++
  fi
}

cd ./Release

#rm -f *.log

#prepare files if not exist
if ! test -f "input"; then
  genrnd "input"    100000000
fi
makefiles "5m"    5000000
makefiles "50m"   50000000
makefiles "500m"  500000000
makefiles "5000m" 5000000000


test1 
test1 "5m"
test1 "5m_bad"
test1 "5m_good"

test1 "50m"
test1 "50m_bad"
test1 "50m_good"
#special cases
test1 "50m" 1000000
test1 "50m" 100000
test1 "50m" 98000
#intentionally failing cases
#test1 "50m" 98765
#test1 "50m" 10000


test1 "500m"
test1 "500m_bad"
test1 "500m_good"

test1 "5000m"
test1 "5000m_bad"
test1 "5000m_good"

echo FAILCOUNT=$FAILCOUNT
exit $FAILCOUNT
