BUILDDIR := ./
PRODUCT := pennant

SRCDIR := ./

HDRS := $(wildcard $(SRCDIR)/*.hh)
SRCS := $(wildcard $(SRCDIR)/*.cc)
OBJS := $(SRCS:$(SRCDIR)/%.cc=$(BUILDDIR)/%.o)
DEPS := $(SRCS:$(SRCDIR)/%.cc=$(BUILDDIR)/%.d)

BINARY := $(BUILDDIR)/$(PRODUCT)

# begin compiler-dependent flags
#
# gcc flags:
#CXX := g++
#CXXFLAGS_DEBUG := -g
#CXXFLAGS_OPT := -O3
#CXXFLAGS_OPENMP := -fopenmp

# intel flags:
CXX := /home/bhomerding/gpfs/projects/optimistic-tuner/release/bin/clang++
CXXFLAGS_DEBUG := -g
CXXFLAGS_OPT := -g -O3 -flto=thin -Wl,-plugin-opt,jobs=1 -ffast-math -march=native -I /home/bhomerding/include -L /home/bhomerding/lib
CXXFLAGS_OPENMP := -fopenmp

# pgi flags:
#CXX := pgCC
#CXXFLAGS_DEBUG := -g
#CXXFLAGS_OPT := -O3 -fastsse
#CXXFLAGS_OPENMP := -mp

# end compiler-dependent flags

# select optimized or debug
CXXFLAGS := $(CXXFLAGS_OPT)
#CXXFLAGS := $(CXXFLAGS_DEBUG)

# add mpi to compile (comment out for serial build)
# the following assumes the existence of an mpi compiler
# wrapper called mpicxx
#CXX := mpicxx
#CXXFLAGS += -DUSE_MPI

# add openmp flags (comment out for serial build)
CXXFLAGS += $(CXXFLAGS_OPENMP)
LDFLAGS += $(CXXFLAGS) -L /home/bhomerding/lib

LD := $(CXX)


# begin rules
all : $(BINARY)

-include $(DEPS)

$(BINARY) : $(OBJS)
	@echo linking $@
	$(maketargetdir)
	$(LD) -o $@ $^ $(LDFLAGS)

$(BUILDDIR)/%.o : $(SRCDIR)/%.cc
	@echo compiling $<
	$(maketargetdir)
	$(CXX) $(CXXFLAGS) $(CXXINCLUDES) -c -o $@ $<

$(BUILDDIR)/%.d : $(SRCDIR)/%.cc
	@echo making depends for $<
	$(maketargetdir)
	@$(CXX) $(CXXFLAGS) $(CXXINCLUDES) -MM $< | sed "1s![^ \t]\+\.o!$(@:.d=.o) $@!" >$@

define maketargetdir
	-@mkdir -p $(dir $@) >/dev/null 2>&1
endef

.PHONY : clean
clean :
	rm -f $(BINARY) $(OBJS) $(DEPS)
