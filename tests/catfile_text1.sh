#!/bin/sh -e

./catfile text1.txt | cmp - output1.txt
