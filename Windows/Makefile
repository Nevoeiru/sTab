CXX ?= ../dependencias/w64devkit/bin/g++
CXXFLAGS = -std=c++17 -O3 -Wall -Wextra -Iinclude
LDFLAGS = -lws2_32 -luser32 -ld3d11 -ldxgi -lmfplat -lmfuuid -lole32 -loleaut32 -static -s

SRCDIR = src
INCDIR = include
OBJDIR = obj
BINDIR = bin

SOURCES = $(SRCDIR)/Main.cpp $(SRCDIR)/PointerInjector.cpp $(SRCDIR)/GestureTranslator.cpp $(SRCDIR)/TcpServer.cpp $(SRCDIR)/DxgiCapture.cpp $(SRCDIR)/H264Encoder.cpp $(SRCDIR)/VideoServer.cpp $(SRCDIR)/TelemetryBridge.cpp
OBJECTS = $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SOURCES))
TARGET = $(BINDIR)/sTab_server.exe
LAUNCHER_TARGET = $(BINDIR)/sTab_launcher.exe

all: directories $(TARGET) $(LAUNCHER_TARGET)

directories:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	@if not exist $(BINDIR) mkdir $(BINDIR)

$(TARGET): $(OBJECTS)
	@echo [LINK] $@
	@$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo [SUCESSO] Executavel gerado em: $@

$(LAUNCHER_TARGET): $(SRCDIR)/sTab_launcher.cpp
	@echo [LINK] $@
	@$(CXX) $(CXXFLAGS) $< -o $@ -static -s
	@echo [SUCESSO] Launcher gerado em: $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@echo [CXX] $<
	@$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@if exist $(OBJDIR) rmdir /s /q $(OBJDIR)
	@if exist $(BINDIR) rmdir /s /q $(BINDIR)

.PHONY: all clean directories
