# FHE 24 Apr 2025 from audio-loop

CXXFLAGS=-Wall -g

# FHE 13 Dec 2023
# automatic dependency generation
# https://stackoverflow.com/questions/97338/gcc-dependency-generation-for-a-different-output-directory
# (Steve Pitchers). with -include line at bottom
%.o: %.cpp
	gcc $(CXXFLAGS) -MMD -c $< -o $@

all: list-word2vec-bin

list-word2vec-bin: list-word2vec-bin.o
	$(CXX) $(CXXFLAGS) -o $@ $< -lasound

-include *.d
