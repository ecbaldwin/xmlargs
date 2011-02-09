#!/bin/bash

echo; echo

PATH=$srcdir/data:.:$PATH

mkdir -p results

echo "Checking no args..."
xmlargs 2>/dev/null
if [ $? -ne 1 ]; then exit 1; fi

echo "Checking bogus arg..."
xmlargs -bogus //name path.sh 2>/dev/null
if [ $? -ne 1 ]; then exit 1; fi

echo "Checking non-existant script..."
cat $srcdir/data/tiny.xml | xmlargs //name pathhh.sh 2>/dev/null
if [ $? -ne 127 ]; then exit 1; fi

echo "Checking non-executable script"
cat $srcdir/data/tiny.xml | xmlargs //name tiny.xml 2>/dev/null
if [ $? -ne 126 ]; then exit 1; fi

echo "Checking failed script"
cat $srcdir/data/tiny.xml | xmlargs //name false 2>/dev/null
if [ $? -ne 123 ]; then exit 1; fi

set -e

echo "Checking missing command arg..."
echo "" | xmlargs -S //name 2>/dev/null

echo "Checking the simplest xml file..."
cat $srcdir/data/tiny.xml  | xmlargs -St      //name echo > results/xmlargs1
diff -u results/xmlargs1 $srcdir/data/golden/xmlargs1

cat $srcdir/data/tiny.xml  | xmlargs -St -n 1 //name echo > results/xmlargs2
diff -u results/xmlargs2 $srcdir/data/golden/xmlargs2

echo "Checking -r"
test "ran" = "$(cat $srcdir/data/tiny.xml | xmlargs -St  //aoeu echo ran)"
test ""    = "$(cat $srcdir/data/tiny.xml | xmlargs -Str //aoeu echo ran)"

echo "Checking a larger xml file..."
cat $srcdir/data/small.xml | xmlargs -St      '/*/*/name' echo > results/xmlargs3
diff -u results/xmlargs3 $srcdir/data/golden/xmlargs3

cat $srcdir/data/small.xml | xmlargs -St -n 3 '/*/*/name' echo > results/xmlargs4
diff -u results/xmlargs4 $srcdir/data/golden/xmlargs4

echo "Checking -W"
test 'schematic-data' != "$(cat $srcdir/data/small.xml | xmlargs -S '/blocks/block[ name = "c" ]/*/files/*/file[ contains( ., "schematic-" ) ]')"
test 'schematic-data'  = "$(xmlargs -f "$srcdir/data/small.xml"  -W '/blocks/block[ name = "c" ]/*/files/*/file[ contains( ., "schematic-" ) ]')"

echo "Checking that all matching nodes get found"
test "7" = $(xmlargs -f $srcdir/data/xmlargs-missed-one -W -n 1 '//block/log/commit/message' | wc -l)
test "7" = $(xmlargs -f $srcdir/data/xmlargs-missed-one -S -n 1 '//block/log/commit/message' | wc -l)
