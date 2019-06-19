#pragma once

#include <IProgram.h>
#include <glad/glad.h>

namespace Catherine
{
	class GLProgram : public IProgram
	{
	public:
		GLProgram();
		~GLProgram();

		virtual void AttachShader(const char * vertex, const char * fragment) override;
		virtual bool Compile() override;
		virtual bool Link() override;
		virtual void Use() override;

		virtual void SetBool(const char * key, bool value) override;
		virtual void SetInt(const char * key, int value) override;
		virtual void SetFloat(const char * key, float value) override;

	private:
		unsigned int CreateShader(GLenum param_Type, const char * param_FileName, const char * param_Default);

		bool CheckCompileStatus(unsigned int param_Shader);
		bool CheckLinkStatus(unsigned int param_Program);

	private:
		unsigned int m_Program = 0;

		unsigned int m_VertexShader = 0;
		unsigned int m_FragmentShader = 0;
	};
}
