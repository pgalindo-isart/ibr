MAKEFLAGS+=--no-builtin-rules --no-builtin-variables

CXXFLAGS=-O0 -g -Wall -Wextra -Wno-unused-parameter -MMD
CFLAGS:=$(CXXFLAGS)
CPPFLAGS=-Iinclude
LDLIBS=-lglfw -lgdi32
LDFLAGS=

# Common list
SRCS=src/main.cpp src/opengl_helpers.cpp src/camera.cpp src/mesh.cpp
# Demo files list
SRCS+=src/demo_base.cpp
# ImGui files list
SRCS+=src/imgui_demo.cpp src/imgui_draw.cpp src/imgui_widgets.cpp src/imgui.cpp src/imgui_impl_glfw.cpp src/imgui_impl_opengl3.cpp
SRCS+=src/tiny_obj_loader.cpp
SRCS+=src/stb_image.cpp

OBJS=$(SRCS:.cpp=.o) src/glad.o
DEPS=$(SRCS:.cpp=.d) src/glad.d

all: ibr

-include $(DEPS)

%.o: %.c
	$(CC) -c $< $(CFLAGS) $(CPPFLAGS) -o $@

%.o: %.cpp
	$(CXX) -c $< $(CFLAGS) $(CPPFLAGS) -o $@

ibr: $(OBJS)
	$(CXX) $^ $(LDFLAGS) $(LDLIBS) -o $@

.PHONY: clean
clean:
	rm -rf ibr $(OBJS) $(DEPS)
