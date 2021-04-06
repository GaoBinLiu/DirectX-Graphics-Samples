#include "Scene.h"

BModel::~BModel()
{
    Clear();
}
void BModel::Clear()
{
    Model::Clear();
    m_VertexExBuffer.Destroy();

    delete[] m_pBMesh;
    m_pBMesh = nullptr;
    m_BHeader.meshCount = 0;

    delete[] m_pNode;
    m_pNode = nullptr;
    m_BHeader.nodeCount = 0;

    delete[] m_pBMaterial;
    m_pBMaterial = nullptr;
}
bool BModel::Load(const char* filename)
{
    m_type = type_H3D;
    FILE* file = nullptr;
    if (0 != fopen_s(&file, filename, "rb"))
        return false;

    bool ok = false;

    if (1 != fread(&m_Header, sizeof(Header), 1, file)) goto h3d_load_fail;
    m_BHeader = *(BHeader*)&m_Header;
    m_pMesh = new Mesh[m_Header.meshCount];
    m_pMaterial = new Material[m_Header.materialCount];

    if (m_Header.meshCount > 0)
        if (1 != fread(m_pMesh, sizeof(Mesh) * m_Header.meshCount, 1, file)) goto h3d_load_fail;
    if (m_Header.materialCount > 0)
        if (1 != fread(m_pMaterial, sizeof(Material) * m_Header.materialCount, 1, file)) goto h3d_load_fail;

    m_VertexStride = m_pMesh[0].vertexStride;
    m_VertexStrideDepth = m_pMesh[0].vertexStrideDepth;
#if _DEBUG
    for (uint32_t meshIndex = 1; meshIndex < m_Header.meshCount; ++meshIndex)
    {
        const Mesh& mesh = m_pMesh[meshIndex];
        ASSERT(mesh.vertexStride == m_VertexStride);
        ASSERT(mesh.vertexStrideDepth == m_VertexStrideDepth);
    }
    for (uint32_t meshIndex = 0; meshIndex < m_Header.meshCount; ++meshIndex)
    {
        const Mesh& mesh = m_pMesh[meshIndex];

        ASSERT(mesh.attribsEnabled ==
            (attrib_mask_position | attrib_mask_texcoord0 | attrib_mask_normal | attrib_mask_tangent | attrib_mask_bitangent));
        ASSERT(mesh.attrib[0].components == 3 && mesh.attrib[0].format == Model::attrib_format_float); // position
        ASSERT(mesh.attrib[1].components == 2 && mesh.attrib[1].format == Model::attrib_format_float); // texcoord0
        ASSERT(mesh.attrib[2].components == 3 && mesh.attrib[2].format == Model::attrib_format_float); // normal
        ASSERT(mesh.attrib[3].components == 3 && mesh.attrib[3].format == Model::attrib_format_float); // tangent
        ASSERT(mesh.attrib[4].components == 3 && mesh.attrib[4].format == Model::attrib_format_float); // bitangent

        ASSERT(mesh.attribsEnabledDepth ==
            (attrib_mask_position));
        ASSERT(mesh.attrib[0].components == 3 && mesh.attrib[0].format == Model::attrib_format_float); // position
    }
#endif

    m_pVertexData = new unsigned char[m_Header.vertexDataByteSize];
    m_pIndexData = new unsigned char[m_Header.indexDataByteSize];
    m_pVertexDataDepth = new unsigned char[m_Header.vertexDataByteSizeDepth];
    m_pIndexDataDepth = new unsigned char[m_Header.indexDataByteSize];

    if (m_Header.vertexDataByteSize > 0)
        if (1 != fread(m_pVertexData, m_Header.vertexDataByteSize, 1, file)) goto h3d_load_fail;
    if (m_Header.indexDataByteSize > 0)
        if (1 != fread(m_pIndexData, m_Header.indexDataByteSize, 1, file)) goto h3d_load_fail;

    if (m_Header.vertexDataByteSizeDepth > 0)
        if (1 != fread(m_pVertexDataDepth, m_Header.vertexDataByteSizeDepth, 1, file)) goto h3d_load_fail;
    if (m_Header.indexDataByteSize > 0)
        if (1 != fread(m_pIndexDataDepth, m_Header.indexDataByteSize, 1, file)) goto h3d_load_fail;

    m_VertexBuffer.Create(L"VertexBuffer", m_Header.vertexDataByteSize / m_VertexStride, m_VertexStride, m_pVertexData);
    m_IndexBuffer.Create(L"IndexBuffer", m_Header.indexDataByteSize / sizeof(uint16_t), sizeof(uint16_t), m_pIndexData);

    initVertexExData();

    delete[] m_pVertexData;
    m_pVertexData = nullptr;
    delete[] m_pIndexData;
    m_pIndexData = nullptr;

    m_VertexBufferDepth.Create(L"VertexBufferDepth", m_Header.vertexDataByteSizeDepth / m_VertexStrideDepth, m_VertexStrideDepth, m_pVertexDataDepth);
    m_IndexBufferDepth.Create(L"IndexBufferDepth", m_Header.indexDataByteSize / sizeof(uint16_t), sizeof(uint16_t), m_pIndexDataDepth);
    delete[] m_pVertexDataDepth;
    m_pVertexDataDepth = nullptr;
    delete[] m_pIndexDataDepth;
    m_pIndexDataDepth = nullptr;

    LoadTextures();

    ok = true;

h3d_load_fail:

    if (EOF == fclose(file))
        ok = false;

    return ok;
}

