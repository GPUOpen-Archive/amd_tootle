/************************************************************************************//**
// Copyright (c) 2006-2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
****************************************************************************************/
#include "TootlePCH.h"
#include "overdraw.h"
#include "soup.h"

#include "array.h"

#include "TootleRaytracer.h"

//=================================================================================================================================
//
//          Internal state
//
//=================================================================================================================================

/// Flag to indicate whether or not the overdraw module has been initialized
static bool s_bInitialized = false;

/// The current soup being optimized
static Soup* s_pSoup = NULL;

/// If number of clusters is higher than this, use the raytracing algorithm
const UINT RAYTRACE_CLUSTER_THRESHOLD = 225;



//=================================================================================================================================
//
//          Internal helper functions
//
//=================================================================================================================================
// compute face normals for the mesh.
static void ComputeFaceNormals(const float*        pfVB,
                               const unsigned int* pnIB,
                               unsigned int        nFaces,
                               float*              pFaceNormals);

//=================================================================================================================================
/// Computes the overdraw graph using the ray tracing implementation
///
/// \param pViewpoints         Array of viewpoints to use for overdraw computation.
/// \param nViewpoints         Size of the viewpoint array
/// \param bCullCCW            Specify true to cull CCW faces, otherwise cull CW faces.
/// \param rClusters           Array identifying the cluster for each face.  Faces are assumed sorted by cluster
/// \param nClusters           The number of clusters in rClusters.
/// \param rGraphOut           An array of edges that will contain the overdraw graph
/// \return TOOTLE_OK, or TOOTLE_OUT_OF_MEMORY
//=================================================================================================================================
TootleResult ODComputeGraphRaytrace(const float*      pViewpoints,
                                    unsigned int      nViewpoints,
                                    bool              bCullCCW,
                                    const Array<int>& rClusters,
                                    UINT              nClusters,
                                    Array<t_edge>&    rGraphOut)
{
    Array<Vector3> tn;

    if (!s_pSoup->ComputeTriNormals(tn))
    {
        return TOOTLE_OUT_OF_MEMORY;
    }

    // initialize per-cluster overdraw table
    TootleOverdrawTable fullgraph;

    try
    {
        fullgraph.resize(nClusters);

        for (int i = 0; i < (int) nClusters; i++)
        {
            fullgraph[i].resize(nClusters);

            for (int j = 0; j < (int) nClusters; j++)
            {
                fullgraph[i][j] = 0;
            }
        }
    }
    catch (std::bad_alloc&)
    {
        return TOOTLE_OUT_OF_MEMORY;
    }


    // initialize the ray tracer
    TootleRaytracer tr;
    const float* pVB          = &s_pSoup->v(0)[ 0 ];
    const UINT*  pIB          = (const UINT*) &s_pSoup->t(0)[ 0 ];
    const float* pFaceNormals = &tn[0][0];
    const UINT   nVertices    = (UINT) s_pSoup->v().size();
    const UINT   nFaces       = (UINT) s_pSoup->t().size();

    if (!tr.Init(pVB, pIB, pFaceNormals, nVertices, nFaces, (const UINT*) &rClusters[ 0 ]))
    {
        return TOOTLE_OUT_OF_MEMORY;
    }

    // generate the per-cluster overdraw table
    if (!tr.CalculateOverdraw(pViewpoints, nViewpoints, TOOTLE_RAYTRACE_IMAGE_SIZE, bCullCCW, &fullgraph))
    {
        return TOOTLE_OUT_OF_MEMORY;
    }

    // clean up the mess
    tr.Cleanup();

    // extract a directed graph from the overdraw table
    for (int i = 0; i < (int) nClusters; i++)
    {
        for (int j = 0; j < (int) nClusters; j++)
        {
            if (fullgraph[ i ] [ j ] > fullgraph[ j ][ i ])
            {
                t_edge t;
                t.from = i;
                t.to = j;
                t.cost = fullgraph[ i ][ j ] - fullgraph[ j ][ i ];

                if (!rGraphOut.PushBack(t))
                {
                    return TOOTLE_OUT_OF_MEMORY;
                }
            }
        }
    }

    return TOOTLE_OK;
}

