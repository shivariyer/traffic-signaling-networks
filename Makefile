CPP := g++
CPPFLAGS := -g
OBJDIR := obj

all: $(OBJDIR) runner

$(OBJDIR): 
	@mkdir -p $(OBJDIR)

runner: $(OBJDIR)/runner.o $(OBJDIR)/traffic_sim.o
	$(CPP) $(CPPFLAGS) -o $@ $^

$(OBJDIR)/traffic_sim.o: traffic_sim.cpp traffic_sim.hh
	$(CPP) $(CPPFLAGS) -o $@ -c $<

$(OBJDIR)/runner.o: runner.cpp
	$(CPP) $(CPPFLAGS) -o $@ -c $<

.PHONY: clean
clean:
	rm -f $(OBJDIR)/*.o runner
	rmdir $(OBJDIR)
