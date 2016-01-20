/************************************************************************************//**
// Copyright (c) 2006-2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
****************************************************************************************/
#ifndef _JRT_MESH_H_
#define _JRT_MESH_H_

#include "JRTCommon.h"

class JRTTriangle;

class JRTCSGNode;
class JRTSurfaceShader;
class JRTPhotonShader;
class JRTBoundingBox;

class JRTMeshAttributes
{
public:
    JRTMeshAttributes() : pShader(NULL), pCSGNode(NULL), bCastsShadows(true), bCastsCaustics(false)
    {};

    JRTSurfaceShader* pShader;
    JRTPhotonShader* pPhotonShader;
    JRTCSGNode* pCSGNode;
    bool bCastsShadows;
    bool bCastsCaustics;
};


class JRTMesh
{
public:

    /// Factory method for creating meshes
    static JRTMesh* CreateMesh(const Vec3f* pPositions,
                               const Vec3f* pNormals,
                               UINT nVertices,
                               UINT nTriangleCount,
                               const UINT* pIndices);

    /// Mesh constructor
    JRTMesh();

    /// Mesh destructor
    ~JRTMesh();

    /// Transforms the mesh and re-preprocesses the triangles
    void Transform(const Matrix4f& rXForm, const Matrix4f& rInverse);

    /// Returns the number of triangles in the mesh
    UINT GetTriangleCount() const { return m_nTriangleCount; };

    /// Accessor for the array of triangles
    inline const JRTTriangle* GetTriangles() const { return m_pTriangles; };


    void GetInterpolants(UINT nTriIndex, const float barycentrics[3], Vec3f* pNormal, Vec2f* pUV) const;

    const JRTMeshAttributes& GetAttributes() const { return m_attributes; };

    void SetAttributes(const JRTMeshAttributes& rAttribs) { m_attributes = rAttribs; };

    /// Removes a particular triangle from the mesh
    void RemoveTriangle(UINT nTri);

    JRTBoundingBox ComputeBoundingBox() const;

    const Vec3f& GetFaceNormal(UINT nTri) const { return m_pFaceNormals[nTri]; };

    const Vec3f& GetVertex(UINT i) const { return m_pPositions[i]; };
    void SetVertex(UINT i, const Vec3f& v) { m_pPositions[i] = v; };

private:

    Vec3f* m_pPositions;
    Vec3f* m_pNormals;
    Vec2f* m_pUVs;

    /// Pre-computed array of face normals.  This is used only if no vertex normals are provided
    Vec3f* m_pFaceNormals;

    JRTTriangle* m_pTriangles;

    UINT m_nTriangleCount;
    UINT m_nVertexCount;

    JRTMeshAttributes m_attributes;

};




class JRTTriangle
{
public:

    /// Returns the first vertex
    inline const Vec3f& GetV1() const { return *(const Vec3f*)m_pV1; };

    /// Returns the second vertex
    inline const Vec3f& GetV2() const { return *(const Vec3f*)m_pV2; };

    /// Returns the third vertex
    inline const Vec3f& GetV3() const { return *(const Vec3f*)m_pV3; };

    /// Returns the mesh which owns this triangle
    inline JRTMesh* GetMesh() const { return m_pMesh; };

    /// Returns the index of this triangle in its parent's triangle array
    inline UINT GetIndexInMesh() const { return (UINT)(this - m_pMesh->GetTriangles()); };

    inline const Vec3f& GetNormal() const { return m_pMesh->GetFaceNormal(GetIndexInMesh()); };
private:


    friend class JRTMesh;

    /// Pointer to first vertex position
    const float* m_pV1;

    /// Pointer to second vertex position
    const float* m_pV2;

    /// Pointer to third vertex position
    const float* m_pV3;

    /// Pointer to the mesh
    JRTMesh* m_pMesh;

};

#endif