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
#include <glm/gtx/string_cast.hpp>

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

struct Vertex
{
	glm::vec4 Position;
	glm::vec4 Normal;
	glm::vec4 Tangent;
	glm::vec4 TexCoord;

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
			return ((hash<glm::vec4>()(vertex.Position) ^
				(hash<glm::vec4>()(vertex.Normal) << 1)) >> 1) ^
				(hash<glm::vec4>()(vertex.TexCoord) << 1);
		}
	};
}