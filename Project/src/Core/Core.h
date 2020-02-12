#pragma once
#include "Log.h"

#include <cstdint>
#include <iostream>
#include <cassert>
#include <cstdio>
#include <algorithm>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include "Common/Debug.h"

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
	
#define SAFEDELETE(object) if ((object)) { delete (object); (object) = nullptr; }

#define ASSERT(condition) assert(condition)

#define LOG(...) logPrintf(__VA_ARGS__); logPrintf("\n")

#ifdef _WIN32
	//TODO: Maybe should check for visual studio and not only windows since __forceinline is MSVC specific
	#define FORCEINLINE __forceinline
#else
	//TODO: Make sure this is actually a forceinline
	#define FORCEINLINE inline
#endif

#if _DEBUG
	#define D_LOG(...) LOG(__VA_ARGS__)
#else
	#define D_LOG(...)
#endif

#define MB(bytes) bytes * 1024 * 1024

//REMEBER ALIGNMENT OF 16 bytes
struct Vertex
{
	alignas(16) glm::vec3 Position;
	alignas(16) glm::vec3 Normal;
	alignas(16) glm::vec3 Tangent;
	glm::vec2 TexCoord;

	bool operator==(const Vertex& other) const 
	{
		return Position == other.Position && Normal == other.Normal && Tangent == other.Tangent && TexCoord == other.TexCoord;
	}
};

namespace std 
{
	template<> struct hash<Vertex> 
	{
		size_t operator()(Vertex const& vertex) const 
		{
			return ((hash<glm::vec3>()(vertex.Position) ^
				(hash<glm::vec3>()(vertex.Normal) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.TexCoord) << 1);
		}
	};
}