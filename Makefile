# tiny-distributer -- a C++ tcp distributer application for Linux/Windows/iOS
# Copyright (C) 2014  Push Chen

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

# You can connect me by email: littlepush@gmail.com, 
# or @me on twitter: @littlepush

TD_ROOT = ./
OUT_DIR = $(TD_ROOT)/result

TD_DEFINES = -DVERSION=\"$(shell ./version)\" \
			 -DTARGET=\"$(shell gcc -v 2> /dev/stdout | grep Target | cut -d ' ' -f 2)\" \
			 -I./inc \
			 -std=c++11 \
			 -DUSE_THREAD_SERVICE \
			 #-DAUTO_TCPRELAY_SOCKS5 \
			 #-DTCPRELAY_DIRECT_TIMEOUT=50
THIRDPARTY = -I./jsoncpp -I./socklite

ifeq "$(MAKECMDGOALS)" "release"
	DEFINES = $(TD_DEFINES) $(THIRDPARTY) -DTINYDST_RELEASE -DRELEASE
	CPPFLAGS = 
	CFLAGS = -O2 -Wall $(DEFINES) -fPIC -pthread
	CXXFLAGS = -O2 -Wall $(DEFINES) -fPIC -pthread
	CMDGOAL_DEF := $(MAKECMDGOALS)
else
	ifeq "$(MAKECMDGOALS)" "withpg"
		DEFINES = $(TD_DEFINES) $(THIRDPARTY) -DTINYDST_WITHPG -DWITHPG -DDEBUG
		CPPFLAGS = 
		CFLAGS = -g -pg -Wall $(DEFINES) -fPIC -pthread
		CXXFLAGS = -g -pg -Wall $(DEFINES) -fPIC -pthread
		CMDGOAL_DEF := $(MAKECMDGOALS)
	else
		DEFINES = $(TD_DEFINES) $(THIRDPARTY) -DTINYDST_DEBUG -DDEBUG
		CPPFLAGS =
		CFLAGS = -g -Wall $(DEFINES) -fPIC -pthread
		CXXFLAGS = -g -Wall $(DEFINES) -fPIC -pthread
		CMDGOAL_DEF := $(MAKECMDGOALS)
	endif
endif

CC	 = g++
CPP	 = g++
CXX	 = g++
AR	 = ar

CPP_FILES = $(wildcard ./src/*.cpp) jsoncpp/jsoncpp.cpp socklite/socketlite.cpp
OBJ_FILES = $(CPP_FILES:.cpp=.o)

STATIC_LIBS = 
DYNAMIC_LIBS = 
EXECUTABLE = tinydst
TEST_CASE = 
RELAY_OBJECT = 

all	: PreProcess $(STATIC_LIBS) $(DYNAMIC_LIBS) $(EXECUTABLE) $(TEST_CASE) AfterMake

PreProcess :
	@if test -d $(OUT_DIR); then rm -rf $(OUT_DIR); fi
	@mkdir -p $(OUT_DIR)/static
	@mkdir -p $(OUT_DIR)/dynamic
	@mkdir -p $(OUT_DIR)/bin
	@mkdir -p $(OUT_DIR)/test
	@mkdir -p $(OUT_DIR)/data
	@mkdir -p $(OUT_DIR)/conf
	@mkdir -p $(OUT_DIR)/log
	@echo $(CPP_FILES)

cmdgoalError :
	@echo '***************************************************'
	@echo '******You must specified a make command goal.******'
	@echo '***************************************************'

debugclean :
	@rm -rf $(TEST_ROOT)/debug

relclean :
	@rm -rf $(TEST_ROOT)/release

pgclean : 
	@rm -rf $(TEST_ROOT)/withpg

clean :
	rm -vf src/*.o; rm -vf jsoncpp/*.o; rm -vf socklite/*.o; rm -rf $(OUT_DIR)

install:
	@if [ -f ./result/bin/tinydst ]; then cp -v ./result/bin/tinydst /usr/bin/; else echo "please make first"; fi

AfterMake : 
	@if [ "$(MAKECMDGOALS)" == "release" ]; then rm -vf src/*.o; rm -vf jsoncpp/*.o; rm -vf socklite/*.o; fi
	@mv -vf $(TD_ROOT)/tinydst $(OUT_DIR)/bin/tinydst

debug : PreProcess $(STATIC_LIBS) $(DYNAMIC_LIBS) $(EXECUTABLE) $(TEST_CASE) AfterMake
	@exit 0

release : PreProcess $(STATIC_LIBS) $(DYNAMIC_LIBS) $(EXECUTABLE) $(TEST_CASE) AfterMake
	@exit 0

withpg : PreProcess $(STATIC_LIBS) $(DYNAMIC_LIBS) $(EXECUTABLE) $(TEST_CASE) AfterMake
	@exit 0

%.o: src/%.cpp
	$(CC) $(CXXFLAGS) -c -o $@ $<

tinydst: $(OBJ_FILES)
	$(CC) -o $@ $^ $(CXXFLAGS)

