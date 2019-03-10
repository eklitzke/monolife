CXXFLAGS := -g -Wall -O2
JOBS := monolife percolate

.PHONY: all
all: $(JOBS)

monolife: monolife.cc
	$(CXX) $(CXXFLAGS) -lmonome $< -o $@

percolate: percolate.cc
	$(CXX) $(CXXFLAGS) -lmonome $< -o $@

.PHONY: clean
clean:
	rm -rf $(JOBS)
