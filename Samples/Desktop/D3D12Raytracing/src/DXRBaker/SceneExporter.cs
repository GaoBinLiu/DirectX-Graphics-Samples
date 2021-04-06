using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEditor;
using System.IO;
using System;
using System.Runtime.InteropServices;
using System.Linq;

public class SceneExporter : EditorWindow
{
    class Styles
    {
        //	public GUIContent m_WarningContent = new GUIContent(string.Empty, EditorGUIUtility.LoadRequired("Builtin Skins/Icons/console.warnicon.sml.png") as Texture2D);
        public GUIStyle mPreviewBox = new GUIStyle("OL Box");
        public GUIStyle mPreviewTitle = new GUIStyle("OL Title");
        public GUIStyle mPreviewTitle1 = new GUIStyle("OL Box");
        public GUIStyle mLoweredBox = new GUIStyle("TextField");
        public GUIStyle mHelpBox = new GUIStyle("helpbox");
        public GUIStyle mMiniLable = new GUIStyle("MiniLabel");
        public GUIStyle mSelected = new GUIStyle("LODSliderRangeSelected");
        public GUIStyle mOLTitle = new GUIStyle("OL Title");
        public GUIStyle mHLine = new GUIStyle();
        public GUIStyle mVLine = new GUIStyle();
        public Styles()
        {
            mLoweredBox.padding = new RectOffset(1, 1, 1, 1);
            mPreviewTitle1.fixedHeight = 0;
            mPreviewTitle1.fontStyle = FontStyle.Bold;
            mPreviewTitle1.alignment = TextAnchor.MiddleLeft;

            mHLine.fixedHeight = 1f;
            mHLine.margin = new RectOffset(0, 0, 0, 0);
            mVLine.fixedWidth = 1f;
            mVLine.stretchHeight = true;
            mVLine.stretchWidth = false;
        }
    }
    private static Styles mStyles;

    #region structs
    enum ANCHOR
    {
        AfterHeader = 100,
        AfterSubMeshes = 200,
        AfterMeshes = 300,
        AfterNodes = 400,
        AfterVertexBuffer = 500,
        AfterIndexBuffer = 600
    }

    enum ATTRIB_MASK
    {
        attrib_mask_0 = (1 << 0),
        attrib_mask_1 = (1 << 1),
        attrib_mask_2 = (1 << 2),
        attrib_mask_3 = (1 << 3),
        attrib_mask_4 = (1 << 4),
        attrib_mask_5 = (1 << 5),
        attrib_mask_6 = (1 << 6),
        attrib_mask_7 = (1 << 7),
        attrib_mask_8 = (1 << 8),
        attrib_mask_9 = (1 << 9),
        attrib_mask_10 = (1 << 10),
        attrib_mask_11 = (1 << 11),
        attrib_mask_12 = (1 << 12),
        attrib_mask_13 = (1 << 13),
        attrib_mask_14 = (1 << 14),
        attrib_mask_15 = (1 << 15),

        // friendly name aliases
        attrib_mask_position = attrib_mask_0,
        attrib_mask_texcoord0 = attrib_mask_1,
        attrib_mask_normal = attrib_mask_2,
        attrib_mask_tangent = attrib_mask_3,
        attrib_mask_bitangent = attrib_mask_4,
    };

    enum ATTRIB
    {
        attrib_0 = 0,
        attrib_1 = 1,
        attrib_2 = 2,
        attrib_3 = 3,
        attrib_4 = 4,
        attrib_5 = 5,
        attrib_6 = 6,
        attrib_7 = 7,
        attrib_8 = 8,
        attrib_9 = 9,
        attrib_10 = 10,
        attrib_11 = 11,
        attrib_12 = 12,
        attrib_13 = 13,
        attrib_14 = 14,
        attrib_15 = 15,

        // friendly name aliases
        attrib_position = attrib_0,
        attrib_texcoord0 = attrib_1,
        attrib_normal = attrib_2,
        attrib_tangent = attrib_3,
        attrib_bitangent = attrib_4,

