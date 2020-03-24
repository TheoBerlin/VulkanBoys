#include "SceneVK.h"

#include "Core/Material.h"

#include "Vulkan/BufferVK.h"
#include "Vulkan/DescriptorPoolVK.h"
#include "Vulkan/DeviceVK.h"
#include "Vulkan/GraphicsContextVK.h"
#include "Vulkan/MeshVK.h"
#include "Vulkan/PipelineLayoutVK.h"
#include "Vulkan/RenderingHandlerVK.h"
#include "Vulkan/SamplerVK.h"
#include "Vulkan/Texture2DVK.h"

#include "Vulkan/CommandPoolVK.h"
#include "Vulkan/CommandBufferVK.h"

#include "Core/TaskDispatcher.h"

#include <algorithm>
#include <tinyobjloader/tiny_obj_loader.h>
#include <imgui/imgui.h>

#ifdef max
    #undef max
#endif

SceneVK::SceneVK(IGraphicsContext* pContext, const RenderingHandlerVK* pRenderingHandler) :
	m_pContext(reinterpret_cast<GraphicsContextVK*>(pContext)),
	m_pCameraBuffer(pRenderingHandler->getCameraBufferGraphics()),
	m_pScratchBuffer(nullptr),
	m_pInstanceBuffer(nullptr),
	m_pGarbageScratchBuffer(nullptr),
	m_TotalNumberOfVertices(0),
	m_TotalNumberOfIndices(0),
	m_pGarbageInstanceBuffer(nullptr),
	m_pCombinedVertexBuffer(nullptr),
	m_pCombinedIndexBuffer(nullptr),
	m_pMeshIndexBuffer(nullptr),
	m_NumBottomLevelAccelerationStructures(0),
	m_pTempCommandPool(nullptr),
	m_pTempCommandBuffer(nullptr),
	m_TopLevelIsDirty(false),
	m_BottomLevelIsDirty(false),
	m_TransformDataIsDirty(false),
	m_MaterialDataIsDirty(false),
	m_MeshDataIsDirty(false),
	m_pDefaultTexture(nullptr),
	m_pDefaultNormal(nullptr),
	m_pDefaultSampler(nullptr),
	m_pMaterialParametersBuffer(nullptr),
	m_pTransformsBufferGraphics(nullptr),
	m_pTransformsBufferCompute(nullptr),
	m_pGarbageTransformsBufferGraphics(nullptr),
	m_pGarbageTransformsBufferCompute(nullptr),
	m_DebugParametersDirty(false),
	m_pProfiler(nullptr),
	m_RayTracingEnabled(pContext->isRayTracingEnabled()),
	m_pDescriptorPool(nullptr),
	m_pGeometryPipelineLayout(nullptr),
	m_pGeometryDescriptorSetLayout(nullptr)
{
	m_pDevice = reinterpret_cast<DeviceVK*>(m_pContext->getDevice());
}

SceneVK::~SceneVK()
{
	SAFEDELETE(m_pProfiler);

	if (m_pTempCommandBuffer != nullptr)
	{
		m_pTempCommandPool->freeCommandBuffer(&m_pTempCommandBuffer);
		m_pTempCommandBuffer = nullptr;
	}

	SAFEDELETE(m_pDescriptorPool);
	SAFEDELETE(m_pGeometryDescriptorSetLayout);
	SAFEDELETE(m_pGeometryPipelineLayout);

	SAFEDELETE(m_pTempCommandPool);
	SAFEDELETE(m_pScratchBuffer);
	SAFEDELETE(m_pInstanceBuffer);
	SAFEDELETE(m_pGarbageScratchBuffer);
	SAFEDELETE(m_pGarbageInstanceBuffer);
	SAFEDELETE(m_pCombinedVertexBuffer);
	SAFEDELETE(m_pCombinedIndexBuffer);
	SAFEDELETE(m_pMeshIndexBuffer);
	SAFEDELETE(m_pDefaultTexture);
	SAFEDELETE(m_pDefaultNormal);
	SAFEDELETE(m_pDefaultSampler);

	SAFEDELETE(m_pMaterialParametersBuffer);

	SAFEDELETE(m_pTransformsBufferGraphics);
	SAFEDELETE(m_pTransformsBufferCompute);
	SAFEDELETE(m_pGarbageTransformsBufferGraphics);
	SAFEDELETE(m_pGarbageTransformsBufferCompute);

	for (auto& bottomLevelAccelerationStructurePerMesh : m_NewBottomLevelAccelerationStructures)
	{
		//All these BLASs are the same on GPU side, so we can just grab the first one and free it
		auto newBottomLevelAccelerationStructure = bottomLevelAccelerationStructurePerMesh.second.begin();
		if (newBottomLevelAccelerationStructure->second.Memory != VK_NULL_HANDLE)
		{
			vkFreeMemory(m_pDevice->getDevice(), newBottomLevelAccelerationStructure->second.Memory, nullptr);
			m_pDevice->vkDestroyAccelerationStructureNV(m_pDevice->getDevice(), newBottomLevelAccelerationStructure->second.AccelerationStructure, nullptr);
		}
	}
	m_NewBottomLevelAccelerationStructures.clear();

	for (auto& bottomLevelAccelerationStructurePerMesh : m_FinalizedBottomLevelAccelerationStructures)
	{
		//All these BLASs are the same on GPU side, so we can just grab the first one and free it
		auto finalizedBottomLevelAccelerationStructure = bottomLevelAccelerationStructurePerMesh.second.begin();
		if (finalizedBottomLevelAccelerationStructure->second.Memory != VK_NULL_HANDLE)
		{
			vkFreeMemory(m_pDevice->getDevice(), finalizedBottomLevelAccelerationStructure->second.Memory, nullptr);
			m_pDevice->vkDestroyAccelerationStructureNV(m_pDevice->getDevice(), finalizedBottomLevelAccelerationStructure->second.AccelerationStructure, nullptr);
		}
	}
	m_FinalizedBottomLevelAccelerationStructures.clear();


	if (m_TopLevelAccelerationStructure.Memory != VK_NULL_HANDLE)
	{
		vkFreeMemory(m_pDevice->getDevice(), m_TopLevelAccelerationStructure.Memory, nullptr);
		m_TopLevelAccelerationStructure.Memory = VK_NULL_HANDLE;
	}

	if (m_TopLevelAccelerationStructure.AccelerationStructure != VK_NULL_HANDLE)
	{
		m_pDevice->vkDestroyAccelerationStructureNV(m_pDevice->getDevice(), m_TopLevelAccelerationStructure.AccelerationStructure, nullptr);
		m_TopLevelAccelerationStructure.AccelerationStructure = VK_NULL_HANDLE;
	}

	m_TopLevelAccelerationStructure.Handle = VK_NULL_HANDLE;

	for (auto pair : m_SceneTextures)
	{
		ITexture2D* pTexture = pair.second;
		SAFEDELETE(pTexture);
	}
	m_SceneTextures.clear();

	for (uint32_t m = 0; m < m_SceneMaterials.size(); m++)
	{
		SAFEDELETE(m_SceneMaterials[m]);
	}
	m_SceneMaterials.clear();

	for (uint32_t m = 0; m < m_SceneMeshes.size(); m++)
	{
		SAFEDELETE(m_SceneMeshes[m]);
	}
	m_SceneMeshes.clear();
}

