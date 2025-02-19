OBJS = src/sbell.o src/conffile.o
CXX = g++
CXXFLAGS = -I src/include

programa: $(OBJS)
	$(CXX) $(OBJS) -o programa

src/sbell.o: src/sbell.cpp src/include/conffile.hpp
	$(CXX) $(CXXFLAGS) -c src/sbell.cpp -o src/sbell.o

src/conffile.o: src/conffile.cpp src/include/conffile.hpp
	$(CXX) $(CXXFLAGS) -c src/conffile.cpp -o src/conffile.o

clean:
	rm -f src/*.o programa

