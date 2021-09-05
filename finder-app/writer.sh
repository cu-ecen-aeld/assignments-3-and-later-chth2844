#!/bin/bash
#writer script
#Author: Chirayu Thakur
WRITEFILE=$1
WRITESTR=$2

#One or more arguments missing
if [ $# -lt 2 ]
then
        echo "Error: Invalid Number of Arguments"
        echo "Total Arguments should be 2"
        exit 1
fi
#File directory found in system
if [ -d  "$WRITEFILE" ]
then
        
        echo "$WRITESTR" > "$WRITEFILE"

#file directory not found in system create new file 
else
      mkdir -p  "$(dirname "$WRITEFILE")"
      touch "$WRITEFILE"
      #check if file created
      if [ -d "$WRITERFILE"]
      then 
	      echo "$WRITESTR">"$WRITEFILE"
      else
	      echo "File Creation Unsuccessfull"
	      exit 1
      fi
fi