bool SceneVK::loadFromFile(const std::string& dir, const std::string& fileName)
{
	tinyobj::attrib_t attributes;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attributes, &shapes, &materials, &warn, &err, (dir + fileName).c_str(), dir.c_str(), true, false))
	{
		LOG("Failed to load scene '%s'. Warning: %s Error: %s", (dir + fileName).c_str(), warn.c_str(), err.c_str());
		return false;
	}

	m_SceneMeshes.resize(shapes.size());
	m_SceneMaterials.resize(materials.size() + 1);

	for (uint32_t i = 0; i < shapes.size(); i++)
	{
		m_SceneMeshes[i] = nullptr;
	}

	for (uint32_t i = 0; i < materials.size() + 1; i++)
	{
		m_SceneMaterials[i] = nullptr;
	}

	SamplerParams samplerParams = {};
	samplerParams.MinFilter = VK_FILTER_LINEAR;
	samplerParams.MagFilter = VK_FILTER_LINEAR;
	samplerParams.WrapModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerParams.WrapModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	Material* pDefaultMaterial = DBG_NEW Material();
	pDefaultMaterial->setAlbedo(glm::vec4(1.0f));
	pDefaultMaterial->setAmbientOcclusion(1.0f);
	pDefaultMaterial->setMetallic(1.0f);
	pDefaultMaterial->setRoughness(1.0f);
	pDefaultMaterial->createSampler(m_pContext, samplerParams);
	m_SceneMaterials[0] = pDefaultMaterial;

	for (uint32_t m = 1; m < materials.size() + 1; m++)
	{
		tinyobj::material_t& material = materials[m - 1];

		Material* pMaterial = DBG_NEW Material();
		if (material.diffuse_texname.length() > 0)
		{
			std::string filename = dir + material.diffuse_texname;
			if (m_SceneTextures.count(filename) == 0)
			{
				ITexture2D* pAlbedoMap = m_pContext->createTexture2D();
				m_SceneTextures[filename] = pAlbedoMap;

				TaskDispatcher::execute([=]
					{
						pAlbedoMap->initFromFile(filename, ETextureFormat::FORMAT_R8G8B8A8_UNORM);
					});
				pMaterial->setAlbedoMap(pAlbedoMap);
			}
			else
			{
				pMaterial->setAlbedoMap(m_SceneTextures[filename]);
			}
		}

		if (material.bump_texname.length() > 0)
		{
			std::string filename = dir + material.bump_texname;
			if (m_SceneTextures.count(filename) == 0)
			{
				ITexture2D* pNormalMap = m_pContext->createTexture2D();
				m_SceneTextures[filename] = pNormalMap;

				TaskDispatcher::execute([=]
					{
						pNormalMap->initFromFile(filename, ETextureFormat::FORMAT_R8G8B8A8_UNORM);
					});
				pMaterial->setNormalMap(pNormalMap);
			}
			else
			{
				pMaterial->setNormalMap(m_SceneTextures[filename]);
			}
		}

		if (material.ambient_texname.length() > 0)
		{
			std::string filename = dir + material.ambient_texname;
			if (m_SceneTextures.count(filename) == 0)
			{
				ITexture2D* pMetallicMap = m_pContext->createTexture2D();
				m_SceneTextures[filename] = pMetallicMap;

				TaskDispatcher::execute([=]
					{
						pMetallicMap->initFromFile(filename, ETextureFormat::FORMAT_R8G8B8A8_UNORM);
					});
				pMaterial->setMetallicMap(pMetallicMap);
			}
			else
			{
				pMaterial->setMetallicMap(m_SceneTextures[filename]);
			}
		}

		if (material.specular_highlight_texname.length() > 0)
		{
			std::string filename = dir + material.specular_highlight_texname;
			if (m_SceneTextures.count(filename) == 0)
			{
				ITexture2D* pRoughnessMap = m_pContext->createTexture2D();
				m_SceneTextures[filename] = pRoughnessMap;

				TaskDispatcher::execute([=]
					{
						pRoughnessMap->initFromFile(filename, ETextureFormat::FORMAT_R8G8B8A8_UNORM);
					});
				pMaterial->setRoughnessMap(pRoughnessMap);
			}
			else
			{
				pMaterial->setRoughnessMap(m_SceneTextures[filename]);
			}
		}

		pMaterial->setAlbedo(glm::vec4(1.0f));
		pMaterial->setAmbientOcclusion(1.0f);
		pMaterial->setMetallic(1.0f);
		pMaterial->setRoughness(1.0f);
		pMaterial->createSampler(m_pContext, samplerParams);
		m_SceneMaterials[m] = pMaterial;
	}

	glm::mat4 transform = glm::scale(glm::mat4(1.0f), glm::vec3(0.005f));
	for (uint32_t s = 0; s < shapes.size(); s++)
	{
		tinyobj::shape_t& shape = shapes[s];

		std::vector<Vertex> vertices = {};
		std::vector<uint32_t> indices = {};
		std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

		for (const tinyobj::index_t& index : shape.mesh.indices)
		{
			Vertex vertex = {};

			//Normals and texcoords are optional, while positions are required
			ASSERT(index.vertex_index >= 0);

			vertex.Position =
			{
				attributes.vertices[3 * (size_t)index.vertex_index + 0],
				attributes.vertices[3 * (size_t)index.vertex_index + 1],
				attributes.vertices[3 * (size_t)index.vertex_index + 2]
			};

			if (index.normal_index >= 0)
			{
				vertex.Normal =
				{
					attributes.normals[3 * (size_t)index.normal_index + 0],
					attributes.normals[3 * (size_t)index.normal_index + 1],
					attributes.normals[3 * (size_t)index.normal_index + 2]
				};
			}

			if (index.texcoord_index >= 0)
			{
				vertex.TexCoord =
				{
					attributes.texcoords[2 * (size_t)index.texcoord_index + 0],
					1.0f - attributes.texcoords[2 * (size_t)index.texcoord_index + 1]
				};
			}

			if (uniqueVertices.count(vertex) == 0)
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}

			indices.push_back(uniqueVertices[vertex]);
		}

		//Calculate tangents
		for (uint32_t index = 0; index < indices.size(); index += 3)
		{
			Vertex& v0 = vertices[indices[(size_t)index + 0]];
			Vertex& v1 = vertices[indices[(size_t)index + 1]];
			Vertex& v2 = vertices[indices[(size_t)index + 2]];

			v0.calculateTangent(v1, v2);
			v1.calculateTangent(v2, v0);
			v2.calculateTangent(v0, v1);
		}

		MeshVK* pMesh = reinterpret_cast<MeshVK*>(m_pContext->createMesh());
		pMesh->initFromMemory(vertices.data(), sizeof(Vertex), uint32_t(vertices.size()), indices.data(), uint32_t(indices.size()));
		m_SceneMeshes[s]  = pMesh;

		uint32_t materialIndex = shape.mesh.material_ids[0] + 1;
		Material* pMaterial = m_SceneMaterials[materialIndex];

		submitGraphicsObject(pMesh, pMaterial, transform);
	}

	return true;
}

bool SceneVK::init()
{
	if (!createGeometryPipelineLayout()) {
		LOG("--- SceneVK: Failed to create geometry pipeline layout");
		return false;
	}

	createProfiler();
	initBuffers();

	return true;
}

bool SceneVK::finalize()
{
	m_pTempCommandPool = DBG_NEW CommandPoolVK(m_pContext->getDevice(), m_pContext->getDevice()->getQueueFamilyIndices().computeFamily.value());
	m_pTempCommandPool->init();

	m_pTempCommandBuffer = m_pTempCommandPool->allocateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);


	if (!createDefaultTexturesAndSamplers())
	{
		LOG("--- SceneVK: Failed to create Default Textures and/or Samplers!");
		return false;
	}


	if (m_RayTracingEnabled)
	{
		if (!createTLAS())
		{
			LOG("--- SceneVK: Failed to create TLAS!");
			return false;
		}

		initAccelerationStructureBuffers();

		//Build BLASs
		if (!buildBLASs())
		{
			LOG("--- SceneVK: Failed to initialize BLASs!");
			return false;
		}

		//Build TLASs
		if (!buildTLAS())
		{
			LOG("--- SceneVK: Failed to initialize TLAS!");
			return false;
		}

		if (!createCombinedGraphicsObjectData())
		{
			LOG("--- SceneVK: Failed to create Combined Graphics Object Data!");
			return false;
		}
	}

	updateMaterials();
	updateTransformBuffer();

	LOG("--- SceneVK: Successfully initialized Acceleration Table!");
	return true;
}

void SceneVK::updateMeshesAndGraphicsObjects()
{
	if (m_RayTracingEnabled)
	{
		if (!m_BottomLevelIsDirty)
		{
			updateTLAS();
		}
		else
		{
			buildBLASs();
			updateTLAS();
			createCombinedGraphicsObjectData();
		}
	}

	updateTransformBuffer();
}