        maxAttribs = 16
    };

    enum ATTRIB_FORMAT
    {
        attrib_format_none = 0,
        attrib_format_ubyte,
        attrib_format_byte,
        attrib_format_ushort,
        attrib_format_short,
        attrib_format_float,

        attrib_formats
    };

    struct Vertex
    {
        public Vector3 pos;
        public Vector2 uv;
        public Vector3 normal;
        public Vector3 tangent;
        public Vector3 binormal;
    }

    struct VertexEx
    {
        public Vector2 uv2;
    }

    struct BoundingBox
    {
        public Vector4 min;
        public Vector4 max;
    }

    struct Header
    {
        public UInt32 subMeshCount;
        public UInt32 materialCount;
        public UInt32 vertexDataByteSize;
        public UInt32 indexDataByteSize;
        public UInt32 vertexDataByteSizeDepth;
        public UInt32 meshCount;
        public UInt32 nodeCount;
        UInt32 padding2;
        public BoundingBox boundingBox;
    }

    struct Attrib
    {
        public UInt16 offset; // byte offset from the start of the vertex
        public UInt16 normalized; // if true, integer formats are interpreted as [-1, 1] or [0, 1]
        public UInt16 components; // 1-4
        public UInt16 format;
    }

    struct BSubMesh
    {
        public BoundingBox boundingBox;

        public UInt32 materialIndex;

        public UInt32 attribsEnabled;
        public UInt32 attribsEnabledDepth;
        public UInt32 vertexStride;
        public UInt32 vertexStrideDepth;
        public Attrib[] attrib;
        public Attrib[] attribDepth;

        public UInt32 vertexDataByteOffset;
        public UInt32 vertexCount;
        public UInt32 indexDataByteOffset;
        public UInt32 indexCount;

        public UInt32 vertexDataByteOffsetDepth;
        public UInt32 vertexCountDepth;
    }

    struct BMesh
    {
        public UInt32 subMeshOffset;
        public UInt32 subMeshCount;
    }

    struct Node
    {
        public Vector4 position;
        public Vector4 scale;
        public Quaternion rotation;
        public UInt32 meshID;
        UInt32 padding0;
        UInt32 padding1;
        UInt32 padding2;
    }

    struct Model
    {
        public Vertex[] vertexBuffer;
        public VertexEx[] vertexExBuffer;
        public Int32[] indexBuffer;

        public BSubMesh[] subMeshes;
        public BMesh[] meshes;

        public Dictionary<Mesh, UInt32> meshIdMap;
    }

    struct Scene
    {
        public Header header;
        public Model model;
        public Node[] nodes;
    }
    #endregion
    public static SceneExporter Window = null;
    private string mSaveMeshPath = "";
    private string mNewMeshName = "";
    private Scene mScene = new Scene();

    [MenuItem("GEffect/SceneExporter")]
    static void AddWindow()
    {
        //if (Window != null)
        //    Window.Close();
        Window = EditorWindow.GetWindow<SceneExporter>(false, "Scene Exporter");
        Window.minSize = new Vector2(350, 200);
        Window.Show();
    }

