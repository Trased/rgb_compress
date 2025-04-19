CXX := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Iinclude

TARGET := smp_codec

SRCDIR := src
INCDIR := include
OBJDIR := build

SRCS := $(wildcard $(SRCDIR)/*.cpp)
OBJS := $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCS))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) $(TARGET)

help:
	@echo "Targets:"
	@echo "  all   - Build the project"
	@echo "  clean - Remove binaries and object files"
	@echo "  help  - Show this help"
