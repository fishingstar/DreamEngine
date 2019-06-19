#include <OpenGLProgram.h>
#include <LogUtility.h>

namespace Catherine
{
	const char * vertexShaderSource = "#version 330 core\n"
		"layout (location = 0) in vec3 aPos;\n"
		"void main()\n"
		"{\n"
		"   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
		"}\0";
	const char * fragmentShaderSource = "#version 330 core\n"
		"out vec4 FragColor;\n"
		"void main()\n"
		"{\n"
		"   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
		"}\n\0";

	GLProgram::GLProgram()
	{
		m_Program = glCreateProgram();
	}

	GLProgram::~GLProgram()
	{
		glDeleteProgram(m_Program);
	}

	void GLProgram::AttachShader(const char * vertex, const char * fragment)
	{
		CreateShader(GL_VERTEX_SHADER, vertex, vertexShaderSource);
		CreateShader(GL_FRAGMENT_SHADER, fragment, fragmentShaderSource);
	}

	bool GLProgram::Compile()
	{
		glCompileShader(m_VertexShader);
		bool tmp_vertexOK = CheckCompileStatus(m_VertexShader);
		if (!tmp_vertexOK)
		{
			LogError("Vertex Shader Compile Failed...");
			return false;
		}

		glCompileShader(m_FragmentShader);
		bool tmp_fragmentOK = CheckCompileStatus(m_FragmentShader);
		if (!tmp_fragmentOK)
		{
			LogError("Fragment Shader Compile Failed...");
			return false;
		}

		return true;
	}

	bool GLProgram::Link()
	{
		glAttachShader(m_Program, m_VertexShader);
		glAttachShader(m_Program, m_FragmentShader);
		glLinkProgram(m_Program);
		bool tmp_programOK = CheckLinkStatus(m_Program);
		if (!tmp_programOK)
		{
			LogError("Program Link Failed...");
			return false;
		}

		return true;
	}

	void GLProgram::Use()
	{
		glUseProgram(m_Program);
	}

	void GLProgram::CreateShader(GLenum param_Type, const char * param_FileName, const char * param_Default)
	{
		const char * tmp_source = nullptr;

		if (param_FileName != nullptr)
		{
			
		}

		if (tmp_source == nullptr)
		{
			tmp_source = param_Default;
		}

		m_VertexShader = glCreateShader(param_Type);
		glShaderSource(m_VertexShader, 1, &tmp_source, nullptr);
	}

	bool GLProgram::CheckCompileStatus(GLuint param_Shader)
	{
		GLint tmp_result = 0;
		char tmp_log[1024] = { 0 };

		glGetShaderiv(param_Shader, GL_COMPILE_STATUS, &tmp_result);
		if (!tmp_result)
		{
			glGetShaderInfoLog(param_Shader, sizeof(tmp_log), nullptr, tmp_log);
			LogError(tmp_log);
		}

		return tmp_result;
	}

	bool GLProgram::CheckLinkStatus(GLuint param_Program)
	{
		GLint tmp_result = 0;
		char tmp_log[1024] = { 0 };

		glGetProgramiv(param_Program, GL_LINK_STATUS, &tmp_result);
		if (!tmp_result)
		{
			glGetProgramInfoLog(param_Program, sizeof(tmp_log), nullptr, tmp_log);
			LogError(tmp_log);
		}

		return tmp_result;
	}
}
