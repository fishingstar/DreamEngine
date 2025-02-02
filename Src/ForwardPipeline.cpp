#include "ForwardPipeline.h"
#include "WorldContext.h"
#include "CameraContext.h"
#include "LightContext.h"
#include "RenderContext.h"
#include "IMaterial.h"
#include "IDevice.h"
#include "IVertexArray.h"
#include "IRenderTarget.h"
#include "RenderTargetManager.h"
#include "ISampler.h"
#include "TextureManager.h"
#include <algorithm>

namespace Catherine
{
	extern IDevice * g_Device;

	const std::vector<std::string> s_PrefilterPath =
	{
		"./res/skybox/day/right.jpg",
		"./res/skybox/day/left.jpg",
		"./res/skybox/day/top.jpg",
		"./res/skybox/day/bottom.jpg",
		"./res/skybox/day/back.jpg",
		"./res/skybox/day/front.jpg"
	};

	const std::string s_BRDF_LUT_Path = "./res/texture/ibl_brdf_lut.png";

	bool ForwardPipeline::Initialize()
	{
		m_RenderTarget_Back = RenderTargetManager::Instance()->GetDefaultRenderTarget();
		m_RenderTarget_Shadow = RenderTargetManager::Instance()->CreateRenderTarget(1024, 1024, 0, true, false);

		m_ShadowSampler = g_Device->CreateSampler();
		m_ShadowSampler->SetMinFilter(Filter::Linear);
		m_ShadowSampler->SetMagFilter(Filter::Linear);
		m_ShadowSampler->SetWrapS(WrapMode::Clamp_To_Edge);
		m_ShadowSampler->SetWrapT(WrapMode::Clamp_To_Edge);

		m_PrefilterMap = TextureManager::Instance()->GetCubeTexture(s_PrefilterPath);
		m_PrefilterSampler = g_Device->CreateSampler();
		m_PrefilterSampler->SetMinFilter(Filter::Linear);
		m_PrefilterSampler->SetMagFilter(Filter::Linear);
		m_PrefilterSampler->SetWrapS(WrapMode::Clamp_To_Edge);
		m_PrefilterSampler->SetWrapT(WrapMode::Clamp_To_Edge);
		m_PrefilterSampler->SetWrapR(WrapMode::Clamp_To_Edge);

		m_BRDF_LUT = TextureManager::Instance()->GetTexture(s_BRDF_LUT_Path);
		m_BRDF_LUTSampler = g_Device->CreateSampler();
		m_BRDF_LUTSampler->SetMinFilter(Filter::Nearest);
		m_BRDF_LUTSampler->SetMagFilter(Filter::Nearest);
		m_BRDF_LUTSampler->SetWrapS(WrapMode::Clamp_To_Edge);
		m_BRDF_LUTSampler->SetWrapT(WrapMode::Clamp_To_Edge);

		return true;
	}

	void ForwardPipeline::Uninitialize()
	{
		RenderTargetManager::Instance()->DeleteRenderTarget(m_RenderTarget_Shadow);
		m_RenderTarget_Shadow = nullptr;
	}

	void ForwardPipeline::Render(const WorldContext * context)
	{
		// render shadow map
		RenderShadow(context);
		// render opaque primitives
		RenderOpaque(context);
		// render transparent primitives
		RenderTransparent(context);
	}

	void ForwardPipeline::RenderShadow(const WorldContext * context)
	{
		m_RenderTarget_Shadow->Bind();
		{
			const CameraContext * tmp_camera = context->GetCameraContext();
			const LightContext * tmp_light = context->GetLightContext();
			std::vector<RenderContext *> tmp_renderContexts = context->GetRenderContexts();

			m_ShadowCameraContext = GenerateShadowCameraContext(tmp_light, tmp_camera);

			// clear screen
			g_Device->Clear((BitField)BufferBit::Depth);

			// sort commands
			std::stable_sort(tmp_renderContexts.begin(), tmp_renderContexts.end(),
				[](const RenderContext * left, const RenderContext * right) -> bool
				{
					bool tmp_reuslt = false;

					IMaterial * leftMaterial = left->GetMaterial();
					IMaterial * rightMaterial = right->GetMaterial();
					if (leftMaterial && rightMaterial)
					{
						if (leftMaterial->GetRenderPriority() < rightMaterial->GetRenderPriority())
						{
							tmp_reuslt = true;
						}
					}

					return tmp_reuslt;
				}
			);

			// render commands
			for (size_t i = 0; i < tmp_renderContexts.size(); i++)
			{
				const RenderContext * tmp_renderContext = tmp_renderContexts[i];
				IMaterial * tmp_material = tmp_renderContext->GetMaterial();

				// skip no shadow objects or material with no shadow pass
				if (!tmp_renderContext->GetCastShadow() || !tmp_material->HasPass(ShaderPass::Shadow))
					continue;

				// material
				tmp_material->SetModelUniform(tmp_renderContext);
				tmp_material->SetCameraUniform(&m_ShadowCameraContext);
				tmp_material->SetLightUniform(tmp_light);
				tmp_material->SetTexture("prefilterMap", m_PrefilterMap);
				tmp_material->SetSampler("prefilterMap", m_PrefilterSampler);
				tmp_material->SetTexture("brdfLUT", m_BRDF_LUT);
				tmp_material->SetSampler("brdfLUT", m_BRDF_LUTSampler);
				tmp_material->Use(ShaderPass::Shadow);

				// vertex buffer
				IVertexArray * tmp_vertexArray = tmp_renderContext->GetVertexArray();
				tmp_vertexArray->Bind();

				// draw command
				g_Device->DrawElement(DrawMode::TRIANGLES, tmp_vertexArray->GetIndexCount(), ValueType::UInt, 0);

				tmp_vertexArray->UnBind();
			}
		}
	}