bool BModel::LoadUnity(const char* filename)
{
    m_type = type_unity;
    FILE* file = nullptr;
    if (0 != fopen_s(&file, filename, "rb"))
        return false;

    bool ok = false;
    //load header
    if (1 != fread(&m_BHeader, sizeof(BHeader), 1, file)) goto h3d_load_fail;
    if (!checkAnchor(anchor_after_header, file)) goto h3d_load_fail;
    m_Header = *(Header*)&m_BHeader;
    //load submeshes
    m_pMesh = new Mesh[m_BHeader.subMeshCount];
    m_pMaterial = new Material[m_BHeader.materialCount];

    if (m_BHeader.subMeshCount > 0)
        if (1 != fread(m_pMesh, sizeof(Mesh) * m_BHeader.subMeshCount, 1, file)) goto h3d_load_fail;
    if (!checkAnchor(anchor_after_subMeshes, file)) goto h3d_load_fail;
    //if (m_Header.materialCount > 0)
    //    if (1 != fread(m_pMaterial, sizeof(Material) * m_Header.materialCount, 1, file)) goto h3d_load_fail;

    m_VertexStride = m_pMesh[0].vertexStride;
    m_VertexStrideDepth = m_pMesh[0].vertexStrideDepth;
#if _DEBUG
    for (uint32_t meshIndex = 1; meshIndex < m_BHeader.subMeshCount; ++meshIndex)
    {
        const Mesh& mesh = m_pMesh[meshIndex];
        ASSERT(mesh.vertexStride == m_VertexStride);
        ASSERT(mesh.vertexStrideDepth == m_VertexStrideDepth);
    }
    for (uint32_t meshIndex = 0; meshIndex < m_BHeader.subMeshCount; ++meshIndex)
    {
        const Mesh& mesh = m_pMesh[meshIndex];

        ASSERT(mesh.attribsEnabled ==
            (attrib_mask_position | attrib_mask_texcoord0 | attrib_mask_normal | attrib_mask_tangent | attrib_mask_bitangent));
        ASSERT(mesh.attrib[0].components == 3 && mesh.attrib[0].format == Model::attrib_format_float); // position
        ASSERT(mesh.attrib[1].components == 2 && mesh.attrib[1].format == Model::attrib_format_float); // texcoord0
        ASSERT(mesh.attrib[2].components == 3 && mesh.attrib[2].format == Model::attrib_format_float); // normal
        ASSERT(mesh.attrib[3].components == 3 && mesh.attrib[3].format == Model::attrib_format_float); // tangent
        ASSERT(mesh.attrib[4].components == 3 && mesh.attrib[4].format == Model::attrib_format_float); // bitangent

    }
#endif

    //load meshes
    m_pBMesh = new BMesh[m_BHeader.meshCount];
    if (m_BHeader.meshCount > 0)
        if (1 != fread(m_pBMesh, sizeof(BMesh) * m_BHeader.meshCount, 1, file)) goto h3d_load_fail;
    if (!checkAnchor(anchor_after_meshes, file)) goto h3d_load_fail;

    //load nodes
    m_pNode = new Node[m_BHeader.nodeCount];
    if (m_BHeader.nodeCount > 0)
        if (1 != fread(m_pNode, sizeof(Node) * m_BHeader.nodeCount, 1, file)) goto h3d_load_fail;
    if (!checkAnchor(anchor_after_nodes, file)) goto h3d_load_fail;

    m_pVertexData = new unsigned char[m_BHeader.vertexDataByteSize];
    m_pIndexData = new unsigned char[m_BHeader.indexDataByteSize];
    

    if (m_BHeader.vertexDataByteSize > 0)
        if (1 != fread(m_pVertexData, m_BHeader.vertexDataByteSize, 1, file)) goto h3d_load_fail;
    if (!checkAnchor(anchor_after_vertexBuffer, file)) goto h3d_load_fail;
    if (m_BHeader.indexDataByteSize > 0)
        if (1 != fread(m_pIndexData, m_BHeader.indexDataByteSize, 1, file)) goto h3d_load_fail;
    if (!checkAnchor(anchor_after_indexBuffer, file)) goto h3d_load_fail;

    m_VertexBuffer.Create(L"VertexBuffer", m_BHeader.vertexDataByteSize / m_VertexStride, m_VertexStride, m_pVertexData);
    m_IndexBuffer.Create(L"IndexBuffer", m_BHeader.indexDataByteSize / sizeof(uint32_t), sizeof(uint32_t), m_pIndexData);

    initVertexExData();

    delete[] m_pVertexData;
    m_pVertexData = nullptr;
    delete[] m_pIndexData;
    m_pIndexData = nullptr;

    //LoadTextures();

    ok = true;

h3d_load_fail:

    if (EOF == fclose(file))
        ok = false;

    return ok;
}

