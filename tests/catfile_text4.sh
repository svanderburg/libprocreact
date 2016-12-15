#!/bin/sh -e

./catfile text4.txt | cmp - output4.txt
