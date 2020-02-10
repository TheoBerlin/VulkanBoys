#pragma once
#include "Core/Core.h"

#include <glm/glm.hpp>

//REMEBER ALIGNMENT OF 16 bytes
struct Vertex
{
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoord;
};

class IMesh
{
public:
    DECL_INTERFACE(IMesh);
    
    virtual bool loadFromFile(const std::string& filepath) = 0;
    virtual bool loadFromMemory(const Vertex* pVertices, uint32_t vertexCount, const uint32_t* pIndices, uint32_t indexCount) = 0;
};
