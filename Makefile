GEM := mruby-curl

include $(MAKEFILE_4_GEM)

ifeq ($(OS),Windows_NT)
MRUBY_LIBS = -lcurldll
else
MRUBY_LIBS = -lcurl
endif

GEM_C_FILES := $(wildcard $(SRC_DIR)/*.c)
GEM_OBJECTS := $(patsubst %.c, %.o, $(GEM_C_FILES))

gem-all : $(GEM_OBJECTS) gem-c-files

gem-clean : gem-clean-c-files
