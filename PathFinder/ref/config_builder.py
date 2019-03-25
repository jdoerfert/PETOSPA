#************************************************************************
#
#        PathFinder: finding a series of labeled nodes within a
#                two-layer directed, cyclic graph.
#               Copyright (2013) Sandia Corporation
#
# Copyright (2013) Sandia Corporation. Under the terms of Contract 
# DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government 
# retains certain rights in this software.
#
# This file is part of PathFinder.
# 
# PathFinder is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as 
# published by the Free Software Foundation, either version 3 of the 
# License, or (at your option) any later version.
#
# PathFinder is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with PathFinder.  If not, see <http://www.gnu.org/licenses/>.
#
# Questions? Contact J. Brian Rigdon (jbrigdo@sandia.gov)

from __future__ import print_function
import os
import glob

def get_signatures_from_file(sig_list,sig_file):
    try:
        sigs = open(sig_file, "r")
        for sig in sigs:
            sig_list.append(sig[0:-1])  ; # -1 to strip the \n
    except:
        print ( "Could not open", sig_file, "for reading\n" )

def config_builder():
    print ( "Welcome to the PathFinder configuration builder." )
    print ( "Please input the name of the config file:" )
    config_file = raw_input ( "(empty string to exit) > " )

    if ( config_file == "" ):
        exit()

    print ( "\nPlease input the directory the data files are in" )
    data_dir = raw_input ( "(complete or relative path) > " )

    print ( "\nPlease input a data file name:" )
    file_name = raw_input ( "(file name or *) > " )
    data_files = []
    if ( file_name == "*" ):
        data_files = glob.glob( data_dir+"/*.adj_list" )
    else:
        while ( file_name != "" ):
            data_files.append(data_dir+"/"+file_name)
            file_name = raw_input ( "(file or \"\" to finish) > " )

    print ( "Do you want to load signatures from a file (or files) or input them manually" )
    sig = raw_input ( "(file/manual) > " )
    signatures = []
    if ( sig == "file" ):
        print ( "\nPlease input a signature file name:" )
        while ( True ):
            sig = raw_input ( "(include path to file; \"\" to finish) > " )
            if sig == "":
                break
            else:
                get_signatures_from_file(signatures, sig)
    else:
        print ( "\nPlease input a signature:" )
        while ( True ):
            sig = raw_input ( "(include path to file; \"\" to finish) > " )
            if sig == "":
                break
            else:
                signatures.append(sig)

    print ( "\nDo you want to do a tree-based search or a diagram-based search" )
    search = raw_input ( "(enter 'tree' or 'diagram') > " )
    if (search != "tree") and (search != "diagram"):
        search = tree

    print ( "\nCreating config file:", config_file );
    config = open( config_file, 'w' )
    config.write("Pathfinder Configuration\n")
    config.write("files " + str(len(data_files)) + "\n" )
    config.write("signatures " + str(len(signatures)) + "\n" )
    config.write("search type " + search + "\n");
    for file in data_files:
        config.write("file " + file + "\n")
    for sig in signatures:
        config.write("signature " + sig + "\n")

    config.close()
    print("\nComplete\n\n");


if __name__ == "__main__":
    config_builder()