void SceneVK::updateMaterials()
{
	if (m_RayTracingEnabled)
	{
		for (uint32_t i = 0; i < MAX_NUM_UNIQUE_MATERIALS; i++)
		{
			if (i < m_Materials.size())
			{
				const Material* pMaterial = m_Materials[i];

				const Texture2DVK* pAlbedoMap = reinterpret_cast<const Texture2DVK*>(pMaterial->getAlbedoMap());
				const Texture2DVK* pNormalMap = reinterpret_cast<const Texture2DVK*>(pMaterial->getNormalMap());
				const Texture2DVK* pAOMap = reinterpret_cast<const Texture2DVK*>(pMaterial->getAmbientOcclusionMap());
				const Texture2DVK* pMetallicMap = reinterpret_cast<const Texture2DVK*>(pMaterial->getMetallicMap());
				const Texture2DVK* pRoughnessMap = reinterpret_cast<const Texture2DVK*>(pMaterial->getRoughnessMap());
				const SamplerVK* pSampler = reinterpret_cast<const SamplerVK*>(pMaterial->getSampler());

				m_AlbedoMaps[i] = pAlbedoMap != nullptr ? pAlbedoMap->getImageView() : m_pDefaultTexture->getImageView();
				m_NormalMaps[i] = pNormalMap != nullptr ? pNormalMap->getImageView() : m_pDefaultNormal->getImageView();
				m_AOMaps[i] = pAOMap != nullptr ? pAOMap->getImageView() : m_pDefaultTexture->getImageView();
				m_MetallicMaps[i] = pMetallicMap != nullptr ? pMetallicMap->getImageView() : m_pDefaultTexture->getImageView();
				m_RoughnessMaps[i] = pRoughnessMap != nullptr ? pRoughnessMap->getImageView() : m_pDefaultTexture->getImageView();
				m_Samplers[i] = pSampler != nullptr ? pSampler : m_pDefaultSampler;
				m_MaterialParameters[i] =
				{
					pMaterial->getAlbedo(),
					pMaterial->getMetallic()* m_SceneParameters.MetallicScale,
					pMaterial->getRoughness()* m_SceneParameters.RoughnessScale,
					pMaterial->getAmbientOcclusion()* m_SceneParameters.AOScale,
					1.0f
				};
			}
			else
			{
				m_AlbedoMaps[i] = m_pDefaultTexture->getImageView();
				m_NormalMaps[i] = m_pDefaultNormal->getImageView();
				m_AOMaps[i] = m_pDefaultTexture->getImageView();
				m_MetallicMaps[i] = m_pDefaultTexture->getImageView();
				m_RoughnessMaps[i] = m_pDefaultTexture->getImageView();
				m_Samplers[i] = m_pDefaultSampler;
				m_MaterialParameters[i] =
				{
					glm::vec4(1.0f),
					m_SceneParameters.MetallicScale,
					m_SceneParameters.RoughnessScale,
					m_SceneParameters.AOScale,
					1.0f
				};
			}
		}

		

		/*void* pDest;
		m_pMaterialParametersBuffer->map(&pDest);
		memcpy(pDest, m_MaterialParameters.data(), SIZE_IN_BYTES);
		m_pMaterialParametersBuffer->unmap();*/
	}
	else
	{
		for (uint32_t i = 0; i < MAX_NUM_UNIQUE_MATERIALS; i++)
		{
			if (i < m_Materials.size())
			{
				const Material* pMaterial = m_Materials[i];
				m_MaterialParameters[i] =
				{
					pMaterial->getAlbedo(),
					pMaterial->getMetallic() * m_SceneParameters.MetallicScale,
					pMaterial->getRoughness() * m_SceneParameters.RoughnessScale,
					pMaterial->getAmbientOcclusion() * m_SceneParameters.AOScale,
					1.0f
				};
			}
			else
			{
				m_MaterialParameters[i] =
				{
					glm::vec4(1.0f),
					m_SceneParameters.MetallicScale,
					m_SceneParameters.RoughnessScale,
					m_SceneParameters.AOScale,
					1.0f
				};
			}
		}

		//void* pDest;
		//m_pMaterialParametersBuffer->map(&pDest);
		//memcpy(pDest, m_MaterialParameters.data(), SIZE_IN_BYTES);
		//m_pMaterialParametersBuffer->unmap();
	}

	m_MaterialDataIsDirty = true;
}

void SceneVK::updateCamera(const Camera& camera)
{
	m_Camera = camera;
}

uint32_t SceneVK::submitGraphicsObject(const IMesh* pMesh, const Material* pMaterial, const glm::mat4& transform, uint8_t customMask)
{
	const MeshVK* pVulkanMesh = reinterpret_cast<const MeshVK*>(pMesh);

	uint32_t materialIndex = 0;

	if (m_RayTracingEnabled)
	{
		if (pVulkanMesh == nullptr || pMaterial == nullptr)
		{
			LOG("--- SceneVK: addGraphicsObjectInstance failed, Mesh or Material is nullptr!");
			return false;
		}

		m_TopLevelIsDirty = true;

		auto newBLASPerMesh			= m_NewBottomLevelAccelerationStructures.find(pVulkanMesh);
		auto finalizedBLASPerMesh	= m_FinalizedBottomLevelAccelerationStructures.find(pVulkanMesh);

		const BottomLevelAccelerationStructure* pBottomLevelAccelerationStructure = nullptr;

		if (newBLASPerMesh == m_NewBottomLevelAccelerationStructures.end())
		{
			//Not in new BLAS per Mesh

			if (finalizedBLASPerMesh == m_FinalizedBottomLevelAccelerationStructures.end())
			{
				//Not in finalized BLAS per Mesh either, create new BLAS

				m_BottomLevelIsDirty = true;

				pBottomLevelAccelerationStructure = createBLAS(pVulkanMesh, pMaterial);
				m_AllMeshes.push_back(pVulkanMesh);
				m_TotalNumberOfVertices += static_cast<uint32_t>(pVulkanMesh->getVertexBuffer()->getSizeInBytes() / sizeof(Vertex));
				m_TotalNumberOfIndices += static_cast<uint32_t>(pVulkanMesh->getIndexBuffer()->getSizeInBytes() / sizeof(uint32_t));
			}
			else if (finalizedBLASPerMesh->second.find(pMaterial) == finalizedBLASPerMesh->second.end())
			{
				//In finalized BLAS per Mesh but not in finalized BLAS per Material, create copy from finalized, add new BLASs per MESH to new, add BLAS to that

				m_BottomLevelIsDirty = true;
				BottomLevelAccelerationStructure blasCopy = finalizedBLASPerMesh->second.begin()->second;

				blasCopy.Index = m_NumBottomLevelAccelerationStructures;
				m_NumBottomLevelAccelerationStructures++;

				blasCopy.MaterialIndex = (uint32_t)m_Materials.size();
				m_Materials.push_back(pMaterial);

				std::map<const Material*, BottomLevelAccelerationStructure> tempBLASPerMesh;
				tempBLASPerMesh[pMaterial] = blasCopy;
				m_NewBottomLevelAccelerationStructures[pVulkanMesh] = tempBLASPerMesh;

				pBottomLevelAccelerationStructure = &m_NewBottomLevelAccelerationStructures[pVulkanMesh][pMaterial];
			}
			else
			{
				//In finalized BLAS per Mesh and and finalized BLAS per Material
				pBottomLevelAccelerationStructure = &finalizedBLASPerMesh->second[pMaterial];
			}
		}
		else if (newBLASPerMesh->second.find(pMaterial) == newBLASPerMesh->second.end())
		{
			//In new BLAS per Mesh but not in new BLAS per material, create copy from new

			m_BottomLevelIsDirty = true;
			BottomLevelAccelerationStructure blasCopy = newBLASPerMesh->second.begin()->second;

			blasCopy.Index = m_NumBottomLevelAccelerationStructures;
			m_NumBottomLevelAccelerationStructures++;

			blasCopy.MaterialIndex = (uint32_t)m_Materials.size();
			m_Materials.push_back(pMaterial);

			newBLASPerMesh->second[pMaterial] = blasCopy;
			pBottomLevelAccelerationStructure = &newBLASPerMesh->second[pMaterial];
		}
		else
		{
			//In new BLAS per Mesh and and new BLAS per Material
			pBottomLevelAccelerationStructure = &newBLASPerMesh->second[pMaterial];
		}

		GeometryInstance geometryInstance = {};
		geometryInstance.Transform = glm::transpose(transform);
		geometryInstance.InstanceId = pBottomLevelAccelerationStructure->Index; //This is not really used anymore, Todo: remove this
		geometryInstance.Mask = customMask;
		geometryInstance.InstanceOffset = 0;
		geometryInstance.Flags = VK_GEOMETRY_INSTANCE_TRIANGLE_CULL_DISABLE_BIT_NV;
		geometryInstance.AccelerationStructureHandle = pBottomLevelAccelerationStructure->Handle;
		m_GeometryInstances.push_back(geometryInstance);

		materialIndex = pBottomLevelAccelerationStructure->MaterialIndex;
	}
	else
	{
		registerMaterial(pMaterial);
	}

	m_GraphicsObjects.push_back({ pVulkanMesh, pMaterial, materialIndex });
	m_SceneTransforms.push_back({ transform, transform });

	return uint32_t(m_GraphicsObjects.size()) - 1u;
}

