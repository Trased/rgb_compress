CXX := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Iinclude

ifdef DEBUG
#CXXFLAGS += -DDEBUG -DDEBUG_COMPRESS -DDEBUG_PROCESS -DDEBUG_BLOCKS -DDEBUG_QUANTIZED_BLOCKS -DDEBUG_LARGE_BLOCK
CXXFLAGS += -DDEBUG -DDEBUG_HUFFMAN
endif

ifdef DEBUG_PROCESS
CXXFLAGS += -DDEBUG_PROCESS
endif

ifdef DEBUG_COMPRESS
CXXFLAGS += -DDEBUG_COMPRESS
endif

ifdef DEBUG_BLOCKS
CXXFLAGS += -DDEBUG_BLOCKS
endif

ifdef DEBUG_QUANTIZED_BLOCKS
CXXFLAGS += -DDEBUG_QUANTIZED_BLOCKS
endif

ifdef DEBUG_LARGE_BLOCK
CXXFLAGS += -DDEBUG_LARGE_BLOCK
endif

ifdef DEBUG_HUFFMAN
CXXFLAGS += -DDEBUG_HUFFMAN
endif

TARGET := smp_codec

SRCDIR := src
INCDIR := include
OBJDIR := build
DEBUGDIR := debug
FILESDIR := files

SRCS := $(wildcard $(SRCDIR)/*.cpp)
OBJS := $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCS))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^
	@touch $(DEBUGDIR)/yuv_frames_output.txt
	@touch $(DEBUGDIR)/blocks_output.txt
	@touch $(DEBUGDIR)/quantized_blocks_output.txt
	@touch $(DEBUGDIR)/large_block_output.txt
	@touch $(DEBUGDIR)/header_output.txt
	@touch $(DEBUGDIR)/compressed_data_output.txt
	@touch $(DEBUGDIR)/compress.txt
	@touch $(FILESDIR)/compress.rgb

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)
	mkdir -p $(DEBUGDIR)

clean:
	rm -rf $(OBJDIR) $(TARGET)
	rm -rf $(DEBUGDIR)
	rm -rf $(FILESDIR)/compress.rgb

help:
	@echo "Targets:"
	@echo "  all   - Build the project"
	@echo "  clean - Remove binaries and object files"
	@echo "  help  - Show this help"
