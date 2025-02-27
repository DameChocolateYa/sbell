EXEC = sbell
SRCS = $(wildcard src/*.cpp)

OBJS = $(SRCS:.cpp=.o)

CXX = g++
CXXFLAGS = -Wall -g -std=c++17

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CXX) $(OBJS) -o $(EXEC)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(EXEC)

fclean: clean
	rm -f $(EXEC)

re: fclean all