void SceneVK::updateGraphicsObjectTransform(uint32_t index, const glm::mat4& transform)
{
	if (m_RayTracingEnabled)
	{
		m_GeometryInstances[index].Transform = glm::transpose(transform);
	}

	GraphicsObjectTransforms& transforms = m_SceneTransforms[index];
	transforms.PrevTransform	= transforms.Transform;
	transforms.Transform		= transform;
}

void SceneVK::copySceneData(CommandBufferVK* pTransferBuffer)
{
	if (m_TransformDataIsDirty)
	{
		pTransferBuffer->updateBuffer(m_pTransformsBufferGraphics, 0, m_SceneTransforms.data(), m_SceneTransforms.size() * sizeof(GraphicsObjectTransforms));
		pTransferBuffer->updateBuffer(m_pTransformsBufferCompute, 0, m_SceneTransforms.data(), m_SceneTransforms.size() * sizeof(GraphicsObjectTransforms));
		
		m_TransformDataIsDirty = false;
	}

	if (m_MaterialDataIsDirty)
	{
		constexpr uint32_t SIZE_IN_BYTES = MAX_NUM_UNIQUE_MATERIALS * sizeof(MaterialParameters);
		pTransferBuffer->updateBuffer(m_pMaterialParametersBuffer, 0, m_MaterialParameters.data(), SIZE_IN_BYTES);

		m_MaterialDataIsDirty = false;
	}

	if (m_MeshDataIsDirty)
	{
		uint32_t vertexBufferOffset = 0;
		uint32_t indexBufferOffset = 0;
		uint64_t meshIndexBufferOffset = 0;
		uint32_t currentCustomInstanceIndexNV = 0;

		for (auto& pMesh : m_AllMeshes)
		{
			uint32_t numVertices = pMesh->getVertexCount();
			uint32_t numIndices = pMesh->getIndexCount();

			pTransferBuffer->copyBuffer(reinterpret_cast<BufferVK*>(pMesh->getVertexBuffer()), 0, m_pCombinedVertexBuffer, vertexBufferOffset * sizeof(Vertex), numVertices * sizeof(Vertex));
			pTransferBuffer->copyBuffer(reinterpret_cast<BufferVK*>(pMesh->getIndexBuffer()), 0, m_pCombinedIndexBuffer, indexBufferOffset * sizeof(uint32_t), numIndices * sizeof(uint32_t));

			for (auto& bottomLevelAccelerationStructure : m_FinalizedBottomLevelAccelerationStructures[pMesh])
			{
				for (uint32_t i = 0; i < m_GraphicsObjects.size(); i++)
				{
					GraphicsObjectVK& graphicsObject = m_GraphicsObjects[i];

					if (graphicsObject.pMesh == pMesh && graphicsObject.pMaterial == bottomLevelAccelerationStructure.first)
					{
						m_GeometryInstances[i].InstanceId = currentCustomInstanceIndexNV;
					}
				}

				uint32_t meshIndices[3] = { vertexBufferOffset, indexBufferOffset, bottomLevelAccelerationStructure.second.MaterialIndex };
				pTransferBuffer->updateBuffer(m_pMeshIndexBuffer, meshIndexBufferOffset * sizeof(uint32_t), meshIndices, 3 * sizeof(uint32_t));

				meshIndexBufferOffset += 3;
				currentCustomInstanceIndexNV++;
			}

			vertexBufferOffset	+= numVertices;
			indexBufferOffset	+= numIndices;
		}

		m_MeshDataIsDirty = false;
	}
}

void SceneVK::updateSceneData()
{
	if (m_pGarbageTransformsBufferGraphics || m_MaterialDataIsDirty)
	{
		m_pDevice->wait();

		for (auto& instance : m_MeshTable)
		{
			instance.second.pDescriptorSets->writeStorageBufferDescriptor(m_pMaterialParametersBuffer, MATERIAL_PARAMETERS_BINDING);
			instance.second.pDescriptorSets->writeStorageBufferDescriptor(m_pTransformsBufferGraphics, INSTANCE_TRANSFORMS_BINDING);
		}

		cleanGarbage();
	}
}

DescriptorSetVK* SceneVK::getDescriptorSetFromMeshAndMaterial(const MeshVK* pMesh, const Material* pMaterial)
{
	MeshFilter filter = {};
	filter.pMesh		= pMesh;
	filter.pMaterial	= pMaterial;

	if (m_MeshTable.count(filter) == 0)
	{
		DescriptorSetVK* pDescriptorSet = m_pDescriptorPool->allocDescriptorSet(m_pGeometryDescriptorSetLayout);
		pDescriptorSet->writeUniformBufferDescriptor(m_pCameraBuffer, CAMERA_BUFFER_BINDING);

		BufferVK* pVertBuffer = reinterpret_cast<BufferVK*>(pMesh->getVertexBuffer());
		pDescriptorSet->writeStorageBufferDescriptor(pVertBuffer, VERTEX_BUFFER_BINDING);

		SamplerVK* pSampler = reinterpret_cast<SamplerVK*>(pMaterial->getSampler());

		Texture2DVK* pAlbedo = m_pDefaultTexture;
		if (pMaterial->hasAlbedoMap())
		{
			pAlbedo = reinterpret_cast<Texture2DVK*>(pMaterial->getAlbedoMap());
		}

		ImageViewVK* pAlbedoView = pAlbedo->getImageView();
		pDescriptorSet->writeCombinedImageDescriptors(&pAlbedoView, &pSampler, 1, ALBEDO_MAP_BINDING);

		Texture2DVK* pNormal = m_pDefaultNormal;
		if (pMaterial->hasNormalMap())
		{
			pNormal = reinterpret_cast<Texture2DVK*>(pMaterial->getNormalMap());
		}

		ImageViewVK* pNormalView = pNormal->getImageView();
		pDescriptorSet->writeCombinedImageDescriptors(&pNormalView, &pSampler, 1, NORMAL_MAP_BINDING);

		Texture2DVK* pAO = m_pDefaultTexture;
		if (pMaterial->hasAmbientOcclusionMap())
		{
			pAO = reinterpret_cast<Texture2DVK*>(pMaterial->getAmbientOcclusionMap());
		}

		ImageViewVK* pAOView = pAO->getImageView();
		pDescriptorSet->writeCombinedImageDescriptors(&pAOView, &pSampler, 1, AO_MAP_BINDING);

		Texture2DVK* pMetallic = m_pDefaultTexture;
		if (pMaterial->hasMetallicMap())
		{
			pMetallic = reinterpret_cast<Texture2DVK*>(pMaterial->getMetallicMap());
		}

		ImageViewVK* pMetallicView = pMetallic->getImageView();
		pDescriptorSet->writeCombinedImageDescriptors(&pMetallicView, &pSampler, 1, METALLIC_MAP_BINDING);

		Texture2DVK* pRoughness = m_pDefaultTexture;
		if (pMaterial->hasRoughnessMap())
		{
			pRoughness = reinterpret_cast<Texture2DVK*>(pMaterial->getRoughnessMap());
		}

		ImageViewVK* pRoughnessView = pRoughness->getImageView();
		pDescriptorSet->writeCombinedImageDescriptors(&pRoughnessView, &pSampler, 1, ROUGHNESS_MAP_BINDING);

		pDescriptorSet->writeStorageBufferDescriptor(m_pMaterialParametersBuffer, MATERIAL_PARAMETERS_BINDING);
		pDescriptorSet->writeStorageBufferDescriptor(m_pTransformsBufferGraphics, INSTANCE_TRANSFORMS_BINDING);

		MeshPipeline meshPipeline = {};
		meshPipeline.pDescriptorSets = pDescriptorSet;

		m_MeshTable.insert(std::make_pair(filter, meshPipeline));
		return pDescriptorSet;
	}

	MeshPipeline meshPipeline = m_MeshTable[filter];
	return meshPipeline.pDescriptorSets;
}

