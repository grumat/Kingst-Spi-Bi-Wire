TARGET  := libSbw.dylib

LINK := -L "../../lib/Mac" -lAnalyzer

CC       := clang++
HFILE    := ../../inc/*.h
SRC      := ../src/*.cpp
INC      := -I ../../inc/
CXXFLAGS := -Wall -O2 -c
FPIC     := -fPIC
SHARE    := -dynamiclib -o
OBJ      := *.o

$(TARGET) : $(SRC)
	$(CC) $(CXXFLAGS) $(FPIC) $(SRC) $(INC)
	$(CC) $(SHARE) $(TARGET) $(OBJ) $(LINK)
