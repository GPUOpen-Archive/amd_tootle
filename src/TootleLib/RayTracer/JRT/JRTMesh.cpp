/************************************************************************************//**
// Copyright (c) 2006-2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
****************************************************************************************/
#include "TootlePCH.h"
#include "JRTCommon.h"
#include "JRTMesh.h"
#include "JRTBoundingBox.h"

///  A mesh must have an array of positions and connectivity.  It may also have per-vertex normals
///  If per-vertex normals are omitted, then per-face normals are used instead.
///
///   The mesh does NOT delete the input arrays
///
/// \param rPositions
///    Array of vertex positions
/// \param pFaceNormals
///    Array of face normals.  Must be same length as nTriangleCount
/// \param nTriangleCount
///     Number of triangles
/// \param pIndices
///     An array of 3*nTriangleCount vertex indices
JRTMesh* JRTMesh::CreateMesh(const Vec3f* pPositions,
                             const Vec3f* pNormals,
                             UINT nVertices,
                             UINT nTriangleCount,
                             const UINT* pIndices)
{
    JRTMesh* pMesh = new JRTMesh();

    pMesh->m_nVertexCount = nVertices;
    pMesh->m_nTriangleCount = nTriangleCount;

    // copy vertex positions
    pMesh->m_pPositions = new Vec3f[ nVertices ];
    memcpy(pMesh->m_pPositions, pPositions, sizeof(Vec3f)*nVertices);

    // copy normals
    pMesh->m_pFaceNormals = new Vec3f[ nTriangleCount ];
    memcpy(pMesh->m_pFaceNormals, pNormals, nTriangleCount * sizeof(Vec3f));

    // create triangles
    pMesh->m_pTriangles = new JRTTriangle[ nTriangleCount ];

    for (UINT i = 0; i < nTriangleCount; i++)
    {
        pMesh->m_pTriangles[i].m_pMesh = pMesh;

        JRT_ASSERT(pIndices[0] < nVertices);
        JRT_ASSERT(pIndices[1] < nVertices);
        JRT_ASSERT(pIndices[2] < nVertices);

        pMesh->m_pTriangles[i].m_pV1 = (const float*) &pMesh->m_pPositions[ pIndices[0] ];
        pMesh->m_pTriangles[i].m_pV2 = (const float*) &pMesh->m_pPositions[ pIndices[1] ];
        pMesh->m_pTriangles[i].m_pV3 = (const float*) &pMesh->m_pPositions[ pIndices[2] ];



        pIndices += 3;
    }

    return pMesh;

}

JRTMesh::JRTMesh() :
    m_nTriangleCount(0),
    m_nVertexCount(0),
    m_pFaceNormals(NULL),
    m_pNormals(NULL),
    m_pPositions(NULL),
    m_pTriangles(NULL),
    m_pUVs(NULL)
{


}


JRTMesh::~JRTMesh()
{
    JRT_SAFE_DELETE_ARRAY(m_pPositions);
    JRT_SAFE_DELETE_ARRAY(m_pTriangles);
    JRT_SAFE_DELETE_ARRAY(m_pNormals);
    JRT_SAFE_DELETE_ARRAY(m_pFaceNormals);
}

void JRTMesh::Transform(const Matrix4f& rXForm, const Matrix4f& rInverse)
{
    // transform positions
    for (UINT i = 0; i < m_nVertexCount; i++)
    {
        TransformPoint(&m_pPositions[i], &rXForm, &m_pPositions[i]);
    }

    Matrix4f inverseTranspose = rInverse.Transpose();

    // transform normals
    if (m_pNormals)
    {
        for (UINT i = 0; i < m_nVertexCount; i++)
        {
            ALIGN16 Vec3f tmp = m_pNormals[i];
            TransformVector(&tmp, &inverseTranspose, &m_pNormals[i]);
            m_pNormals[i] = Normalize(m_pNormals[i]);
        }
    }
    else
    {
        // transform face normals
        for (UINT i = 0; i < m_nTriangleCount; i++)
        {
            ALIGN16 Vec3f tmp = m_pFaceNormals[i];
            TransformVector(&tmp, &inverseTranspose, &m_pFaceNormals[i]);
            m_pFaceNormals[i] = tmp;
        }
    }



}


void JRTMesh::GetInterpolants(UINT nTriIndex, const float barycentrics[], Vec3f* pNormal, Vec2f* /*pUV*/) const
{
    UINT nIndex1 = (UINT)(m_pTriangles[nTriIndex].m_pV1 - (const float*)m_pPositions) / 3;
    UINT nIndex2 = (UINT)(m_pTriangles[nTriIndex].m_pV2 - (const float*)m_pPositions) / 3;
    UINT nIndex3 = (UINT)(m_pTriangles[nTriIndex].m_pV3 - (const float*)m_pPositions) / 3;

    if (m_pNormals)
    {
        *pNormal = Normalize(BarycentricLerp3f(m_pNormals[nIndex1], m_pNormals[nIndex2], m_pNormals[nIndex3], barycentrics));
    }
    else
    {
        *pNormal = Normalize(m_pFaceNormals[ nTriIndex ]);
    }
}



void JRTMesh::RemoveTriangle(UINT nTri)
{
    if (nTri <= m_nTriangleCount)
    {
        // swap this triangle with the last one in the mesh
        JRTTriangle tmpTri = m_pTriangles[m_nTriangleCount - 1];
        m_pTriangles[m_nTriangleCount - 1] = m_pTriangles[nTri];
        m_pTriangles[nTri] = tmpTri;

        // swap face normals if there are any
        if (!m_pNormals)
        {
            Vec3f tmpNorm = m_pFaceNormals[ m_nTriangleCount - 1 ];
            m_pFaceNormals[m_nTriangleCount - 1] = m_pFaceNormals[nTri];
            m_pFaceNormals[ nTri ] = tmpNorm;
        }

        m_nTriangleCount--;
    }
}


JRTBoundingBox JRTMesh::ComputeBoundingBox() const
{
    return JRTBoundingBox((const float*)this->m_pPositions, m_nVertexCount);
}