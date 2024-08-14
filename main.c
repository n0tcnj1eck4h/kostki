#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>
#include <cglm/cglm.h>
#include <cglm/vec3.h>
#include <glad/glad.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

uint32_t fragment_spv[] =
#include "fragment.spv"
	;

uint32_t vertex_spv[] =
#include "vertex.spv"
	;

void GLAPIENTRY gl_message_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
	fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n", (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type, severity, message);
}

typedef struct {
	vec3 camera_position;
	float _pad1;
	vec3 camera_direction;
	float _pad2;
	vec2 screen_size;
} UniformData;

#define UP ((vec3){0, 1, 0})

int main(int argc, char* argv[]) {
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
	SDL_Window* window = SDL_CreateWindow("Kostka", 800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	SDL_GLContext context = SDL_GL_CreateContext(window);

	gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);
	SDL_GL_MakeCurrent(window, context);

	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(gl_message_callback, NULL);

	GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderBinary(1, &vertex_shader, GL_SHADER_BINARY_FORMAT_SPIR_V, (void*)vertex_spv, sizeof(vertex_spv));
	glSpecializeShader(vertex_shader, "main", 0, NULL, NULL);

	glShaderBinary(1, &fragment_shader, GL_SHADER_BINARY_FORMAT_SPIR_V, (void*)fragment_spv, sizeof(fragment_spv));

	glSpecializeShader(fragment_shader, "main", 0, NULL, NULL);

	GLuint program = glCreateProgram();
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);

	glLinkProgram(program);

	glDetachShader(program, vertex_shader);
	glDetachShader(program, fragment_shader);

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	glUseProgram(program);
	glViewport(0, 0, 800, 600);

	GLuint useless_vao;
	glGenVertexArrays(1, &useless_vao);
	glBindVertexArray(useless_vao);

	GLuint ubo;
	glGenBuffers(1, &ubo);
	glBindBuffer(GL_UNIFORM_BUFFER, ubo);

	UniformData uniform = {.camera_position = {3, 3, 3}, .camera_direction = {0, 0, 0}, .screen_size = {800.0, 600.0}};
	glm_vec3_negate_to(uniform.camera_position, uniform.camera_direction);
	glm_vec3_normalize(uniform.camera_direction);

	GLuint ubo_index = glGetUniformBlockIndex(program, "UniformBlock");
	glBufferData(GL_UNIFORM_BUFFER, sizeof(UniformData), &uniform, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo);
	glUniformBlockBinding(program, ubo_index, 0);

	uint32_t storage_data[16 * 16 * 16] = {0};
	storage_data[1] = 0x00FF0000;
	storage_data[2] = 0x0000FF00;
	storage_data[3] = 0x000000FF;

	GLuint ssbo;
	glGenBuffers(1, &ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(storage_data), storage_data, GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);

	glClearColor(1.0, 0.0, 0.0, 1.0);

	uint64_t time = SDL_GetTicks();
	uint64_t last_time;

	const uint8_t* keyboard_state = SDL_GetKeyboardState(NULL);

	vec3 camera_right;
	glm_vec3_crossn(uniform.camera_direction, UP, camera_right);

	bool capturing_input = false;
	float sensitivity = 0.005;
	float speed = 20;

	SDL_GL_SetSwapInterval(1);

	SDL_Event event;
	while (1) {
		last_time = time;
		time = SDL_GetTicks();
		float delta = time - last_time;
		delta /= 1000.0;

		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_EVENT_KEY_DOWN:
				switch (event.key.scancode) {
				case SDL_SCANCODE_ESCAPE:
					capturing_input = false;
					SDL_SetRelativeMouseMode(false);
					break;
				default:
					break;
				}
				break;
			case SDL_EVENT_QUIT:
				goto exit;
			case SDL_EVENT_MOUSE_BUTTON_DOWN:
				if (capturing_input && event.button.button) {
					int64_t index = (int64_t)uniform.camera_position[0] + (int64_t)uniform.camera_position[1] * 16 + (int64_t)uniform.camera_position[2] * 16 * 16;
					index %= 16 * 16 * 16;
					storage_data[index] = 0x00FF00FF;
					glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(storage_data), storage_data, GL_STATIC_DRAW);
				}
				capturing_input = true;
				SDL_SetRelativeMouseMode(SDL_TRUE);
				break;
			case SDL_EVENT_MOUSE_MOTION:
				if (!capturing_input)
					break;
				glm_vec3_crossn(uniform.camera_direction, UP, camera_right);
				glm_vec3_rotate(uniform.camera_direction, -event.motion.yrel * sensitivity, camera_right);
				glm_vec3_rotate(uniform.camera_direction, -event.motion.xrel * sensitivity, UP);
				break;

			case SDL_EVENT_WINDOW_RESIZED:
				glViewport(0, 0, event.window.data1, event.window.data2);
				uniform.screen_size[0] = event.window.data1;
				uniform.screen_size[1] = event.window.data2;
			}
		}

		printf("%f, %f, %f, %f, %f, %f\n", uniform.camera_position[0], uniform.camera_position[1], uniform.camera_position[2], uniform.camera_direction[0], uniform.camera_direction[1], uniform.camera_direction[2]);
		printf("%f\n", 1 / delta);

		if (keyboard_state[SDL_SCANCODE_W])
			glm_vec3_muladds(uniform.camera_direction, delta * speed, uniform.camera_position);

		if (keyboard_state[SDL_SCANCODE_S])
			glm_vec3_muladds(uniform.camera_direction, -delta * speed, uniform.camera_position);

		if (keyboard_state[SDL_SCANCODE_D])
			glm_vec3_muladds(camera_right, delta * speed, uniform.camera_position);

		if (keyboard_state[SDL_SCANCODE_A])
			glm_vec3_muladds(camera_right, -delta * speed, uniform.camera_position);

		if (keyboard_state[SDL_SCANCODE_SPACE])
			glm_vec3_muladds(UP, delta * speed, uniform.camera_position);

		if (keyboard_state[SDL_SCANCODE_LCTRL])
			glm_vec3_muladds(UP, -delta * speed, uniform.camera_position);

		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(UniformData), &uniform);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		SDL_GL_SwapWindow(window);
	}

exit:
	glDeleteBuffers(1, &ubo);
	glDeleteVertexArrays(1, &useless_vao);
	glDeleteProgram(program);
	SDL_GL_DestroyContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
