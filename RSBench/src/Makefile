#===============================================================================
# User Options
#===============================================================================

COMPILER  = gnu
OPTIMIZE  = yes
DEBUG     = yes
PROFILE   = no
STATUS    = no
PAPI      = no
VERIFY    = yes

#===============================================================================
# Program name & source code list
#===============================================================================

program = rsbench

source = \
main.c \
simulation.c \
xs_kernel.c \
io.c \
init.c \
material.c \
utils.c 

obj = $(source:.c=.o)

#===============================================================================
# Sets Flags
#===============================================================================

# Standard Flags
CFLAGS := -std=gnu99 -Wall

# gcc Compiler
ifeq ($(COMPILER),gnu)
  CC = /gpfs/jlse-fs0/users/bhomerding/projects/optimistic-tuner/release/bin/clang 
#  CC = clang
#  CFLAGS += -fopenmp -ffast-math
  CFLAGS += -fopenmp  -ffast-math -I /home/bhomerding/spack/opt/spack/linux-rhel7-x86_64/gcc-4.8.5/llvm-7.0.0-derveqa4vvel7hhsijnauy3of437ymrr/include/c++/v1
endif

# intel compiler
ifeq ($(COMPILER),intel)
  CC = icc
  CFLAGS += -qopenmp -xhost -ansi-alias -no-prec-div
endif

# PGI compiler (thanks Chip!)
ifeq ($(COMPILER),pgi)
  CC = pgcc
  CFLAGS := -mp -fastsse
endif

# Linker Flags
LDFLAGS = -lm

# Debug Flags
ifeq ($(DEBUG),yes)
  CFLAGS += -g
  LDFLAGS += -g
endif

# Profiling Flags
ifeq ($(PROFILE),yes)
  CFLAGS += -pg
  LDFLAGS  += -pg
endif

# Optimization Flags
ifeq ($(OPTIMIZE),yes)
  CFLAGS += -Ofast -march=native
endif

# Calculation Status % Complete Indicator
ifeq ($(STATUS),yes)
  CFLAGS += -DSTATUS
endif

# PAPI Performance Counters
ifeq ($(PAPI),yes)
  CFLAGS += -DPAPI
  source += papi.c
  LDFLAGS += -lpapi
endif

# Verification of results mode
ifeq ($(VERIFY),yes)
  CFLAGS += -DVERIFICATION
endif

#===============================================================================
# Targets to Build
#===============================================================================

$(program): $(obj) rsbench.h Makefile
	$(CC) $(CFLAGS) $(obj) -o $@ $(LDFLAGS)

%.o: %.c rsbench.h Makefile
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf rsbench $(obj)

edit:
	vim -p $(source) papi.c rsbench.h

run:
	./rsbench
