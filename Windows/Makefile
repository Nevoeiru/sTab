CXX ?= ../dependencias/w64devkit/bin/g++
CXXFLAGS = -std=c++17 -O3 -Wall -Wextra -Iinclude
LDFLAGS = -lws2_32 -luser32 -ld3d11 -ldxgi -lmfplat -lmfuuid -lole32 -loleaut32 -static -s

SRCDIR = src
INCDIR = include
OBJDIR = obj
BINDIR = bin

SOURCES = $(SRCDIR)/Main.cpp $(SRCDIR)/PointerInjector.cpp $(SRCDIR)/GestureTranslator.cpp $(SRCDIR)/TcpServer.cpp $(SRCDIR)/DxgiCapture.cpp $(SRCDIR)/H264Encoder.cpp $(SRCDIR)/VideoServer.cpp
OBJECTS = $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SOURCES))
TARGET = $(BINDIR)/sTab_server.exe

all: directories $(TARGET)

directories:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	@if not exist $(BINDIR) mkdir $(BINDIR)

$(TARGET): $(OBJECTS)
	@echo [LINK] $@
	@$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo [SUCESSO] Executavel gerado em: $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@echo [CXX] $<
	@$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@if exist $(OBJDIR) rmdir /s /q $(OBJDIR)
	@if exist $(BINDIR) rmdir /s /q $(BINDIR)

.PHONY: all clean directories
