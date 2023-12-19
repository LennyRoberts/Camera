CXX=arm-linux-gnueabihf-g++
TARGET=camera

$(TARGET) : v4l2.o
	$(CXX) -o $(TARGET) v4l2.o

v4l2.o : v4l2.cpp
	$(CXX) -c -std=c++11 -lm -ljpeg -lrt v4l2.cpp

.PHONY: clean
clean:
	rm -rf $(TARGET) v4l2.o