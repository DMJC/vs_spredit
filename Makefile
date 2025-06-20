CXX := g++
PKGS := gtkmm-3.0 cairomm-1.0
CXXFLAGS := -std=c++20 $(shell pkg-config $(PKGS) --cflags)
LDFLAGS := $(shell pkg-config $(PKGS) --libs)

TARGET := vs_spredit
SRCS := vegastrike_animation.cpp
OBJS := $(SRCS:.cpp=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
