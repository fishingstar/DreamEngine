#pragma once

#include "IPipeline.h"
#include "CameraContext.h"

namespace Catherine
{
	class IRenderTarget;
	class ISampler;
	class ITexture;
	class LightContext;

	class ForwardPipeline : public IPipeline
	{
	public:
		virtual ~ForwardPipeline() override { }

		virtual bool Initialize() override;
		virtual void Uninitialize() override;

		virtual void Render(const WorldContext * context) override;

	private:
		void RenderShadow(const WorldContext * context);
		void RenderOpaque(const WorldContext * context);
		void RenderTransparent(const WorldContext * context);

		CameraContext GenerateShadowCameraContext(const LightContext * light, const CameraContext * camera);

	private:
		IRenderTarget * m_RenderTarget_Back = nullptr;
		IRenderTarget * m_RenderTarget_Shadow = nullptr;

		ISampler * m_ShadowSampler = nullptr;
		CameraContext m_ShadowCameraContext;

		ITexture * m_PrefilterMap = nullptr;
		ISampler * m_PrefilterSampler = nullptr;

		ITexture * m_BRDF_LUT = nullptr;
		ISampler * m_BRDF_LUTSampler = nullptr;
	};
}