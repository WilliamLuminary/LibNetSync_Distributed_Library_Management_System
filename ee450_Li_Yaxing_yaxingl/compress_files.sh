#!/bin/bash
# Shell script to copy files into a directory and then compress that directory

# Step 1: Create the directory with the desired name
mkdir ee450_Li_Yaxing_yaxingl

# Step 2: Copy the files into the directory
cp *.cpp *.h README.md Makefile ee450_Li_Yaxing_yaxingl/

# Step 3: Compress the directory
tar -czvf ee450_Li_Yaxing_yaxingl.tar.gz ee450_Li_Yaxing_yaxingl/

# End of script