bool BModel::checkAnchor(uint32_t anchor, FILE* file)
{
    uint32_t buf;
    if (1 != fread(&buf, sizeof(uint32_t), 1, file))
        return false;
#if _DEBUG
    ASSERT(buf == anchor);
#endif
    return buf == anchor;

}

void BModel::initVertexExData()
{
    struct vertex
    {
        float position[3];
        float uv[2];
        float normal[3];
        float tangent[3];
        float bitangent[3];
    };
    struct vertexEx
    {
        float uv1[2];
    };
    uint32_t elementCount = m_BHeader.vertexDataByteSize / m_VertexStride;
    uint32_t elementSize = sizeof(vertex);
#if _DEBUG
    ASSERT(elementSize == m_VertexStride);
#endif
    uint32_t exElementSize = sizeof(vertexEx);
    m_pVertexExData = new unsigned char[elementCount * exElementSize];
    vertexEx* vertEx = (vertexEx*)m_pVertexExData;
    vertex* vert = (vertex*)m_pVertexData;
    for (int i = 0; i < elementCount; i++)
    {
        vertEx->uv1[0] = vert->uv[0];
        vertEx->uv1[1] = vert->uv[1];
        vertEx++;
        vert++;
    }

    m_pMeshEx = new MeshEx[m_BHeader.subMeshCount];
    float uOffset = 0;
    float vOffset = 0;
    float width = 0.1f;
    for (uint32_t meshIndex = 1; meshIndex < m_BHeader.subMeshCount; ++meshIndex)
    {
        const Mesh& mesh = m_pMesh[meshIndex];
        MeshEx& meshEx = m_pMeshEx[meshIndex];
        meshEx.vertexStride = exElementSize;
        meshEx.vertexDataByteOffset = (mesh.vertexDataByteOffset / elementSize) * exElementSize;
        meshEx.attrib[0].components = 2;
        meshEx.attrib[0].format = Model::attrib_format_float;
        meshEx.attrib[0].normalized = 0;
        meshEx.attrib[0].offset = 0;

        vertexEx* vertEx = (vertexEx*)(m_pVertexExData + meshEx.vertexDataByteOffset);

        for (uint32_t i = 0; i < mesh.vertexCount; i++)
        {
            vertEx->uv1[0] = vertEx->uv1[0] * width + uOffset;
            vertEx->uv1[1] = vertEx->uv1[1] * width + vOffset;
            vertEx++;
        }
        uOffset += width;
        if (uOffset + width >= 1.0f)
        {
            uOffset = 0;
            vOffset += width;
        }
    }

    m_VertexExBuffer.Create(L"VertexExBuffer", elementCount, exElementSize, m_pVertexExData);
    delete[] m_pVertexExData;
    m_pVertexExData = nullptr;
}