	void ForwardPipeline::RenderOpaque(const WorldContext * context)
	{
		m_RenderTarget_Back->Bind();
		{
			const CameraContext * tmp_camera = context->GetCameraContext();
			const LightContext * tmp_light = context->GetLightContext();
			std::vector<RenderContext *> tmp_renderContexts = context->GetRenderContexts();

			// clear screen
			const glm::vec3 & tmp_color = tmp_camera->GetClearColor();
			g_Device->SetClearColor(tmp_color.r, tmp_color.g, tmp_color.b, 1.0f);
			g_Device->Clear((BitField)BufferBit::All);

			// sort commands
			std::stable_sort(tmp_renderContexts.begin(), tmp_renderContexts.end(),
				[](const RenderContext * left, const RenderContext * right) -> bool
				{
					bool tmp_reuslt = false;

					IMaterial * leftMaterial = left->GetMaterial();
					IMaterial * rightMaterial = right->GetMaterial();
					if (leftMaterial && rightMaterial)
					{
						if (leftMaterial->GetRenderPriority() < rightMaterial->GetRenderPriority())
						{
							tmp_reuslt = true;
						}
					}

					return tmp_reuslt;
				}
			);

			// render commands
			for (size_t i = 0; i < tmp_renderContexts.size(); i++)
			{
				const RenderContext * tmp_renderContext = tmp_renderContexts[i];

				// material
				IMaterial * tmp_material = tmp_renderContext->GetMaterial();
				tmp_material->SetModelUniform(tmp_renderContext);
				tmp_material->SetCameraUniform(tmp_camera);
				tmp_material->SetLightUniform(tmp_light);
				tmp_material->SetShadowUniform(&m_ShadowCameraContext);
				tmp_material->SetTexture("shadowmap", m_RenderTarget_Shadow->GetDepthAttachment());
				tmp_material->SetSampler("shadowmap", m_ShadowSampler);
				tmp_material->SetTexture("prefilterMap", m_PrefilterMap);
				tmp_material->SetSampler("prefilterMap", m_PrefilterSampler);
				tmp_material->SetTexture("brdfLUT", m_BRDF_LUT);
				tmp_material->SetSampler("brdfLUT", m_BRDF_LUTSampler);
				tmp_material->Use(ShaderPass::Forward);

				// vertex buffer
				IVertexArray * tmp_vertexArray = tmp_renderContext->GetVertexArray();
				tmp_vertexArray->Bind();

				// draw command
				g_Device->DrawElement(DrawMode::TRIANGLES, tmp_vertexArray->GetIndexCount(), ValueType::UInt, 0);

				tmp_vertexArray->UnBind();
			}
		}
	}

	void ForwardPipeline::RenderTransparent(const WorldContext * context)
	{

	}

	CameraContext ForwardPipeline::GenerateShadowCameraContext(const LightContext * light, const CameraContext * camera)
	{
		// get any view matrix of light
		auto tmp_dirContext = light->GetDirectionContext();
		glm::mat4x4 tmp_view = tmp_dirContext->GetDynamicViewMatrix(glm::vec3(0.0f, 0.0f, 0.0f));

		// transform to light space
		std::vector<glm::vec4> tmp_points = camera->GetFrustumPoints(0.0f, 50.0f);
		for (size_t i = 0; i < tmp_points.size(); ++i)
		{
			tmp_points[i] = tmp_view * tmp_points[i];
		}

		// calculate AABB box
		glm::vec4 tmp_minPoint = tmp_points[0];
		glm::vec4 tmp_maxPoint = tmp_points[0];
		for (size_t i = 1; i < tmp_points.size(); ++i)
		{
			tmp_minPoint = glm::min(tmp_minPoint, tmp_points[i]);
			tmp_maxPoint = glm::max(tmp_maxPoint, tmp_points[i]);
		}
		glm::vec3 tmp_center = (tmp_minPoint + tmp_maxPoint) / 2.0f;
		tmp_center.z = tmp_maxPoint.z;
		tmp_center = glm::inverse(tmp_view) * glm::vec4(tmp_center, 1.0f);

		// build new camera context
		CameraContext tmp_newContext;
		tmp_newContext.SetProjectionMode(ProjectionMode::Ortho);
		tmp_newContext.SetSize(tmp_maxPoint.y - tmp_minPoint.y);
		tmp_newContext.SetAspect((tmp_maxPoint.x - tmp_minPoint.x) / (tmp_maxPoint.y - tmp_minPoint.y)); // (camera->GetAspect());
		tmp_newContext.SetNearPlane(0.0f);
		tmp_newContext.SetFarPlane(tmp_maxPoint.z - tmp_minPoint.z);
		tmp_newContext.SetRotation(tmp_dirContext->m_Rotation);
		tmp_newContext.SetPosition(tmp_center);
		tmp_newContext.Apply();
		
		return tmp_newContext;
	}
}