#!/bin/bash

echo; echo

PATH=$srcdir/data:.:$PATH

mkdir -p results

echo "Checking no args..."
xmlforeach 2>/dev/null
if [ $? -ne 1 ]; then exit 1; fi

echo "Checking missing command arg..."
xmlforeach //block 2>/dev/null
if [ $? -ne 1 ]; then exit 1; fi

echo "Checking bogus arg..."
xmlforeach -bogus //block path.sh 2>/dev/null
if [ $? -ne 1 ]; then exit 1; fi

echo "Checking non-existant script..."
cat $srcdir/data/tiny.xml | xmlforeach //block pathhh.sh 2>/dev/null
if [ $? -ne 127 ]; then exit 1; fi

echo "Checking non-executable script"
cat $srcdir/data/tiny.xml | xmlforeach //block tiny.xml 2>/dev/null
if [ $? -ne 126 ]; then exit 1; fi

echo "Checking failed script"
cat $srcdir/data/tiny.xml | xmlforeach //block false 2>/dev/null
if [ $? -ne 123 ]; then exit 1; fi

echo "Checking failed script with maxprocs = 2"
xmlforeach -S -f $srcdir/data/tiny.xml  -t -P 2 //block false
if [ $? -ne 123 ]; then exit 1; fi

set -e

echo "Checking the simplest xml file..."
cat $srcdir/data/tiny.xml | xmlforeach -S -t //block path.sh > results/tiny.path

echo "Checking strange seg-fault"
cat $srcdir/data/failed | xmlforeach -S //block -- true

echo "Output is correct? ..."
diff -u results/tiny.path $srcdir/data/golden/tiny.path
