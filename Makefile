.PHONY: all
all: monolife percolate

monolife: monolife.cc
	$(CXX) -g -lmonome $< -o $@

percolate: percolate.cc
	$(CXX) -g -lmonome $< -o $@
