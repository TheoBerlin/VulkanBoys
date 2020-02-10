#pragma once
#include <cstdint>
#include <iostream>
#include <cassert>
#include <cstdio>
#include <algorithm>

#include <glm/glm.hpp>

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

#define LOG(...) printf(__VA_ARGS__); printf("\n")

#if _DEBUG
	#define D_LOG(...) LOG(__VA_ARGS__)
#else
	#define D_LOG(...) LOG(__VA_ARGS__)
#endif

#define MB(bytes) bytes * 1024 * 1024

//REMEBER ALIGNMENT OF 16 bytes
struct Vertex
{
	alignas(16) glm::vec3 Position;
	alignas(16) glm::vec3 Normal;
	alignas(16) glm::vec3 Tangent;
	glm::vec2 TexCoord;
};