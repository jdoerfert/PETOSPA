#@header
# ************************************************************************
#
#      PathFinder: graph pathway analysis miniapp
#              Copyright (2013) Sandia corporation
#
# Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
# license for use of this work by or on behalf of the U.S. Government.
#
# This library is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation; either version 2.1 of the
# License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
# USA
# Questions? Contact Richard F. Barrett (rfbarre@sandia.gov) or
#                    Michael A. Heroux (maherou@sandia.gov)
#
# ************************************************************************
#@header

# Simple hand-tuned makefile, configured for the gnu compiler.
# Modify as necessary for your environment.

#-----------------------------------------------------------------------

#CC=cc
CC= /gpfs/jlse-fs0/users/bhomerding/projects/optimistic-tuner/release/bin/clang
#CC= clang

CFLAGS = $(OPT)
#CFLAGS += -openmp
CFLAGS += -fopenmp
#CFLAGS += -D_USE_PAT_API

LD      = $(CC)
LDFLAGS = $(CFLAGS)

# Optimization
OPT = -g -O3 -march=native 
#OPT = -g

LIBS = -lgomp -lm

include make_targets

# End makefile