bool SceneVK::createDefaultTexturesAndSamplers()
{
	uint8_t whitePixels[] = { 255, 255, 255, 255 };
	m_pDefaultTexture = DBG_NEW Texture2DVK(m_pContext->getDevice());
	if (!m_pDefaultTexture->initFromMemory(whitePixels, 1, 1, ETextureFormat::FORMAT_R8G8B8A8_UNORM, 0, false))
	{
		return false;
	}

	uint8_t pixels[] = { 127, 127, 255, 255 };
	m_pDefaultNormal = DBG_NEW Texture2DVK(m_pContext->getDevice());
	if (!m_pDefaultNormal->initFromMemory(pixels, 1, 1, ETextureFormat::FORMAT_R8G8B8A8_UNORM, 0, false))
	{
		return false;
	}

	SamplerParams samplerParams = {};
	samplerParams.MinFilter = VkFilter::VK_FILTER_LINEAR;
	samplerParams.MagFilter = VkFilter::VK_FILTER_LINEAR;
	samplerParams.WrapModeU = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerParams.WrapModeV = samplerParams.WrapModeU;
	samplerParams.WrapModeW = samplerParams.WrapModeU;

	m_pDefaultSampler = DBG_NEW SamplerVK(m_pContext->getDevice());
	if (!m_pDefaultSampler->init(samplerParams))
	{
		return false;
	}

	m_AlbedoMaps.resize(MAX_NUM_UNIQUE_MATERIALS);
	m_NormalMaps.resize(MAX_NUM_UNIQUE_MATERIALS);
	m_AOMaps.resize(MAX_NUM_UNIQUE_MATERIALS);
	m_MetallicMaps.resize(MAX_NUM_UNIQUE_MATERIALS);
	m_RoughnessMaps.resize(MAX_NUM_UNIQUE_MATERIALS);
	m_Samplers.resize(MAX_NUM_UNIQUE_MATERIALS);
	m_MaterialParameters.resize(MAX_NUM_UNIQUE_MATERIALS);

	return true;
}

void SceneVK::initBuffers()
{
	BufferParams materialParametersBufferParams = {};
	materialParametersBufferParams.Usage			= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	materialParametersBufferParams.MemoryProperty	= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	materialParametersBufferParams.SizeInBytes		= sizeof(MaterialParameters) * MAX_NUM_UNIQUE_MATERIALS;
	materialParametersBufferParams.IsExclusive		= true;

	m_pMaterialParametersBuffer = reinterpret_cast<BufferVK*>(m_pContext->createBuffer());
	m_pMaterialParametersBuffer->init(materialParametersBufferParams);

	BufferParams transformBufferParams = {};
	transformBufferParams.Usage				= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	transformBufferParams.MemoryProperty	= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	transformBufferParams.SizeInBytes		= sizeof(GraphicsObjectTransforms) * NUM_INITIAL_GRAPHICS_OBJECTS;
	transformBufferParams.IsExclusive		= true;

	m_pTransformsBufferGraphics = reinterpret_cast<BufferVK*>(m_pContext->createBuffer());
	m_pTransformsBufferGraphics->init(transformBufferParams);

	m_pTransformsBufferCompute = reinterpret_cast<BufferVK*>(m_pContext->createBuffer());
	m_pTransformsBufferCompute->init(transformBufferParams);
}

bool SceneVK::createGeometryPipelineLayout()
{
	//Descriptorpool
	DescriptorCounts descriptorCounts = {};
	descriptorCounts.m_SampledImages	= 4096;
	descriptorCounts.m_StorageImages	= 1024;
	descriptorCounts.m_StorageBuffers	= 2048;
	descriptorCounts.m_UniformBuffers	= 1024;

	m_pDescriptorPool = DBG_NEW DescriptorPoolVK(m_pContext->getDevice());
	if (!m_pDescriptorPool->init(descriptorCounts, 512))
	{
		return false;
	}

	//GeometryPass
	m_pGeometryDescriptorSetLayout = DBG_NEW DescriptorSetLayoutVK(m_pContext->getDevice());
	m_pGeometryDescriptorSetLayout->addBindingUniformBuffer(VK_SHADER_STAGE_VERTEX_BIT, CAMERA_BUFFER_BINDING, 1);
	m_pGeometryDescriptorSetLayout->addBindingStorageBuffer(VK_SHADER_STAGE_VERTEX_BIT, VERTEX_BUFFER_BINDING, 1);
	m_pGeometryDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_FRAGMENT_BIT, nullptr, ALBEDO_MAP_BINDING, 1);
	m_pGeometryDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_FRAGMENT_BIT, nullptr, NORMAL_MAP_BINDING, 1);
	m_pGeometryDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_FRAGMENT_BIT, nullptr, AO_MAP_BINDING, 1);
	m_pGeometryDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_FRAGMENT_BIT, nullptr, METALLIC_MAP_BINDING, 1);
	m_pGeometryDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_FRAGMENT_BIT, nullptr, ROUGHNESS_MAP_BINDING, 1);
	m_pGeometryDescriptorSetLayout->addBindingStorageBuffer(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, MATERIAL_PARAMETERS_BINDING, 1);
	m_pGeometryDescriptorSetLayout->addBindingStorageBuffer(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, INSTANCE_TRANSFORMS_BINDING, 1);

	if (!m_pGeometryDescriptorSetLayout->finalize())
	{
		return false;
	}

	//Transform and Color
	VkPushConstantRange pushConstantRange = {};
	pushConstantRange.size			= sizeof(glm::mat4) + sizeof(glm::vec4) + sizeof(glm::vec3);
	pushConstantRange.stageFlags	= VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset		= 0;
	std::vector<VkPushConstantRange> pushConstantRanges = { pushConstantRange };
	std::vector<const DescriptorSetLayoutVK*> descriptorSetLayouts = { m_pGeometryDescriptorSetLayout };

	m_pGeometryPipelineLayout = DBG_NEW PipelineLayoutVK(m_pContext->getDevice());
	if (!m_pGeometryPipelineLayout->init(descriptorSetLayouts, pushConstantRanges))
	{
		return false;
	}

	return true;
}

void SceneVK::initAccelerationStructureBuffers()
{
	// Acceleration structure build requires some scratch space to store temporary information
	VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo = {};
	memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
	memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV;

	VkMemoryRequirements2 memReqTLAS;
	memoryRequirementsInfo.accelerationStructure = m_TopLevelAccelerationStructure.AccelerationStructure;
	m_pDevice->vkGetAccelerationStructureMemoryRequirementsNV(m_pDevice->getDevice(), &memoryRequirementsInfo, &memReqTLAS);

	BufferParams scratchBufferParams = {};
	scratchBufferParams.Usage			= VK_BUFFER_USAGE_RAY_TRACING_BIT_NV;
	scratchBufferParams.MemoryProperty	= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	scratchBufferParams.SizeInBytes		= std::max(findMaxMemReqBLAS(), memReqTLAS.memoryRequirements.size);
	scratchBufferParams.IsExclusive		= true;

	m_pScratchBuffer = reinterpret_cast<BufferVK*>(m_pContext->createBuffer());
	m_pScratchBuffer->init(scratchBufferParams);

	BufferParams instanceBufferParams = {};
	instanceBufferParams.Usage			= VK_BUFFER_USAGE_RAY_TRACING_BIT_NV | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	instanceBufferParams.MemoryProperty	= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	instanceBufferParams.SizeInBytes	= sizeof(GeometryInstance) * m_GeometryInstances.size();
	instanceBufferParams.IsExclusive	= true;

	m_pInstanceBuffer = reinterpret_cast<BufferVK*>(m_pContext->createBuffer());
	m_pInstanceBuffer->init(instanceBufferParams);
}

