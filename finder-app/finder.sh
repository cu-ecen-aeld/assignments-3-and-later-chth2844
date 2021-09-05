#!/bin/bash
#finder script
#Author: Chirayu Thakur
FILESDIR=$1
SEARCHSTR=$2

#One or more arguments missing
if [ $# -lt 2 ]
then
	echo "Error: Invalid Number of Arguments"
	echo "Total Arguments should be 2"
	exit 1

#File directory found in system
elif [ -d  "$FILESDIR" ]
then
        
	NUMLINES=$(grep -r $SEARCHSTR $FILESDIR | wc -l)
	#NUMFILES=$(ls $FILESDIR | wc -l)
	NUMFILES=$(grep -lr $SEARCHSTR $FILESDIR | wc -l)
	echo "The number of files are $NUMFILES and the number of matching lines are $NUMLINES"

#file directory not found in system 
else
	echo "$FILESDIR does not represent a file in the system"
	exit 1
fi
