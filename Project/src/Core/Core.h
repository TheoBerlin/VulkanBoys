#pragma once

// Disable warnings in visual studio
#ifdef _MSC_VER
	#pragma warning(disable : 4201) // Non standard struct/union
	#pragma warning(disable : 4324) // Padding
#endif

#include "Log.h"

#include <cstdint>
#include <iostream>
#include <cassert>
#include <cstdio>
#include <algorithm>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/string_cast.hpp>
#include "glm/gtx/vector_angle.hpp"

#include "Common/Debug.h"

// Common defines for creating classes
#define DECL_NO_COPY(Type) \
	Type(Type&&) = delete; \
	Type(const Type&) = delete; \
	Type& operator=(Type&&) = delete; \
	Type& operator=(const Type&) = delete

#define DECL_INTERFACE(Type) \
		Type() = default; \
		virtual ~Type() = default; \
		DECL_NO_COPY(Type)

#define DECL_STATIC_CLASS(Type) \
		Type()	= delete; \
		~Type() = delete; \
		DECL_NO_COPY(Type)

// Resources
#define SAFEDELETE(object) if ((object)) { delete (object); (object) = nullptr; }

// Assert
#define ASSERT(condition) assert(condition)

// Log defines
#define LOG(...) logPrintLine(__VA_ARGS__)

#if _DEBUG
	#define D_LOG(...) LOG(__VA_ARGS__)
#else
	#define D_LOG(...)
#endif

// Remove the unused parameter warning temporarily
#define UNREFERENCED_PARAMETER(param) (param)

// Compiler macros
#ifdef _MSC_VER
	#define FORCEINLINE __forceinline
#else
	//TODO: Make sure this is actually a forceinline
	#define FORCEINLINE inline
#endif

// Size macros
#define MB(bytes) bytes * 1024 * 1024

//Define Common structures
struct Vertex
{
	alignas(16) glm::vec3 Position;
	alignas(16) glm::vec3 Normal;
	alignas(16) glm::vec3 Tangent;
	alignas(16) glm::vec2 TexCoord;

	bool operator==(const Vertex& other) const
	{
		return Position == other.Position && Normal == other.Normal && Tangent == other.Tangent && TexCoord == other.TexCoord;
	}

	void calculateTangent(const Vertex& v1, const Vertex& v2)
	{
		glm::vec3 edge1 = v1.Position - this->Position;
		glm::vec3 edge2 = v2.Position - this->Position;
		glm::vec2 deltaUV1 = v1.TexCoord - this->TexCoord;
		glm::vec2 deltaUV2 = v2.TexCoord - this->TexCoord;

		float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

		glm::vec3 tangent;
		tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
		tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
		tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
		tangent = glm::normalize(tangent);

		this->Tangent = tangent;
	}
};

namespace std
{
	template<> struct hash<Vertex>
	{
		size_t operator()(Vertex const& vertex) const
		{
			return
				((hash<glm::vec3>()(vertex.Position) ^
				(hash<glm::vec3>()(vertex.Normal) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.TexCoord) << 1);
		}
	};
}

enum class ETextureFormat : uint8_t
{
	FORMAT_NONE					= 0,
	FORMAT_R8G8B8A8_UNORM		= 1,
	FORMAT_R16G16_FLOAT			= 2,
	FORMAT_R16G16B16A16_FLOAT	= 3,
	FORMAT_R32G32B32A32_FLOAT	= 4
};

inline uint32_t textureFormatStride(ETextureFormat format)
{
	switch (format)
	{
	case ETextureFormat::FORMAT_R8G8B8A8_UNORM:
	case ETextureFormat::FORMAT_R16G16_FLOAT:		return 4;
	case ETextureFormat::FORMAT_R16G16B16A16_FLOAT: return 8;
	case ETextureFormat::FORMAT_R32G32B32A32_FLOAT: return 16;
	}

	return 0;
}