    public void OnGUI()
    {
        if (GUILayout.Button("Export selections"))
        {
            if (!SelectionsToScene(ref mScene))
                return;
            string path = EditorUtility.SaveFilePanel("save mesh file", mSaveMeshPath, mNewMeshName, "bscene");
            if (!string.IsNullOrEmpty(path))
            {
                int id = path.LastIndexOf("/");
                mNewMeshName = path.Substring(id + 1);
                mSaveMeshPath = path.Substring(0, id + 1);
                StreamWriter writer = new StreamWriter(path);
                var stream = writer.BaseStream;
                unsafe
                {
                    //header
                    var arr = new byte[sizeof(Header)];
                    HeaderToBytes(ref mScene.header, ref arr);
                    stream.Write(arr, 0, arr.Length);
                    //Anchor AfterHeader
                    WriteAnchor(ANCHOR.AfterHeader, ref stream);

                    //subMeshes
                    for(int i = 0;i<mScene.model.subMeshes.Length;i++)
                    {
                        WriteBSubMesh(ref mScene.model.subMeshes[i], ref stream);
                    }
                    //Anchor AfterSubMeshes
                    WriteAnchor(ANCHOR.AfterSubMeshes, ref stream);

                    //meshes
                    arr = new byte[sizeof(BMesh) * mScene.model.meshes.Length];
                    var len = BMeshToBytes(ref mScene.model.meshes, ref arr);
                    stream.Write(arr, 0, len);
                    //Anchor AfterMeshes
                    WriteAnchor(ANCHOR.AfterMeshes, ref stream);

                    //nodes
                    arr = new byte[sizeof(Node) * mScene.nodes.Length];
                    len = NodeToBytes(ref mScene.nodes, ref arr);
                    stream.Write(arr, 0, len);
                    //Anchor AfterNodes
                    WriteAnchor(ANCHOR.AfterNodes, ref stream);

                    //vertex buffer
                    var arrlen = Math.Max(mScene.header.vertexDataByteSize, mScene.header.indexDataByteSize);
                    arr = new byte[arrlen];
                    len = VertexToBytes(ref mScene.model.vertexBuffer, ref arr);
                    stream.Write(arr, 0, len);
                    //Anchor AfterVertexBuffer
                    WriteAnchor(ANCHOR.AfterVertexBuffer, ref stream);

                    //index buffer
                    len = Int32ToBytes(ref mScene.model.indexBuffer, ref arr);
                    stream.Write(arr, 0, len);
                    //Anchor AfterIndexBuffer
                    WriteAnchor(ANCHOR.AfterIndexBuffer, ref stream);

                }
                writer.Flush();
                writer.Close();
            }
        }
    }

    bool SelectionsToScene(ref Scene scene)
    {
        var objs = Selection.gameObjects;
        List<MeshFilter> meshFilters = new List<MeshFilter>();
        if(objs!=null)
        {
            foreach(var obj in objs)
            {
                var filters = obj?.GetComponentsInChildren<MeshFilter>();
                foreach(var filter in filters)
                {
                    if(filter!=null && filter.sharedMesh!=null && filter.gameObject.activeInHierarchy && !meshFilters.Contains(filter))
                    {
                        meshFilters.Add(filter);
                    }
                }
            }
        } 
        if(meshFilters.Count>0)
        {
            List<Mesh> meshList = new List<Mesh>();
            foreach(var fileter in meshFilters)
            {
                var mesh = fileter.sharedMesh;
                if (mesh != null && !meshList.Contains(mesh))
                {
                    meshList.Add(mesh);
                }
            }

            ToModel(ref scene, meshList);
            List<Node> nodes = new List<Node>();
            foreach (var fileter in meshFilters)
            {
                var mesh = fileter.sharedMesh;
                if (mesh != null)
                {
                    var trans = fileter.transform;
                    Node node = new Node();
                    node.meshID = scene.model.meshIdMap[mesh];
                    node.position = trans.position;
                    node.scale = trans.lossyScale;
                    node.rotation = trans.rotation;
                    nodes.Add(node);
                }
            }
            scene.nodes = nodes.ToArray();
            scene.header.nodeCount = (uint)scene.nodes.Length;
            return true;
        }
        return false;
    }

