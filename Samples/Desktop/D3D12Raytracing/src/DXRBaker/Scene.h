#pragma once
#include "stdafx.h"
#include "ShaderResource.h"
#include "Model.h"

class BModel : public Model
{
public:
    enum
    {
        type_H3D,
        type_unity
    };
    enum
    {
        anchor_after_header = 100,
        anchor_after_subMeshes = 200,
        anchor_after_meshes = 300,
        anchor_after_nodes= 400,
        anchor_after_vertexBuffer = 500,
        anchor_after_indexBuffer = 600
    };
    struct BHeader
    {
        uint32_t subMeshCount;
        uint32_t materialCount;
        uint32_t vertexDataByteSize;
        uint32_t indexDataByteSize;
        uint32_t vertexDataByteSizeDepth;
        uint32_t meshCount;
        uint32_t nodeCount;
        BoundingBox boundingBox;
    };
    BHeader m_BHeader;

    struct BMesh
    {
        uint32_t subMeshOffset;
        uint32_t subMeshCount;
    };
    BMesh* m_pBMesh;

    struct Node
    {
        Vector3 position;
        Vector3 scale;
        Quaternion rotation;
        uint32_t meshID;
    };
    Node* m_pNode;

    struct BMaterial
    {
        Vector3 diffuse;
        //Vector3 specular;
        //Vector3 ambient;
        //Vector3 emissive;
        //Vector3 transparent; // light passing through a transparent surface is multiplied by this filter color
        //float opacity;
        //float shininess; // specular exponent
        //float specularStrength; // multiplier on top of specular color

        //enum {maxTexPath = 128};
        //enum {texCount = 6};
        //char texDiffusePath[maxTexPath];
        //char texSpecularPath[maxTexPath];
        //char texEmissivePath[maxTexPath];
        //char texNormalPath[maxTexPath];
        //char texLightmapPath[maxTexPath];
        //char texReflectionPath[maxTexPath];

        //enum {maxMaterialName = 128};
        //char name[maxMaterialName];
    };
    BMaterial *m_pBMaterial; 

    struct MeshEx
    {
        unsigned int vertexStride;
        unsigned int vertexDataByteOffset;
        Attrib attrib[1];
    };
    MeshEx* m_pMeshEx;
    ~BModel();
    void Clear() override;
    bool Load(const char* filename) override;
    bool LoadUnity(const char* filename);

    StructuredBuffer m_VertexExBuffer;
    uint32_t m_VertexExStride;
    uint32_t m_type = type_H3D;
protected:

    void initVertexExData();
    bool checkAnchor(uint32_t anchor, FILE* file);
    unsigned char* m_pVertexExData;
};

class EditorModel : public Model
{
public:
    bool Load();
    
};
