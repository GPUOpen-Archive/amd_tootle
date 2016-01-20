/************************************************************************************//**
// Copyright (c) 2006-2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
****************************************************************************************/

#ifndef _OBJLOADER_H_
#define _OBJLOADER_H_

#include <algorithm>
#include <vector>
#include <functional>

//..............................................................................................................//
//..............................................................................................................//
//..............................................................................................................//
// OBJ File structure
//..............................................................................................................//
//..............................................................................................................//
//..............................................................................................................//

// 3D Vector ( for position and normals )
struct ObjVertex3D
{
    float x;
    float y;
    float z;
}; // End of ObjVertex3D


// 2D Vector ( for texture coordinates )
struct ObjVertex2D
{
    float x;
    float y;
}; // End of ObjVertex2D


// Final Vertex Structure
struct ObjVertexFinal
{
    ObjVertexFinal()
    {
        pos.x = 0.0f;
        pos.y = 0.0f;
        pos.z = 0.0f;

        normal.x = 0.0f;
        normal.y = 0.0f;
        normal.z = 0.0f;

        texCoord.x = 0.0f;
        texCoord.y = 0.0f;
    }; // End of Constructor

    ObjVertex3D pos;
    ObjVertex3D normal;
    ObjVertex2D texCoord;

    // indices of vertex, normal, and texcoord that make up this vertex
    unsigned int nVertexIndex;
    unsigned int nNormalIndex;
    unsigned int nTexcoordIndex;

}; // End of ObjVertexFinal


// Face
struct ObjFace
{
    ObjFace()
    {
        int i;

        for (i = 0; i < 3; i++)
        {
            vertexIndices[i]   = 0;
            texCoordIndices[i] = 0;
            normalIndices[i]   = 0;

            finalVertexIndices[i] = 0;
        } // End for
    }; // End of ObjFace

    unsigned int vertexIndices[3];
    unsigned int texCoordIndices[3];
    unsigned int normalIndices[3];

    // This is the index used to for rendering
    unsigned int finalVertexIndices[3];
}; // End of ObjFace




//=================================================================================================================================
/// \brief An OBJ file loader
///  This OBJ loader supports a subset of the OBJ file format.  In particular:
///    - It supports only polygonal primitives
///    - It does not support materials
///    - It does not support texture coordinates with three channels
///
//=================================================================================================================================
class ObjLoader
{
public:

    /// Loads a mesh from a wavefront OBJ file
    bool LoadGeometry(const char* strFileName, std::vector<ObjVertexFinal>& rVerticesOut, std::vector<ObjFace>& rFacesOut);


private:

    // VertexHashData
    struct VertexHashData
    {
        unsigned int vertexIndex;
        unsigned int texCoordIndex;
        unsigned int normalIndex;

        unsigned int finalIndex;
    }; // End of VertexHashData


    struct vertex_less : public std::less<VertexHashData>
    {
        bool operator()(const VertexHashData& x, const VertexHashData& y) const
        {
            if (x.vertexIndex < y.vertexIndex)
            {
                return true;
            }

            if (x.vertexIndex > y.vertexIndex)
            {
                return false;
            }

            if (x.texCoordIndex < y.texCoordIndex)
            {
                return true;
            }

            if (x.texCoordIndex > y.texCoordIndex)
            {
                return false;
            }

            if (x.normalIndex < y.normalIndex)
            {
                return true;
            }

            if (x.normalIndex > y.normalIndex)
            {
                return false;
            }

            return false;
        }; // End of operator()
    }; // End of vertex_less


    bool BuildModel(const std::vector<ObjVertex3D>& vertices,
                    const std::vector<ObjVertex3D>& normals,
                    const std::vector<ObjVertex2D>& texCoords,
                    std::vector<ObjVertexFinal>& finalVertices,
                    std::vector<ObjFace>& faces);

    // Returns how many vertices are specified in one line read from OBJ
    int  GetNumberOfVerticesInLine(const char* szLine);

    // Read vertex indices
    void ReadVertexIndices(const char* szLine, int numVertsInLine,
                           bool bHasTexCoords, bool bHasNormals,
                           int* pVertIndices, int* pTexCoords, int* pNormals);

    // Buildup vertex hash map
    void BuildFinalVertices(const std::vector<ObjVertex3D>& vertices,
                            const std::vector<ObjVertex3D>& normals,
                            const std::vector<ObjVertex2D>& texCoords,
                            std::vector<ObjFace>&     faces,
                            std::vector<ObjVertexFinal>& finalVertices);

};


#endif // _OBJLOADER_H_