    private void ToModel(ref Scene scene, List<Mesh> meshes)
    {
        if (meshes.Count == 0)
            return;
        scene.model.meshIdMap = new Dictionary<Mesh, UInt32>();
        uint sizeOfVertex = 0;
        uint sizeOfUInt32 = 0;
        uint sizeOfFloat = 0;
        unsafe
        {
            sizeOfVertex = (uint)sizeof(Vertex);
            sizeOfUInt32 = (uint)sizeof(UInt32);
            sizeOfFloat = (uint)sizeof(float);
        }

        int vn = 0;
        int sn = 0;
        int mn = 0;
        uint idn = 0;
        Bounds bounds = meshes[0].bounds;
        foreach(var mesh in meshes)
        {
            if (mesh == null)
                continue;
            mn++;
            vn += mesh.vertexCount;
            sn += mesh.subMeshCount;

            for(int i = 0;i<mesh.subMeshCount;i++)
            {
                idn += mesh.GetIndexCount(i);
            }
            bounds.Encapsulate(mesh.bounds);
        }

        scene.model.vertexBuffer = new Vertex[vn];
        scene.model.vertexExBuffer = new VertexEx[vn];
        scene.model.indexBuffer = new Int32[idn];
        scene.model.subMeshes = new BSubMesh[sn];
        scene.model.meshes = new BMesh[mn];

        int voffset = 0;
        int ioffset = 0;
        int soffset = 0;
        int moffset = 0;
        int id = 0;
        foreach (var mesh in meshes)
        {
            scene.model.meshIdMap[mesh] = (UInt32)moffset;
            int mvn = mesh.vertexCount;
            //fill vertex buffer
            var vertices = mesh.vertices;
            var uv = mesh.uv;
            var normals = mesh.normals;
            var tangents = mesh.tangents;
            var uv2 = mesh.uv2;
            for (int i = 0; i < mvn; i++)
            {
                id = i + voffset;
                scene.model.vertexBuffer[id].pos = vertices[i];
                if (uv.Length == mvn)
                    scene.model.vertexBuffer[id].uv = uv[i];
                if (normals.Length == mvn)
                    scene.model.vertexBuffer[id].normal = normals[i];
                if(tangents.Length == mvn)
                    scene.model.vertexBuffer[id].tangent = tangents[i];

                if (uv2.Length == mvn)
                {
                    scene.model.vertexExBuffer[id].uv2 = uv2[i];
                }
            }
            scene.model.meshes[moffset].subMeshCount = (uint)mesh.subMeshCount;
            scene.model.meshes[moffset].subMeshOffset = (uint)soffset;
            //submesh
            for (int i = 0; i < mesh.subMeshCount; i++)
            {
                //fill index buffer
                var ids = mesh.GetIndices(i);
                for (int j = 0;j<ids.Length;j++)
                {
                    id = j + ioffset;
                    scene.model.indexBuffer[id] = ids[j];
                }

                //fill BMesh info(BMesh <---> SubMesh)
                BSubMesh bmesh = new BSubMesh();
                bmesh.boundingBox.min = mesh.bounds.min;
                bmesh.boundingBox.max = mesh.bounds.max;
                bmesh.materialIndex = (uint)soffset;
                bmesh.attribsEnabled = (uint)(
                    ATTRIB_MASK.attrib_mask_position |
                    ATTRIB_MASK.attrib_mask_texcoord0 |
                    ATTRIB_MASK.attrib_mask_normal | 
                    ATTRIB_MASK.attrib_mask_tangent|
                    ATTRIB_MASK.attrib_mask_bitangent);
                bmesh.attribsEnabledDepth = 0;
                bmesh.vertexStride = sizeOfVertex;
                bmesh.vertexStrideDepth = 0;
                bmesh.attrib = new Attrib[(int)ATTRIB.maxAttribs];
                {
                    ushort offset = 0;
                    //pos
                    bmesh.attrib[0].offset = offset;
                    bmesh.attrib[0].normalized = 0;
                    bmesh.attrib[0].components = 3;
                    bmesh.attrib[0].format = (ushort)ATTRIB_FORMAT.attrib_format_float;
                    offset += (ushort)(bmesh.attrib[0].components * sizeOfFloat);
                    //uv
                    bmesh.attrib[1].offset = offset;
                    bmesh.attrib[1].normalized = 0;
                    bmesh.attrib[1].components = 2;
                    bmesh.attrib[1].format = (ushort)ATTRIB_FORMAT.attrib_format_float;
                    offset += (ushort)(bmesh.attrib[1].components * sizeOfFloat);
                    //normal
                    bmesh.attrib[2].offset = offset;
                    bmesh.attrib[2].normalized = 1;
                    bmesh.attrib[2].components = 3;
                    bmesh.attrib[2].format = (ushort)ATTRIB_FORMAT.attrib_format_float;
                    offset += (ushort)(bmesh.attrib[2].components * sizeOfFloat);
                    //tangent
                    bmesh.attrib[3].offset = offset;
                    bmesh.attrib[3].normalized = 1;
                    bmesh.attrib[3].components = 3;
                    bmesh.attrib[3].format = (ushort)ATTRIB_FORMAT.attrib_format_float;
                    offset += (ushort)(bmesh.attrib[3].components * sizeOfFloat);
                    //binormal
                    bmesh.attrib[4].offset = offset;
                    bmesh.attrib[4].normalized = 1;
                    bmesh.attrib[4].components = 3;
                    bmesh.attrib[4].format = (ushort)ATTRIB_FORMAT.attrib_format_float;
                }
                bmesh.attribDepth = new Attrib[(int)ATTRIB.maxAttribs];
                bmesh.vertexDataByteOffset = (uint)voffset * sizeOfVertex;
                bmesh.vertexCount = (uint)mvn;
                bmesh.indexDataByteOffset = (uint)ioffset * sizeOfUInt32;
                bmesh.indexCount = (uint)ids.Length;
                bmesh.vertexDataByteOffsetDepth = 0;
                bmesh.vertexCountDepth = 0;
                scene.model.subMeshes[soffset] = bmesh;
                ioffset += ids.Length;
                soffset++;
            }
            voffset += mvn;
            moffset++;
        }
        //header
        scene.header.subMeshCount = (uint)sn;
        scene.header.meshCount = (uint)mn;
        scene.header.materialCount = 0;
        scene.header.vertexDataByteSize = (uint)vn * sizeOfVertex;
        scene.header.indexDataByteSize = (uint)idn * sizeOfUInt32;
        scene.header.vertexDataByteSizeDepth = 0;
        scene.header.boundingBox.min = bounds.min;
        scene.header.boundingBox.max = bounds.max;
    }