//=================================================================================================================================
//
//          Exported functions
//
//=================================================================================================================================


//=================================================================================================================================
/// Initializes the overdraw computation module with a set of viewpoints
/// \return TOOTLE_OK if successful.  May return other results if errors occur
//=================================================================================================================================

TootleResult ODInit()
{
    return TOOTLE_OK;
}

//=================================================================================================================================
/// \return True if ODInit was called successfully, false otherwise
//=================================================================================================================================
bool ODIsInitialized()
{
    return true;
}


//=================================================================================================================================
/// Sets the triangle soup that will be used for the overdraw computations
/// It is not necessary to call this method again when the contents of the soup changes.  This will be done
/// automatically before computing overdraw
///
/// \param pSoup         The soup to use for overdraw computation
/// \param eFrontWinding The front face winding for the soup
/// \return TOOTLE_OK
///         TOOTLE_INTERNAL_ERROR if ODInit() hasn't been called,
///         TOOTLE_3D_API_ERROR if VB/IB allocation fails
//=================================================================================================================================
TootleResult ODSetSoup(Soup* pSoup, TootleFaceWinding /*eFrontWinding*/)
{
    s_pSoup = pSoup;

    return TOOTLE_OK;
}

//=================================================================================================================================
/// Computes the object overdraw for the triangle soup the ray tracing implementation
///
/// \param pfVB           A pointer to the vertex buffer.  The pointer pVB must point to the vertex position.  The vertex
///                        position must be a 3-component floating point value (X,Y,Z).
/// \param pnIB           The index buffer.  Must be a triangle list.
/// \param nVertices      The number of vertices. This must be non-zero and less than TOOTLE_MAX_VERTICES.
/// \param nFaces         The number of indices.  This must be non-zero and less than TOOTLE_MAX_FACES.
/// \param pViewpoints    The viewpoints to use to measure overdraw
/// \param nViewpoints    The number of viewpoints in the array
/// \param bCullCCW       Set to true to cull CCW faces, otherwise cull CW faces.
/// \param fODAvg         (Output) Average overdraw
/// \param fODMax         (Output) Maximum overdraw
/// \return TOOTLE_OK, TOOTLE_OUT_OF_MEMORY
//=================================================================================================================================
TootleResult ODObjectOverdrawRaytrace(const float*        pfVB,
                                      const unsigned int* pnIB,
                                      unsigned int        nVertices,
                                      unsigned int        nFaces,
                                      const float*        pViewpoints,
                                      unsigned int        nViewpoints,
                                      bool                bCullCCW,
                                      float&              fAvgOD,
                                      float&              fMaxOD)
{
    assert(pfVB);
    assert(pnIB);

    // compute face normals of the triangles
    float* pFaceNormals;

    try
    {
        pFaceNormals = new float [ 3 * nFaces ];
    }
    catch (std::bad_alloc&)
    {
        return TOOTLE_OUT_OF_MEMORY;
    }

    ComputeFaceNormals(pfVB, pnIB, nFaces, pFaceNormals);

    TootleRaytracer tr;

    if (!tr.Init(pfVB, pnIB, pFaceNormals, nVertices, nFaces, NULL))
    {
        delete[] pFaceNormals;

        return TOOTLE_OUT_OF_MEMORY;
    }

    // generate the per-cluster overdraw table
    if (!tr.MeasureOverdraw(pViewpoints, nViewpoints, TOOTLE_RAYTRACE_IMAGE_SIZE, bCullCCW, fAvgOD, fMaxOD))
    {
        delete[] pFaceNormals;
        return TOOTLE_OUT_OF_MEMORY;
    }

    // clean up the mess
    tr.Cleanup();

    delete[] pFaceNormals;

    return TOOTLE_OK;
}

