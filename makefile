CC      = g++
CFLAGS  = -std=c++17 -Wall -Wextra -O2 -g

BIN     = iosched            # final executable name
SRCS    = iosched.cpp        # source files
OUTDIR  = output             # where runit.sh will save results
REFDIR  = refout             # reference outputs
DEBUGDIR = debug             # where debug outputs will be saved

.PHONY: all test grade logs runall clean debug

# default target -------------------------------------------------------
all: $(BIN)

# build executable -----------------------------------------------------
$(BIN): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^

# run full test suite --------------------------------------------------
# outputs placed under $(OUTDIR)/

test: $(BIN)
	@mkdir -p $(OUTDIR)
	bash runit.sh $(OUTDIR) ./$(BIN)

# compare against reference answers -----------------------------------

grade: test
	bash gradeit.sh $(REFDIR) $(OUTDIR)

# generate build + run + grade logs -----------------------------------

logs:
	(hostname; $(MAKE) clean; $(MAKE) grade 2>&1) > make.log
	bash gradeit.sh $(REFDIR) $(OUTDIR) > gradeit.log

# run selected scheduler / flags quickly ------------------------------
# usage: make debug ARGS="-sS -v -q" (example)
ARGS?=-v -q -f   # default: SSTF with all debug flags

debug: $(BIN)
	@mkdir -p $(DEBUGDIR)
	@echo "Running: ./$(BIN) $(ARGS) -> $(DEBUGDIR)/out_*"
	bash runit.sh $(DEBUGDIR) ./$(BIN) $(ARGS)

# convenience meta target ---------------------------------------------
runall: all test grade logs

# cleanup --------------------------------------------------------------
clean:
	rm -f $(BIN)
	rm -rf $(OUTDIR) $(DEBUGDIR) *.o *.d