    int WriteBSubMesh(ref BSubMesh bmesh, ref Stream stream)
    {
        byte[] arr = new byte[1000];
        int offset = 0;
        offset += BoundsToBytes(ref bmesh.boundingBox, ref arr, offset);
        offset += Int32ToBytes(ref bmesh.materialIndex, ref arr, offset);
        offset += Int32ToBytes(ref bmesh.attribsEnabled, ref arr, offset);
        offset += Int32ToBytes(ref bmesh.attribsEnabledDepth, ref arr, offset);
        offset += Int32ToBytes(ref bmesh.vertexStride, ref arr, offset);
        offset += Int32ToBytes(ref bmesh.vertexStrideDepth, ref arr, offset);
        offset += AttribToBytes(ref bmesh.attrib, ref arr, offset);
        offset += AttribToBytes(ref bmesh.attribDepth, ref arr, offset);
        offset += Int32ToBytes(ref bmesh.vertexDataByteOffset, ref arr, offset);
        offset += Int32ToBytes(ref bmesh.vertexCount, ref arr, offset);
        offset += Int32ToBytes(ref bmesh.indexDataByteOffset, ref arr, offset);
        offset += Int32ToBytes(ref bmesh.indexCount, ref arr, offset);
        offset += Int32ToBytes(ref bmesh.vertexDataByteOffsetDepth, ref arr, offset);
        offset += Int32ToBytes(ref bmesh.vertexCountDepth, ref arr, offset);
        offset = ((offset - 1) / 16 + 1) * 16;
        stream.Write(arr, 0, offset);
        return offset;
    }

