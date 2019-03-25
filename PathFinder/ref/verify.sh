#!/bin/bash

echo `pwd`
make -s clean
make -s PathFinder.x
found=`./PathFinder.x -x ../generatedData/small1.adj_list  | grep searches | awk -F" " '{ print $1 }'`
if [[ ${found} == 3329 ]] ; then 
    echo ""
    echo "PathFinder execution verified"
    echo ""
else 
    echo ""
    echo "PathFinder check FAILED\!"; 
    echo ""
fi
