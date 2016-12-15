#!/bin/sh -e

./catfile text3.txt | cmp - output3.txt
