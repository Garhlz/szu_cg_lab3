// InitShader.cpp
#include "Angel.h"
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>

// 移除有问题的 fopen_s 宏定义
// #define fopen_s(pFile, filename, mode) ((*(pFile)) = fopen((filename), (mode))) == NULL

namespace Angel
{
	// Create a NULL-terminated string by reading the provided file
	// 修正版本，使用标准 fopen 并添加错误检查
	static char *readShaderSource(const char *shaderFile)
	{
		FILE *fp = nullptr;

#ifdef _WIN32
		// 在 Windows 上，可以选择使用 fopen_s 以获得潜在的安全优势
		errno_t err = fopen_s(&fp, shaderFile, "r");
		if (err != 0 || fp == nullptr)
		{
			std::cerr << "错误: 无法打开着色器文件 (fopen_s): " << shaderFile << std::endl;
			// 根据原始代码行为，这里应该返回 NULL
			return NULL;
		}
#else
		// 在非 Windows 平台（Linux, macOS 等）使用标准 fopen
		fp = fopen(shaderFile, "r");
		if (fp == NULL)
		{
			// 使用 perror 输出更详细的系统错误信息
			perror(("错误: 无法打开着色器文件 (fopen): " + std::string(shaderFile)).c_str());
			return NULL;
		}
#endif

		// 获取文件大小
		fseek(fp, 0L, SEEK_END);
		long size = ftell(fp);
		if (size == -1L)
		{ // 添加错误检查
			perror("错误: 获取着色器文件大小失败");
			fclose(fp);
			return NULL;
		}
		fseek(fp, 0L, SEEK_SET); // 回到文件开头

		// 分配内存 (+1 用于 '\0')
		char *buf = new (std::nothrow) char[size + 1]; // 使用 nothrow 避免异常
		if (!buf)
		{
			std::cerr << "错误: 分配着色器源码内存失败" << std::endl;
			fclose(fp);
			return NULL;
		}

		// 清空缓冲区 (可选，但良好实践)
		memset(buf, 0, size + 1);

		// 读取文件内容
		size_t bytesRead = fread(buf, 1, size, fp);
		// 检查读取是否完整以及是否发生错误
		if (bytesRead < size && ferror(fp))
		{
			perror(("错误: 读取着色器文件失败: " + std::string(shaderFile)).c_str());
			delete[] buf; // 释放内存
			fclose(fp);
			return NULL;
		}

		// 确保字符串以 null 结尾 (即使 fread 提前结束)
		buf[bytesRead] = '\0';
		fclose(fp);

		return buf;
	}

	// Create a GLSL program object from vertex and fragment shader files
	// InitShader 函数保持与你提供的版本完全相同的结构和行为
	GLuint InitShader(const char *vShaderFile, const char *fShaderFile)
	{
		struct Shader
		{
			const char *filename;
			GLenum type;
			GLchar *source; // 保持使用 GLchar*
		} shaders[2] = {
			{vShaderFile, GL_VERTEX_SHADER, NULL},
			{fShaderFile, GL_FRAGMENT_SHADER, NULL}};

		GLuint program = glCreateProgram();

		for (int i = 0; i < 2; ++i)
		{
			Shader &s = shaders[i];
			// 调用修正后的 readShaderSource
			s.source = readShaderSource(s.filename);
			// 保持原始的 NULL 检查方式
			if (shaders[i].source == NULL)
			{
				std::cerr << "Failed to read " << s.filename << std::endl;
				exit(EXIT_FAILURE);
			}

			GLuint shader = glCreateShader(s.type);
			glShaderSource(shader, 1, (const GLchar **)&s.source, NULL);
			glCompileShader(shader);

			GLint compiled;
			glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
			if (!compiled)
			{
				std::cerr << s.filename << " failed to compile:" << std::endl;
				GLint logSize;
				glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logSize);
				char *logMsg = new char[logSize];
				glGetShaderInfoLog(shader, logSize, NULL, logMsg);
				std::cerr << logMsg << std::endl;
				delete[] logMsg;
				// 在编译失败时也需要释放 s.source
				delete[] s.source;
				exit(EXIT_FAILURE);
			}

			// 保持在编译和附加后释放源码内存
			delete[] s.source;

			glAttachShader(program, shader);
			// 保持原始代码行为，不在链接前删除 shader 对象
		}

		/* link and error check */
		glLinkProgram(program);

		GLint linked;
		glGetProgramiv(program, GL_LINK_STATUS, &linked);
		if (!linked)
		{
			std::cerr << "Shader program failed to link" << std::endl;
			GLint logSize;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logSize);
			char *logMsg = new char[logSize];
			glGetProgramInfoLog(program, logSize, NULL, logMsg);
			std::cerr << logMsg << std::endl;
			delete[] logMsg;

			exit(EXIT_FAILURE);
		}

		/* use program object */
		// 保持原始代码的行为，在函数末尾调用 glUseProgram
		glUseProgram(program);

		return program;
	}

} // Close namespace Angel block