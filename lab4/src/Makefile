CXX = g++
CXXFLAGS = -Wall -std=c++11 -fPIC

BUILD_DIR = build

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
    SHARED_FLAG = -dynamiclib -install_name @rpath/$(@F)
    LIB_EXT = dylib
    RPATH_FLAG = -Wl,-rpath,@executable_path
else
    SHARED_FLAG = -shared -Wl,-soname,$(@F)
    LIB_EXT = so
    RPATH_FLAG = -Wl,-rpath,$(BUILD_DIR)
endif

LIB1_TARGET_NAME = libimpl1.$(LIB_EXT)
LIB2_TARGET_NAME = libimpl2.$(LIB_EXT)

LIB1 = $(BUILD_DIR)/$(LIB1_TARGET_NAME)
LIB2 = $(BUILD_DIR)/$(LIB2_TARGET_NAME)
PROG1 = $(BUILD_DIR)/program1
PROG2 = $(BUILD_DIR)/program2

all: $(PROG1) $(PROG2)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(LIB1): lib1/lib1.cpp contracts/contracts.h | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(SHARED_FLAG) -o $@ $<

$(LIB2): lib2/lib2.cpp contracts/contracts.h | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(SHARED_FLAG) -o $@ $<

$(PROG1): program1/main1.cpp $(LIB1) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $< -L$(BUILD_DIR) -limpl1 $(RPATH_FLAG)

$(PROG2): program2/main2.cpp $(LIB1) $(LIB2) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $< -L$(BUILD_DIR) -limpl1 -limpl2 $(RPATH_FLAG)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
