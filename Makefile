CFLAGS := $(shell pkg-config --cflags sdl3 cglm) -Iinclude
LDLIBS := $(shell pkg-config --libs sdl3 cglm) -lm

.PHONY: clean

kostka: main.o glad.o
	$(CC) $^ -o $@ $(LDLIBS)

shaders: fragment.spv vertex.spv
	
main.o: shaders

fragment.spv: fragment.glsl
	glslc -fshader-stage=frag -mfmt=c --target-env=opengl -o $@ $^

vertex.spv: vertex.glsl
	glslc -fshader-stage=vert -mfmt=c --target-env=opengl -o $@ $^

clean:
	$(RM) kostka *.o *.spv
