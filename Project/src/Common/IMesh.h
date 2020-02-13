#pragma once
#include "Core/Core.h"

#include <glm/glm.hpp>

class IBuffer;

class IMesh
{
public:
    DECL_INTERFACE(IMesh);
    
    virtual bool initFromFile(const std::string& filepath) = 0;
    virtual bool initFromMemory(const Vertex* pVertices, uint32_t vertexCount, const uint32_t* pIndices, uint32_t indexCount) = 0;

    virtual IBuffer* getVertexBuffer() const = 0;
    virtual IBuffer* getIndexBuffer() const = 0;

    virtual uint32_t getVertexCount() const = 0;
    virtual uint32_t getIndexCount() const = 0;

    virtual uint32_t getMeshID() const = 0;
};