    void WriteAnchor(ANCHOR anchor,ref Stream stream)
    {
        byte[] arr = new byte[4];
        int i = (int)anchor;
        var len = Int32ToBytes(ref i, ref arr);
        stream.Write(arr, 0, len);
    }

    #region to bytes
    int NodeToBytes(ref Node[] n, ref byte[] arr, int offset = 0)
    {
        unsafe
        {
            fixed (void* ptr = &n[0])
            {
                var len = sizeof(Node) * n.Length;
                Marshal.Copy((IntPtr)ptr, arr, offset, len);
                return len;
            }
        }
    }


    int BMeshToBytes(ref BMesh[] m, ref byte[] arr, int offset = 0)
    {
        unsafe
        {
            fixed (void* ptr = &m[0])
            {
                var len = sizeof(BMesh) * m.Length;
                Marshal.Copy((IntPtr)ptr, arr, offset, len);
                return len;
            }
        }
    }

    int HeaderToBytes(ref Header header, ref byte[] arr, int offset = 0)
    {
        unsafe
        {
            fixed (void* ptr = &header)
            {
                var len = sizeof(Header);
                Marshal.Copy((IntPtr)ptr, arr, offset, len);
                return len;
            }
        }
    }
    int BoundsToBytes(ref BoundingBox bounds, ref byte[] arr, int offset = 0)
    {
        unsafe
        {
            fixed (void* ptr = &bounds)
            {
                var len = sizeof(BoundingBox);
                Marshal.Copy((IntPtr)ptr, arr, offset, len);
                return len;
            }
        }
    }

    int VertexToBytes(ref Vertex v, ref byte[] arr, int offset = 0)
    {
        unsafe
        {
            fixed (void* ptr = &v)
            {
                var len = sizeof(Vertex);
                Marshal.Copy((IntPtr)ptr, arr, offset, len);
                return len;
            }
        }
    }
    int AttribToBytes(ref Attrib[] a, ref byte[] arr, int offset = 0)
    {
        unsafe
        {
            fixed (void* ptr = &a[0])
            {
                var len = sizeof(Attrib) * a.Length;
                Marshal.Copy((IntPtr)ptr, arr, offset, len);
                return len;
            }
        }
    }
    int VertexToBytes(ref Vertex[] v, ref byte[] arr, int offset = 0)
    {
        unsafe
        {
            fixed (void* ptr = &v[0])
            {
                var len = sizeof(Vertex) * v.Length;
                Marshal.Copy((IntPtr)ptr, arr, offset, len);
                return len;
            }
        }
    }
    int VertexToBytes(ref VertexEx[] v, ref byte[] arr, int offset = 0)
    {
        unsafe
        {
            fixed (void* ptr = &v[0])
            {
                var len = sizeof(VertexEx) * v.Length;
                Marshal.Copy((IntPtr)ptr, arr, offset, len);
                return len;
            }
        }
    }
    int Int32ToBytes(ref UInt32 v, ref byte[] arr, int offset = 0)
    {
        unsafe
        {
            fixed (void* ptr = &v)
            {
                var len = sizeof(UInt32);
                Marshal.Copy((IntPtr)ptr, arr, offset, len);
                return len;
            }
        }
    }
    int Int32ToBytes(ref Int32 v, ref byte[] arr, int offset = 0)
    {
        unsafe
        {
            fixed (void* ptr = &v)
            {
                var len = sizeof(Int32);
                Marshal.Copy((IntPtr)ptr, arr, offset, len);
                return len;
            }
        }
    }
    int Int32ToBytes(ref Int32[] v, ref byte[] arr, int offset = 0)
    {
        unsafe
        {
            fixed (void* ptr = &v[0])
            {
                var len = sizeof(Int32) * v.Length;
                Marshal.Copy((IntPtr)ptr, arr, offset, len);
                return len;
            }
        }
    }
    #endregion
}