bool EditorModel::Load()
{
    struct vertex
    {
        float position[3];
        float color[4];
    };
    m_pVertexData = new unsigned char[5000];
    m_pIndexData = new unsigned char[2000];
    m_pMesh = new Mesh[2];
    //grid
    vertex *v = (vertex*)m_pVertexData;
    uint32_t* id = (uint32_t*)m_pIndexData;
    float x = -5.0f;
    float z = -5.0f;
    for (int i = 0; i < 11; i++)
    {
        v->position[0] = x;
        v->position[1] = 0.0f;
        v->position[2] = z;
        v->color[0] = 0.5;
        v->color[1] = 0.5;
        v->color[2] = 0.5;
        v->color[3] = 1;
        v++;
        v->position[0] = x;
        v->position[1] = 0.0f;
        v->position[2] = -z;
        v->color[0] = 0.5;
        v->color[1] = 0.5;
        v->color[2] = 0.5;
        v->color[3] = 1;
        v++;
        x += 1.0f;
    }
    x = -5.0f;
    z = -5.0f;
    for (int i = 0; i < 11; i++)
    {
        v->position[0] = x;
        v->position[1] = 0.0f;
        v->position[2] = z;
        v->color[0] = 0.5;
        v->color[1] = 0.5;
        v->color[2] = 0.5;
        v->color[3] = 1;
        v++;
        v->position[0] = -x;
        v->position[1] = 0.0f;
        v->position[2] = z;
        v->color[0] = 0.5;
        v->color[1] = 0.5;
        v->color[2] = 0.5;
        v->color[3] = 1;
        v++;
        z += 1.0f;
    }
    Mesh& mesh = m_pMesh[0];
    mesh.attrib[0].format = attrib_format_float;
    mesh.attrib[0].components = 3;
    mesh.attrib[0].normalized = 0;
    mesh.attrib[0].offset = 0;
    mesh.attrib[1].format = attrib_format_float;
    mesh.attrib[1].components = 4;
    mesh.attrib[1].normalized = 1;
    mesh.attrib[1].offset = sizeof(float) * 3;
    mesh.attribsEnabled = attrib_position | attrib_1;
    mesh.vertexStride = sizeof(vertex);
    mesh.vertexDataByteOffset = 0;
    mesh.vertexCount = v - (vertex*)m_pVertexData;
    mesh.indexCount = 0;
    mesh.indexDataByteOffset = 0;
    mesh.materialIndex = 0;
    mesh.boundingBox.min = Math::Vector3(-10.0f, 0, -10.0f);
    mesh.boundingBox.max = Math::Vector3(10.0f, 0.0f, 10.0f);

    //arrow x
    auto vstart = v;
    v->position[0] = 0;
    v->position[1] = 0;
    v->position[2] = 0;
    v->color[0] = 1;
    v->color[1] = 0;
    v->color[2] = 0;
    v->color[3] = 1;
    v++;
    v->position[0] = 0.2f;
    v->position[1] = 0;
    v->position[2] = 0;
    v->color[0] = 1;
    v->color[1] = 0;
    v->color[2] = 0;
    v->color[3] = 0;
    v++;
    //y
    v->position[0] = 0;
    v->position[1] = 0;
    v->position[2] = 0;
    v->color[0] = 0;
    v->color[1] = 1;
    v->color[2] = 0;
    v->color[3] = 1;
    v++;
    v->position[0] = 0;
    v->position[1] = 0.2f;
    v->position[2] = 0;
    v->color[0] = 0;
    v->color[1] = 1;
    v->color[2] = 0;
    v->color[3] = 0;
    v++;
    //z
    v->position[0] = 0;
    v->position[1] = 0;
    v->position[2] = 0;
    v->color[0] = 0.1f;
    v->color[1] = 0.1f;
    v->color[2] = 1;
    v->color[3] = 1;
    v++;
    v->position[0] = 0;
    v->position[1] = 0;
    v->position[2] = 0.2f;
    v->color[0] = 0.1f;
    v->color[1] = 0.1f;
    v->color[2] = 1;
    v->color[3] = 0;
    v++;

    Mesh& mesh1 = m_pMesh[1];
    mesh1 = m_pMesh[0];
    mesh1.vertexDataByteOffset = (byte*)vstart - m_pVertexData;
    mesh1.vertexCount = v - (vertex*)vstart;
    mesh1.indexCount = 0;
    mesh1.indexDataByteOffset = 0;
    mesh1.materialIndex = 0;
    mesh1.boundingBox.min = Math::Vector3(0.0f, 0.0f, 0.0f);
    mesh1.boundingBox.max = Math::Vector3(1.0f, 1.0f, 1.0f);

    m_VertexBuffer.Create(L"VertexBuffer", v - (vertex*)m_pVertexData, sizeof(vertex), m_pVertexData);
    m_IndexBuffer.Create(L"IndexBuffer", 1, sizeof(uint32_t), m_pIndexData);

    delete[] m_pVertexData;
    m_pVertexData = nullptr;
    delete[] m_pIndexData;
    m_pIndexData = nullptr;
    return false;
}