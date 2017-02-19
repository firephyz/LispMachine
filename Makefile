############################################
# Program building and installing makefile #
#                                          #
# Requires a directory structure as:       #
# $(TOP_DIR)                               #
#     - Makefile                           # 
#     - $(SRC_DIR)                         # 
#     - $(INCLUDE_DIR)                     #
#     - $(OTHER_REQUIRED_STUFF)            #
#                                          #
# Required libraries are listed like so:   #
#     :lib1 :lib2 :lib3...etc.             #
#                                          #
############################################

#################### Basic Info###################
NAME = lisp
DIR = $(shell pwd)
CC = gcc
REQUIRED_LIBRARIES = 

################# Directories ################
SRC_DIR = $(DIR)/src/
INCLUDE_DIR = $(DIR)/include/
BUILD_DIR = $(DIR)/build/
BIN_DIR = $(DIR)/bin/
INSTALL_BIN_DIR = /usr/local/bin/

################# Flags #######################
CCFLAGS = -g -g3 -Wall -std=c99 -D_POSIX_C_SOURCE=200900L
LIB_FLAGS = $(subst :,-l$,$(REQUIRED_LIBRARIES))
INCLUDE_FLAGS = -I$(INCLUDE_DIR) $(subst :,-I/usr/local/include/,$(REQUIRED_LIBRARIES))
ALL_FLAGS =  -Wl,-rpath=/usr/local/lib $(CCFLAGS) $(LIB_FLAGS) $(INCLUDE_FLAGS)

################ Program files ####################
SRC_FILES := $(wildcard $(SRC_DIR)*.c)
BUILD_FILES := $(subst src,build,$(SRC_FILES:.c=.o))

################## Targets #######################
all: | clean init $(BUILD_FILES)
	$(CC) $(ALL_FLAGS) $(BUILD_FILES) -o $(BIN_DIR)/$(NAME)

%.o:
	$(CC) -c $(ALL_FLAGS) $(subst build,src,$*.c) -o $*.o

install: install_util
	@echo Done!

uninstall: uninstall_util
	@echo Done!

clean:
	rm -r -f $(BUILD_DIR)
	rm -r -f $(BIN_DIR)

############# Utilies ###################
install_util:
	@echo Installing libraries...
	cp $(wildcard $(BIN_DIR)* $(INSTALL_BIN_DIR))

uninstall_util:
	@echo Uninstalling libraries...
	rm $(INSTALL_BIN_DIR)$(subst $(BIN_DIR),,$(wildcard $(BIN_DIR)*))

init:
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BIN_DIR)