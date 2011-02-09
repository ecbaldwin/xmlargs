#!/bin/bash

echo; echo

PATH=$srcdir/data:.:$PATH

mkdir -p results

echo "Checking no args..."
xmltsort 2>/dev/null
if [ $? -ne 1 ]; then exit 1; fi

echo "Checking missing command arg..."
xmltsort //block 2>/dev/null
if [ $? -ne 1 ]; then exit 1; fi

echo "Checking bogus arg..."
xmltsort -bogus //block /block/name /block/hierarchy/child/name path.sh 2>/dev/null
if [ $? -ne 1 ]; then exit 1; fi

echo "Checking non-existant script..."
cat $srcdir/data/tiny.xml | xmltsort //block /block/name /block/hierarchy/child/name pathhh.sh 2>/dev/null
if [ $? -ne 127 ]; then exit 1; fi

echo "Checking non-executable script"
cat $srcdir/data/tiny.xml | xmltsort //block /block/name /block/hierarchy/child/name tiny.xml 2>/dev/null
if [ $? -ne 126 ]; then exit 1; fi

echo "Checking failed script"
xmltsort -f $srcdir/data/tiny.xml -E errors.xml //block /block/name /block/hierarchy/child/name false 2>/dev/null
if [ $? -ne 123 ]; then exit 1; fi
test -s errors.xml
xmllint --noout errors.xml

echo "Checking failed script with maxprocs = 2"
cat $srcdir/data/tiny.xml | xmltsort -StP 2 //block /block/name /block/hierarchy/child/name false
if [ $? -ne 123 ]; then exit 1; fi

echo "Checking failed script with cycle"
cat $srcdir/data/small-cycle.xml | xmltsort -S //block /*/name /*/hierarchy/child/name true
if [ $? -ne 122 ]; then exit 1; fi

set -e

echo "Checking xml file with some missing leaves in the hierarchy."
cat $srcdir/data/incomplete.xml | xmltsort -St //block /*/name /*/hierarchy/child/name path.sh > results/incomplete.path
echo "Output is correct? ..."
diff -u results/incomplete.path $srcdir/data/golden/incomplete.path

echo "Checking the simplest xml file..."
cat $srcdir/data/tiny.xml | xmltsort -St //block /block/name /block/hierarchy/child/name path.sh > results/tiny.path
echo "Output is correct? ..."
diff -u results/tiny.path $srcdir/data/golden/tiny.path

rm -rf results/run
mkdir results/run

echo "Checking a little more complex hierarchy..."
cat $srcdir/data/small.xml | xmltsort -S //block /block/name /block/hierarchy/child/name hier.sh

echo "Checking results"
for letter in a b c d e f g h i j k; do
  test -f results/run/$letter
done

rm -rf results/run
mkdir results/run

echo "Checking a little more complex hierarchy with parallel processes..."
cat $srcdir/data/small.xml | xmltsort -SP 5 //block /block/name /block/hierarchy/child/name hier.sh > results/bad-crawl

echo "Checking results"
for letter in a b c d e f g h i j k; do
  test -f results/run/$letter
done

echo "Checking a bug where some elements may not be handled."
cat $srcdir/data/bad-crawl.xml |
  xmltsort -S '//block' '/*/name' '/*/*/child/name' -- sh -c 'echo $name' > results/bad-crawl
diff -u $srcdir/data/golden/bad-crawl results/bad-crawl
