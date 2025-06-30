CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2
LIBS = -lasound -pthread

TARGET = main
SRC = main.cpp

.PHONY: all clean run display

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

run: $(TARGET)
	./$(TARGET)

display:
	python3 display.py

translate:
	python3 translate.py transcript.txt translated.txt

clean:
	rm -f $(TARGET) *.o *.wav transcript.txt translated.txt
