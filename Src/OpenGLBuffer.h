#pragma once

#include "glad/glad.h"
#include "DeviceCommon.h"

namespace Catherine
{
	class OpenGLBuffer
	{
	public:
		bool Initialize(GLenum target, Usage usage, size_t size, const void * data = nullptr);
		void Uninitialize();

		void Bind();
		void Bind(uint8_t slot);

	private:
		GLenum m_Target;
		GLenum m_Usage;
		GLuint m_Resource;
	};
}