//=================================================================================================================================
/// Calculate face normals for the mesh.
///
/// \param pfVB            A pointer to the vertex buffer.  The pointer pVB must point to the vertex position.  The vertex
///                         position must be a 3-component floating point value (X,Y,Z).
/// \param pnIB            The index buffer.  Must be a triangle list.
/// \param nFaces          The number of indices.  This must be non-zero and less than TOOTLE_MAX_FACES.
/// \param pFaceNormals    The output face normals.  May not be NULL.  Need to be pre-allocated of size 3*nFaces.
/// \param bFrontCWWinding Specify true if the mesh has CW front face winding, false otherwise.
///
/// \return void
//=================================================================================================================================
void ComputeFaceNormals(const float*        pfVB,
                        const unsigned int* pnIB,
                        unsigned int        nFaces,
                        float*              pFaceNormals)
{
    assert(pnIB);
    assert(pFaceNormals);

    // triangle index
    unsigned int nFirst;
    unsigned int nSecond;
    unsigned int nThird;
    Vector3 vNormal;

    for (unsigned int i = 0; i < nFaces; i++)
    {
        nFirst  = pnIB[ 3 * i     ];
        nSecond = pnIB[ 3 * i + 1 ];
        nThird  = pnIB[ 3 * i + 2 ];

        const Vector3 p0(pfVB[ 3 * nFirst  ], pfVB[ 3 * nFirst  + 1 ], pfVB[ 3 * nFirst  + 2 ]);
        const Vector3 p1(pfVB[ 3 * nSecond ], pfVB[ 3 * nSecond + 1 ], pfVB[ 3 * nSecond + 2 ]);
        const Vector3 p2(pfVB[ 3 * nThird  ], pfVB[ 3 * nThird  + 1 ], pfVB[ 3 * nThird  + 2 ]);

        Vector3 a = p0 - p1, b = p1 - p2;

        vNormal = Normalize(Cross(a, b));

        pFaceNormals[ 3 * i     ] = vNormal[ 0 ];
        pFaceNormals[ 3 * i + 1 ] = vNormal[ 1 ];
        pFaceNormals[ 3 * i + 2 ] = vNormal[ 2 ];
    }
}

//=================================================================================================================================
/// \param pViewpoints         Array of viewpoints to use for overdraw computation.
/// \param nViewpoints         Size of the viewpoint array
/// \param bCullCCW            Specify true to cull CCW faces, otherwise cull CW faces.
/// \param rClusters           Array identifying the cluster for each face.  Faces are assumed sorted by cluster
/// \param rClusterStart       Array giving the index of the first triangle in each cluster.  The size should be one plus the number
///                             of clusters.  The value of the last element of this array is the number of triangles in the mesh
/// \param rGraphOut           An array of edges that will contain the overdraw graph
/// \return TOOTLE_OK, TOOTLE_INTERNAL_ERROR, TOOTLE_3D_API_ERROR, TOOTLE_OUT_OF_MEMORY
//=================================================================================================================================
TootleResult ODOverdrawGraph(const float*            pViewpoints,
                             unsigned int            nViewpoints,
                             bool                    bCullCCW,
                             const Array<int>&       rClusters,
                             const Array<int>&       rClusterStart,
                             Array<t_edge>&          rGraphOut,
                             TootleOverdrawOptimizer eOverdrawOptimizer)
{
    if (!s_bInitialized || !s_pSoup)
    {
        // ODInit has not been called, or soup isn't set
        return TOOTLE_INTERNAL_ERROR;
    }

    // sanity check
    if (rClusters.size() != s_pSoup->t().size())
    {
        return TOOTLE_INTERNAL_ERROR;
    }

    switch (eOverdrawOptimizer)
    {
        case TOOTLE_OVERDRAW_RAYTRACE:

            return ODComputeGraphRaytrace(pViewpoints, nViewpoints, bCullCCW,
                                          rClusters, (UINT) rClusterStart.size() - 1, rGraphOut);
            break;

        default:
            return TOOTLE_INTERNAL_ERROR;
            break;
    }
}



//=================================================================================================================================
/// Cleans up any memory allocated by the overdraw module
//=================================================================================================================================

void ODCleanup()
{
}