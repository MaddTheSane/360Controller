#!/bin/bash
for i in Install360Controller; do
    rm -rf $i/build
done
rm -rf build
rm -rf "10.6Build"
echo "Deleted build products."
