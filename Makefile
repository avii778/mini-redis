# Compiler and flags
CXX := g++
CXXFLAGS := -Wall -Wextra -O2 -g

# Common source files (helpers used by both server and client)
SOURCES_COMMON := read_and_write.cpp
OBJECTS_COMMON := $(SOURCES_COMMON:.cpp=.o)

# Individual targets
SOURCES_SERVER := server.cpp
OBJECTS_SERVER := $(SOURCES_SERVER:.cpp=.o)

SOURCES_CLIENT := client.cpp
OBJECTS_CLIENT := $(SOURCES_CLIENT:.cpp=.o)

# Default target: build both
all: server client

# Build server
server: $(OBJECTS_SERVER) $(OBJECTS_COMMON)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Build client
client: $(OBJECTS_CLIENT) $(OBJECTS_COMMON)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Generic rule to build any .o from .cpp
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up
clean:
	rm -f *.o server client

.PHONY: all clean
