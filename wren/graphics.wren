foreign class TextureFormat {
    construct new(i) {}
    
    foreign static unknown
    foreign static rgba8_unorm
}

foreign class GeomTopology {
    construct new(i) {}
    
    foreign static undefined
    foreign static points
    foreign static lines
    foreign static triangles
}

foreign class FaceCull {
    construct new(i) {}
    
    foreign static none
    foreign static cw
    foreign static ccw
}

foreign class ShaderType {
    construct new(i) {}
    
    foreign static none
    foreign static vertex
    foreign static pixel
    foreign static geometry
    foreign static compute
}

foreign class ClearFlags {
    construct new(i) {}
    
    foreign static none
    foreign static depth
    foreign static stencil
}

foreign class ValueType {
    construct new(i) {}
    
    foreign static undefined
    foreign static int8
    foreign static int16
    foreign static int32
    foreign static uint8
    foreign static uint16
    foreign static uint32
    foreign static float16
    foreign static float32
}

foreign class UsageFlags {
    construct new(i) {}
    
    foreign static immutable
    foreign static default
    foreign static dynamic
}

foreign class BindFlags {
    construct new(i) {}
    
    foreign static vertex
    foreign static index
    foreign static uniform
}

foreign class CpuAccessFlags {
    construct new(i) {}
    
    foreign static none
    foreign static read
    foreign static write
}

class Graphics {
    foreign static getColorBufferFormat()
    foreign static getDepthBufferFormat()
    
    /*foreign static createCubemap(cubemap)
    foreign static createMesh(mesh)

    foreign static batchBegin()
    foreign static batchInstance(model, material)
    foreign static batchDraw(modelId, count)
    foreign static batchEnd()

    foreign static drawBegin(shaderId)
    foreign static drawSetFloat(loc, v)
    foreign static drawSetVec2(loc, v)
    foreign static drawSetVec3(loc, v)
    foreign static drawSetVec4(loc, v)
    foreign static drawSetMat4(loc, v)
    foreign static drawBindTexture(loc, i, id)
    foreign static drawBindCubemap(loc, i, id)
    foreign static drawEnd(primitive)

    foreign static convoluteEnvMap(shaderId, cubemapId, resolution)
    foreign static prefilterEnvMap(shaderId, cubemapId, resolution)
    foreign static genBrdfLut(shaderId, resolution)

    foreign static drawScreen(shaderId, textureId)
    foreign static drawSkybox(shaderId, cubemapId, view, proj)*/

    foreign static debugLine(a, b, color, lifespan)
}

foreign class Shader {
    construct new() { }

    foreign source
}

foreign class Texture {
    construct new() { }
}

foreign class Material {
    construct new() { }
}

foreign class Mesh {
    construct new() { }
}