SceneVK::BottomLevelAccelerationStructure* SceneVK::createBLAS(const MeshVK* pMesh, const Material* pMaterial)
{
	BottomLevelAccelerationStructure bottomLevelAccelerationStructure = {};

	bottomLevelAccelerationStructure.Geometry.sType = VK_STRUCTURE_TYPE_GEOMETRY_NV;
	bottomLevelAccelerationStructure.Geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_NV;
	bottomLevelAccelerationStructure.Geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_GEOMETRY_TRIANGLES_NV;
	bottomLevelAccelerationStructure.Geometry.geometry.triangles.vertexData = ((BufferVK*)pMesh->getVertexBuffer())->getBuffer();
	bottomLevelAccelerationStructure.Geometry.geometry.triangles.vertexOffset = 0;
	bottomLevelAccelerationStructure.Geometry.geometry.triangles.vertexCount = pMesh->getVertexCount();
	bottomLevelAccelerationStructure.Geometry.geometry.triangles.vertexStride = sizeof(Vertex);
	bottomLevelAccelerationStructure.Geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
	bottomLevelAccelerationStructure.Geometry.geometry.triangles.indexData = ((BufferVK*)pMesh->getIndexBuffer())->getBuffer();
	bottomLevelAccelerationStructure.Geometry.geometry.triangles.indexOffset = 0;
	bottomLevelAccelerationStructure.Geometry.geometry.triangles.indexCount = pMesh->getIndexCount();
	bottomLevelAccelerationStructure.Geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
	bottomLevelAccelerationStructure.Geometry.geometry.triangles.transformData = VK_NULL_HANDLE;
	bottomLevelAccelerationStructure.Geometry.geometry.triangles.transformOffset = 0;
	bottomLevelAccelerationStructure.Geometry.geometry.aabbs = {};
	bottomLevelAccelerationStructure.Geometry.geometry.aabbs.sType = { VK_STRUCTURE_TYPE_GEOMETRY_AABB_NV };
	bottomLevelAccelerationStructure.Geometry.flags = VK_GEOMETRY_OPAQUE_BIT_NV;

	VkAccelerationStructureInfoNV accelerationStructureInfo = {};
	accelerationStructureInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
	accelerationStructureInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
	accelerationStructureInfo.instanceCount = 0;
	accelerationStructureInfo.geometryCount = 1;
	accelerationStructureInfo.pGeometries = &bottomLevelAccelerationStructure.Geometry;

	VkAccelerationStructureCreateInfoNV accelerationStructureCreateInfo = {};
	accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
	accelerationStructureCreateInfo.info = accelerationStructureInfo;
	VK_CHECK_RESULT(m_pDevice->vkCreateAccelerationStructureNV(m_pDevice->getDevice(), &accelerationStructureCreateInfo, nullptr, &bottomLevelAccelerationStructure.AccelerationStructure), "--- RayTracingScene: Could not create BLAS!");

	VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo = {};
	memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
	memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;
	memoryRequirementsInfo.accelerationStructure = bottomLevelAccelerationStructure.AccelerationStructure;

	VkMemoryRequirements2 memoryRequirements2 = {};
	m_pDevice->vkGetAccelerationStructureMemoryRequirementsNV(m_pDevice->getDevice(), &memoryRequirementsInfo, &memoryRequirements2);

	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memoryRequirements2.memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = findMemoryType(m_pDevice->getPhysicalDevice(), memoryRequirements2.memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK_RESULT(vkAllocateMemory(m_pDevice->getDevice(), &memoryAllocateInfo, nullptr, &bottomLevelAccelerationStructure.Memory), "--- RayTracingScene: Could not allocate memory for BLAS!");

	VkBindAccelerationStructureMemoryInfoNV accelerationStructureMemoryInfo = {};
	accelerationStructureMemoryInfo.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
	accelerationStructureMemoryInfo.accelerationStructure = bottomLevelAccelerationStructure.AccelerationStructure;
	accelerationStructureMemoryInfo.memory = bottomLevelAccelerationStructure.Memory;
	VK_CHECK_RESULT(m_pDevice->vkBindAccelerationStructureMemoryNV(m_pDevice->getDevice(), 1, &accelerationStructureMemoryInfo), "--- RayTracingScene: Could not bind memory for BLAS!");

	VK_CHECK_RESULT(m_pDevice->vkGetAccelerationStructureHandleNV(m_pDevice->getDevice(), bottomLevelAccelerationStructure.AccelerationStructure, sizeof(uint64_t), &bottomLevelAccelerationStructure.Handle), "--- RayTracingScene: Could not get handle for BLAS!");

	bottomLevelAccelerationStructure.Index = m_NumBottomLevelAccelerationStructures;
	m_NumBottomLevelAccelerationStructures++;

	bottomLevelAccelerationStructure.MaterialIndex = registerMaterial(pMaterial);
	std::map<const Material*, BottomLevelAccelerationStructure> newBLASPerMesh;
	newBLASPerMesh[pMaterial] = bottomLevelAccelerationStructure;
	m_NewBottomLevelAccelerationStructures[pMesh] = newBLASPerMesh;

	return &m_NewBottomLevelAccelerationStructures[pMesh][pMaterial];
}

bool SceneVK::buildBLASs()
{
	cleanGarbage();
	updateScratchBufferForBLAS();

	//Create Memory Barrier
	VkMemoryBarrier memoryBarrier = {};
	memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	memoryBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
	memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;

	m_pTempCommandBuffer->reset(true);
	m_pTempCommandBuffer->begin(nullptr, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	for (auto& bottomLevelAccelerationStructurePerMesh : m_NewBottomLevelAccelerationStructures)
	{
		const MeshVK* pMesh = bottomLevelAccelerationStructurePerMesh.first;

		std::map<const Material*, BottomLevelAccelerationStructure>* finalizedBLASperMaterial;
		auto finalizedBLASperMaterialIt = m_FinalizedBottomLevelAccelerationStructures.find(pMesh);

		//Check if this map exists in finalized maps
		if (finalizedBLASperMaterialIt == m_FinalizedBottomLevelAccelerationStructures.end())
		{
			m_FinalizedBottomLevelAccelerationStructures[pMesh] = std::map<const Material*, BottomLevelAccelerationStructure>();
			finalizedBLASperMaterial = &m_FinalizedBottomLevelAccelerationStructures[pMesh];
		}
		else
		{
			finalizedBLASperMaterial = &finalizedBLASperMaterialIt->second;
		}

		for (auto& bottomLevelAccelerationStructure : bottomLevelAccelerationStructurePerMesh.second)
		{
			/*
				Build bottom level acceleration structure
			*/
			VkAccelerationStructureInfoNV buildInfo = {};
			buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
			buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
			buildInfo.geometryCount = 1;
			buildInfo.pGeometries = &bottomLevelAccelerationStructure.second.Geometry;

			//Todo: Make this better?
			m_pDevice->vkCmdBuildAccelerationStructureNV(
				m_pTempCommandBuffer->getCommandBuffer(),
				&buildInfo,
				VK_NULL_HANDLE,
				0,
				VK_FALSE,
				bottomLevelAccelerationStructure.second.AccelerationStructure,
				VK_NULL_HANDLE,
				m_pScratchBuffer->getBuffer(),
				0);

			finalizedBLASperMaterial->insert(std::make_pair(bottomLevelAccelerationStructure.first, bottomLevelAccelerationStructure.second));
			vkCmdPipelineBarrier(m_pTempCommandBuffer->getCommandBuffer(), VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1, &memoryBarrier, 0, 0, 0, 0);
		}

		bottomLevelAccelerationStructurePerMesh.second.clear();
	}

	m_BottomLevelIsDirty = false;
	m_NewBottomLevelAccelerationStructures.clear();

	m_pTempCommandBuffer->end();
	m_pContext->getDevice()->executeCompute(m_pTempCommandBuffer, nullptr, nullptr, 0, nullptr, 0);

	return true;
}

bool SceneVK::createTLAS()
{
	VkAccelerationStructureInfoNV accelerationStructureInfo = {};
	accelerationStructureInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
	accelerationStructureInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
	accelerationStructureInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_NV;
	accelerationStructureInfo.instanceCount = uint32_t(m_GeometryInstances.size());
	accelerationStructureInfo.geometryCount = 0;

	VkAccelerationStructureCreateInfoNV accelerationStructureCreateInfo = {};
	accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
	accelerationStructureCreateInfo.info = accelerationStructureInfo;
	VK_CHECK_RESULT_RETURN_FALSE(m_pDevice->vkCreateAccelerationStructureNV(m_pDevice->getDevice(), &accelerationStructureCreateInfo, nullptr, &m_TopLevelAccelerationStructure.AccelerationStructure), "--- RayTracingScene: Could not create TLAS!");

	VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo = {};
	memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
	memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;
	memoryRequirementsInfo.accelerationStructure = m_TopLevelAccelerationStructure.AccelerationStructure;

	VkMemoryRequirements2 memoryRequirements2 = {};
	m_pDevice->vkGetAccelerationStructureMemoryRequirementsNV(m_pDevice->getDevice(), &memoryRequirementsInfo, &memoryRequirements2);

	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memoryRequirements2.memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = findMemoryType(m_pDevice->getPhysicalDevice(), memoryRequirements2.memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK_RESULT_RETURN_FALSE(vkAllocateMemory(m_pDevice->getDevice(), &memoryAllocateInfo, nullptr, &m_TopLevelAccelerationStructure.Memory), "--- RayTracingScene: Could not allocate memory for TLAS!");

	VkBindAccelerationStructureMemoryInfoNV accelerationStructureMemoryInfo = {};
	accelerationStructureMemoryInfo.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
	accelerationStructureMemoryInfo.accelerationStructure = m_TopLevelAccelerationStructure.AccelerationStructure;
	accelerationStructureMemoryInfo.memory = m_TopLevelAccelerationStructure.Memory;
	VK_CHECK_RESULT_RETURN_FALSE(m_pDevice->vkBindAccelerationStructureMemoryNV(m_pDevice->getDevice(), 1, &accelerationStructureMemoryInfo), "--- RayTracingScene: Could not allocate bind memory for TLAS!");

	VK_CHECK_RESULT_RETURN_FALSE(m_pDevice->vkGetAccelerationStructureHandleNV(m_pDevice->getDevice(), m_TopLevelAccelerationStructure.AccelerationStructure, sizeof(uint64_t), &m_TopLevelAccelerationStructure.Handle), "--- RayTracingScene: Could not get handle for TLAS!");
	return true;
}

bool SceneVK::buildTLAS()
{
	updateInstanceBuffer();

	VkAccelerationStructureInfoNV buildInfo = {};
	buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
	buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
	buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_NV;
	buildInfo.pGeometries = 0;
	buildInfo.geometryCount = 0;
	buildInfo.instanceCount = (uint32_t)m_GeometryInstances.size();

	//Create Memory Barrier
	VkMemoryBarrier memoryBarrier = {};
	memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	memoryBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
	memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;

	m_pTempCommandBuffer->reset(true);
	m_pTempCommandBuffer->begin(nullptr, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	m_pDevice->vkCmdBuildAccelerationStructureNV(
		m_pTempCommandBuffer->getCommandBuffer(),
		&buildInfo,
		m_pInstanceBuffer->getBuffer(),
		0,
		VK_FALSE,
		m_TopLevelAccelerationStructure.AccelerationStructure,
		VK_NULL_HANDLE,
		m_pScratchBuffer->getBuffer(),
		0);

	vkCmdPipelineBarrier(m_pTempCommandBuffer->getCommandBuffer(), VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1, &memoryBarrier, 0, 0, 0, 0);

	m_pTempCommandBuffer->end();
	m_pContext->getDevice()->executeCompute(m_pTempCommandBuffer, nullptr, nullptr, 0, nullptr, 0);

	return true;
}

bool SceneVK::updateTLAS()
{
	if (m_TopLevelIsDirty)
	{
		//Instance count changed, recreate TLAS
		m_TopLevelIsDirty = false;
		m_OldTopLevelAccelerationStructure = m_TopLevelAccelerationStructure;

		if (!createTLAS())
		{
			LOG("--- SceneVK: Could not create TLAS!");
			return false;
		}

		cleanGarbage();
		updateScratchBufferForTLAS();
		updateInstanceBuffer();

		VkAccelerationStructureInfoNV buildInfo = {};
		buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
		buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
		buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_NV;
		buildInfo.pGeometries = 0;
		buildInfo.geometryCount = 0;
		buildInfo.instanceCount = uint32_t(m_GeometryInstances.size());

		//Create Memory Barrier
		VkMemoryBarrier memoryBarrier = {};
		memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		memoryBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
		memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;

		m_pTempCommandBuffer->reset(true);
		m_pTempCommandBuffer->begin(nullptr, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		m_pProfiler->reset(0, m_pTempCommandBuffer);
		m_pProfiler->beginFrame(m_pTempCommandBuffer);
		m_pProfiler->beginTimestamp(&m_TimestampBuildAccelStruct);

		m_pDevice->vkCmdBuildAccelerationStructureNV(
			m_pTempCommandBuffer->getCommandBuffer(),
			&buildInfo,
			m_pInstanceBuffer->getBuffer(),
			0,
			VK_FALSE,
			m_TopLevelAccelerationStructure.AccelerationStructure,
			VK_NULL_HANDLE,
			m_pScratchBuffer->getBuffer(),
			0);

		m_pProfiler->endTimestamp(&m_TimestampBuildAccelStruct);
		vkCmdPipelineBarrier(m_pTempCommandBuffer->getCommandBuffer(), VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1, &memoryBarrier, 0, 0, 0, 0);

		m_pProfiler->endFrame();
		m_pTempCommandBuffer->end();
		m_pContext->getDevice()->executeCompute(m_pTempCommandBuffer, nullptr, nullptr, 0, nullptr, 0);
	}
	else
	{
		//Instance count has not changed, update old TLAS

		cleanGarbage();
		updateScratchBufferForTLAS();
		updateInstanceBuffer();

		VkAccelerationStructureInfoNV buildInfo = {};
		buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
		buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
		buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_NV;
		buildInfo.pGeometries = 0;
		buildInfo.geometryCount = 0;
		buildInfo.instanceCount = uint32_t(m_GeometryInstances.size());

		//Create Memory Barrier
		VkMemoryBarrier memoryBarrier = {};
		memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		memoryBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
		memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;

		m_pTempCommandBuffer->reset(true);
		m_pTempCommandBuffer->begin(nullptr, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		m_pProfiler->reset(0, m_pTempCommandBuffer);
		m_pProfiler->beginFrame(m_pTempCommandBuffer);
		m_pProfiler->beginTimestamp(&m_TimestampBuildAccelStruct);

		m_pDevice->vkCmdBuildAccelerationStructureNV(
			m_pTempCommandBuffer->getCommandBuffer(),
			&buildInfo,
			m_pInstanceBuffer->getBuffer(),
			0,
			VK_TRUE,
			m_TopLevelAccelerationStructure.AccelerationStructure,
			m_TopLevelAccelerationStructure.AccelerationStructure,
			m_pScratchBuffer->getBuffer(),
			0);

		m_pProfiler->endTimestamp(&m_TimestampBuildAccelStruct);
		vkCmdPipelineBarrier(m_pTempCommandBuffer->getCommandBuffer(), VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1, &memoryBarrier, 0, 0, 0, 0);

		m_pProfiler->endFrame();
		m_pTempCommandBuffer->end();
		m_pContext->getDevice()->executeCompute(m_pTempCommandBuffer, nullptr, nullptr, 0, nullptr, 0);
	}

	return true;
}

void SceneVK::createProfiler()
{
	m_pProfiler = DBG_NEW ProfilerVK("Scene Update", m_pDevice);
	m_pProfiler->initTimestamp(&m_TimestampBuildAccelStruct, "Build top-level acceleration structure");
}

void SceneVK::cleanGarbage()
{
	SAFEDELETE(m_pGarbageScratchBuffer);
	SAFEDELETE(m_pGarbageInstanceBuffer);

	SAFEDELETE(m_pGarbageTransformsBufferGraphics);
	SAFEDELETE(m_pGarbageTransformsBufferCompute);

	if (m_OldTopLevelAccelerationStructure.Memory != VK_NULL_HANDLE)
	{
		vkFreeMemory(m_pDevice->getDevice(), m_OldTopLevelAccelerationStructure.Memory, nullptr);
		m_OldTopLevelAccelerationStructure.Memory = VK_NULL_HANDLE;
	}

	if (m_OldTopLevelAccelerationStructure.AccelerationStructure != VK_NULL_HANDLE)
	{
		m_pDevice->vkDestroyAccelerationStructureNV(m_pDevice->getDevice(), m_OldTopLevelAccelerationStructure.AccelerationStructure, nullptr);
		m_OldTopLevelAccelerationStructure.AccelerationStructure = VK_NULL_HANDLE;
	}

	m_OldTopLevelAccelerationStructure.Handle = VK_NULL_HANDLE;
}

void SceneVK::updateScratchBufferForBLAS()
{
	VkDeviceSize requiredSize = findMaxMemReqBLAS();

	if (m_pScratchBuffer->getSizeInBytes() < requiredSize)
	{
		m_pGarbageScratchBuffer = m_pScratchBuffer;

		BufferParams scratchBufferParams = {};
		scratchBufferParams.Usage = VK_BUFFER_USAGE_RAY_TRACING_BIT_NV;
		scratchBufferParams.MemoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		scratchBufferParams.SizeInBytes = requiredSize;

		m_pScratchBuffer = reinterpret_cast<BufferVK*>(m_pContext->createBuffer());
		m_pScratchBuffer->init(scratchBufferParams);
	}
}

void SceneVK::updateScratchBufferForTLAS()
{
	VkDeviceSize requiredSize = findMaxMemReqTLAS();

	if (m_pScratchBuffer->getSizeInBytes() < requiredSize)
	{
		m_pGarbageScratchBuffer = m_pScratchBuffer;

		BufferParams scratchBufferParams = {};
		scratchBufferParams.Usage = VK_BUFFER_USAGE_RAY_TRACING_BIT_NV;
		scratchBufferParams.MemoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		scratchBufferParams.SizeInBytes = requiredSize;

		m_pScratchBuffer = reinterpret_cast<BufferVK*>(m_pContext->createBuffer());
		m_pScratchBuffer->init(scratchBufferParams);
	}
}

void SceneVK::updateInstanceBuffer()
{
	if (m_pInstanceBuffer->getSizeInBytes() < sizeof(GeometryInstance) * m_GeometryInstances.size())
	{
		m_pGarbageInstanceBuffer = m_pInstanceBuffer;

		BufferParams instanceBufferParmas = {};
		instanceBufferParmas.Usage = VK_BUFFER_USAGE_RAY_TRACING_BIT_NV | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		instanceBufferParmas.MemoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		instanceBufferParmas.SizeInBytes = sizeof(GeometryInstance) * m_GeometryInstances.size();

		m_pInstanceBuffer = reinterpret_cast<BufferVK*>(m_pContext->createBuffer());
		m_pInstanceBuffer->init(instanceBufferParmas);
	}

	m_pTempCommandBuffer->reset(true);
	m_pTempCommandBuffer->begin(nullptr, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	m_pTempCommandBuffer->updateBuffer(m_pInstanceBuffer, 0, m_GeometryInstances.data(), m_GeometryInstances.size() * sizeof(GeometryInstance));

	m_pTempCommandBuffer->end();
	m_pDevice->executeCompute(m_pTempCommandBuffer, nullptr, nullptr, 0, nullptr, 0);

}

void SceneVK::updateTransformBuffer()
{
	const uint32_t sizeInBytes = sizeof(GraphicsObjectTransforms) * uint32_t(m_SceneTransforms.size());
	if (m_pTransformsBufferGraphics->getSizeInBytes() < sizeInBytes)
	{
		m_pGarbageTransformsBufferGraphics	= m_pTransformsBufferGraphics;
		m_pGarbageTransformsBufferCompute	= m_pTransformsBufferCompute;

		BufferParams transformBufferParams = {};
		transformBufferParams.Usage				= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		transformBufferParams.MemoryProperty	= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		transformBufferParams.SizeInBytes		= sizeInBytes;
		transformBufferParams.IsExclusive		= true;

		m_pTransformsBufferGraphics = reinterpret_cast<BufferVK*>(m_pContext->createBuffer());
		m_pTransformsBufferGraphics->init(transformBufferParams);

		m_pTransformsBufferCompute = reinterpret_cast<BufferVK*>(m_pContext->createBuffer());
		m_pTransformsBufferCompute->init(transformBufferParams);
	}

	m_TransformDataIsDirty = true;
}

bool SceneVK::createCombinedGraphicsObjectData()
{
	if (m_NewBottomLevelAccelerationStructures.size() > 0)
	{
		LOG("--- SceneVK: Can't create Combined Graphics Object Data if BLASs aren't finalized!");
		return false;
	}

	SAFEDELETE(m_pCombinedVertexBuffer);
	SAFEDELETE(m_pCombinedIndexBuffer);
	SAFEDELETE(m_pMeshIndexBuffer);

	m_pCombinedVertexBuffer = reinterpret_cast<BufferVK*>(m_pContext->createBuffer());
	m_pCombinedIndexBuffer	= reinterpret_cast<BufferVK*>(m_pContext->createBuffer());
	m_pMeshIndexBuffer		= reinterpret_cast<BufferVK*>(m_pContext->createBuffer());

	BufferParams vertexBufferParams = {};
	vertexBufferParams.Usage			= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	vertexBufferParams.SizeInBytes		= sizeof(Vertex) * m_TotalNumberOfVertices;
	vertexBufferParams.MemoryProperty	= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	vertexBufferParams.IsExclusive		= true;

	BufferParams indexBufferParams = {};
	indexBufferParams.Usage				= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	indexBufferParams.SizeInBytes		= sizeof(uint32_t) * m_TotalNumberOfIndices;
	indexBufferParams.MemoryProperty	= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	indexBufferParams.IsExclusive		= true;

	BufferParams meshIndexBufferParams = {};
	meshIndexBufferParams.Usage				= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	meshIndexBufferParams.SizeInBytes		= sizeof(uint32_t) * 3 * m_NumBottomLevelAccelerationStructures;
	meshIndexBufferParams.MemoryProperty	= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	meshIndexBufferParams.IsExclusive		= true;

	m_pCombinedVertexBuffer->init(vertexBufferParams);
	m_pCombinedIndexBuffer->init(indexBufferParams);
	m_pMeshIndexBuffer->init(meshIndexBufferParams);

	m_MeshDataIsDirty = true;

	return true;
}

VkDeviceSize SceneVK::findMaxMemReqBLAS()
{
	VkDeviceSize maxSize = 0;

	VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo = {};
	memoryRequirementsInfo.sType	= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
	memoryRequirementsInfo.type		= VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV;

	//Todo: Do we need to loop through finalized BLASs here as well?
	for (auto& bottomLevelAccelerationStructurePerMesh : m_NewBottomLevelAccelerationStructures)
	{
		for (auto& bottomLevelAccelerationStructure : bottomLevelAccelerationStructurePerMesh.second)
		{
			VkMemoryRequirements2 memReqBLAS = {};
			memoryRequirementsInfo.accelerationStructure = bottomLevelAccelerationStructure.second.AccelerationStructure;
			m_pDevice->vkGetAccelerationStructureMemoryRequirementsNV(m_pDevice->getDevice(), &memoryRequirementsInfo, &memReqBLAS);

			if (memReqBLAS.memoryRequirements.size > maxSize)
				maxSize = memReqBLAS.memoryRequirements.size;
		}
	}

	return maxSize;
}

VkDeviceSize SceneVK::findMaxMemReqTLAS()
{
	VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo = {};
	memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
	memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_UPDATE_SCRATCH_NV;
	memoryRequirementsInfo.accelerationStructure = m_TopLevelAccelerationStructure.AccelerationStructure;

	VkMemoryRequirements2 memReqTLAS = {};
	m_pDevice->vkGetAccelerationStructureMemoryRequirementsNV(m_pDevice->getDevice(), &memoryRequirementsInfo, &memReqTLAS);

	return memReqTLAS.memoryRequirements.size;
}

uint32_t SceneVK::registerMaterial(const Material* pMaterial)
{
	auto entry = m_MaterialIndices.find(pMaterial);
	if (entry == m_MaterialIndices.end())
	{
		m_Materials.push_back(pMaterial);
		m_MaterialIndices[pMaterial] = (uint32_t)m_Materials.size() - 1;
	}

	return m_MaterialIndices[pMaterial];
}

void SceneVK::renderUI()
{
	ImGui::SetNextWindowSize(ImVec2(430, 450), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Scene", NULL))
	{
		m_DebugParametersDirty = m_DebugParametersDirty || ImGui::SliderFloat("Roughness Scale", &m_SceneParameters.RoughnessScale, 0.01f, 10.0f);
		m_DebugParametersDirty = m_DebugParametersDirty || ImGui::SliderFloat("Metallic Scale", &m_SceneParameters.MetallicScale, 0.01f, 10.0f);
		m_DebugParametersDirty = m_DebugParametersDirty || ImGui::SliderFloat("Ambient Occlusion Scale", &m_SceneParameters.AOScale, 0.01f, 1.0f);
	}
	ImGui::End();

	if (m_LightSetup.hasDirectionalLight()) {
		ImGui::SetNextWindowSize(ImVec2(430, 100), ImGuiCond_FirstUseEver);

		if (ImGui::Begin("Directional Light", NULL, ImGuiWindowFlags_NoResize)) {
			DirectionalLight* pDirectionalLight = m_LightSetup.getDirectionalLight();

			float particleG = pDirectionalLight->getParticleG();
			float scatterAmount = pDirectionalLight->getScatterAmount();

			if (ImGui::SliderFloat("Particle G", &particleG, 0.0f, 1.0f)) {
				pDirectionalLight->setParticleG(particleG);
			}

			if (ImGui::SliderFloat("Scatter Amount", &scatterAmount, 0.0f, 1.0f)) {
				pDirectionalLight->setScatterAmount(scatterAmount);
			}
		}
		ImGui::End();
	}
}

void SceneVK::updateDebugParameters()
{
	if (m_DebugParametersDirty)
	{
		updateMaterials();
		m_DebugParametersDirty = false;
	}
}
