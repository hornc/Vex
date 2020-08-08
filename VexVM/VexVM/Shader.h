#pragma once

#include <glad/glad.h>

struct Shader {
	Shader();
	Shader(const char* vertex_path, const char* fragment_path);
	~Shader();

	GLuint ID();

	void Load();
	void SendLine(GLuint vao, float x0, float y0, float x1, float y1);
private:
	GLuint id;
	const char* vertex_path;
	const char* fragment_path;
};