#!/bin/sh -e

./catfile text2.txt | cmp - output2.txt
