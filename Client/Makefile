CXX=arm-linux-gnueabihf-g++
#CXX=g++
TARGET=camera

SRCS = ./src/main.cpp \
			 ./lib/cJsonObject/cJSON.cpp \
			 ./lib/cJsonObject/CJsonObject.cpp
	
INC = -I ./inc ./lib/cJsonObject
SO = -L ./inc ./lib/cJsonObject

OBJS = $(SRCS:.cpp=.o)

$(TARGET):$(OBJS)
#$(CXX) -o $@ $^ -g -std=c++11 -lpthread -lm -lrt
	$(CXX) -o $@ $^ -std=c++11 -lpthread -lm -ljpeg -lrt

.PHONY: clean
clean:
	rm -rf $(TARGET) $(OBJS)

%.o:%.cpp
#$(CXX) $(CFLAGS) $(INC) -o $@ -c $< -g -std=c++11 -lpthread -lm -lrt
	$(CXX) $(CFLAGS) $(INC) -o $@ -c $< -std=c++11 -lpthread -lm -ljpeg -lrt