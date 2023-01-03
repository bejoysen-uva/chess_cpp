CXX = clang++
CXXFLAGS = -std=c++11 -Wall -g
TARGET = chess

all: $(TARGET)
$(TARGET): $(TARGET).cpp chess_state.o
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(TARGET).cpp chess_state.o
chess_state.o: chess_state.h

clean:
	$(RM) *.o chess