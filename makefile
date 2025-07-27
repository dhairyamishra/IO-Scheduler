CC      = g++
CFLAGS  = -std=c++17 -Wall -Wextra -O2 -g

BIN     = iosched            # final executable name
SRCS    = iosched.cpp        # source files
OUTDIR  = output             # where runit.sh will save results
REFDIR  = refout             # reference outputs

.PHONY: all test grade logs runall clean

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

# convenience meta target ---------------------------------------------
runall: all test grade logs

# cleanup --------------------------------------------------------------
clean:
	rm -f $(BIN)
	rm -rf $(OUTDIR) *.o *.d