BASE = asst

all: $(BASE)

OS := $(shell uname -s)

ifeq ($(OS), Linux) # Science Center Linux Boxes
  CPPFLAGS = -I/home/l/i/lib175/usr/glew/include
  LDFLAGS += -L/home/l/i/lib175/usr/glew/lib -L/usr/X11R6/lib
  LIBS += -lGL -lGLU -lglut
endif

ifeq ($(OS), Darwin) # Assume OS X
  CPPFLAGS += -D__MAC__
  LDFLAGS += -framework GLUT -framework OpenGL
endif

ifdef OPT 
  #turn on optimization
  CXXFLAGS += -O2
else 
  #turn on debugging
  CXXFLAGS += -g
endif

CXX = g++ 

SRC_DIR := source
INC_DIR := include
OBJ_DIR := bin-int

SRC_FILES := $(wildcard $(SRC_DIR)/*.cpp)
OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC_FILES))

$(BASE): $(OBJ_FILES)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS) -lGLEW

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(OBJ_DIR) $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@ -I$(INC_DIR)

$(OBJ_DIR) $(BIN_DIR):
	mkdir $@

clean:
	rm -f $(BASE)
	rm -rf $(OBJ_DIR)
