/************************************************************************************//**
// Copyright (c) 2006-2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
****************************************************************************************/
#include <stdio.h>
#include <vector>
#include "tootlelib.h"

typedef unsigned int UINT;

//=================================================================================================================================
/// A utility function to sort the materials in a mesh to minimize overdraw, without optimizing within materials.  This kind of
///  optimization can be very effective by itself, or it can be combined with inter-material optimization for even better results.
///  This function is currently not used in the sample, it is provided for illustrative purposes only.
///
///  - pTriMaterialIDs is an array containing a material index for each triangle
///  - nMaterials is the number of materials in the mesh
///
/// The triangles are assumed to be sorted by material, meaning that pTriMaterialIDs contains 00000...1111....2222.... and so on...
///
/// \param pfVB              A pointer to the vertex buffer.  The pointer pVB must point to the vertex position.  The vertex
///                            position must be a 3-component floating point value (X,Y,Z).
/// \param pnIB               The mesh index buffer.  This must be a triangle list.  The faces must be clustered
/// \param nVertices          The number of vertices in the mesh.  This must be non-zero and less than TOOTLE_MAX_VERTICES.
/// \param nFaces             The number of faces in the mesh.  This must be non-zero and less than TOOTLE_MAX_FACES.
/// \param nVBStride          The distance between successive vertices in the vertex buffer, in bytes.  This must be at least
///                            3*sizeof(float)./// \param pnIB
/// \param pnTriMaterialIDs  An array containing a material index for each triangle.
/// \param nMaterials        the number of materials in the mesh.
///
/// \return      An array containing the new material order.  Element 0 in this array contains the ID of the
///               material that should be drawn first, element 1 contains the second, element 2 contains the third, and so on.
//=================================================================================================================================
UINT* SortMaterials(const float* pfVB,
                    const UINT*  pnIB,
                    UINT         nVertices,
                    UINT         nVBStride,
                    UINT         nFaces,
                    UINT*        pnTriMaterialIDs,
                    UINT         nMaterials)
{
    // make an array to hold the material re-mapping
    UINT* pnMaterialRemap = new UINT [ nMaterials ];

    // optimize the draw order
    TootleResult result = TootleOptimizeOverdraw(pfVB, pnIB, nVertices, nFaces, nVBStride,
                                                 NULL, 0,    // use default viewpoints
                                                 TOOTLE_CCW,
                                                 pnTriMaterialIDs,  // cluster IDs == material ID
                                                 NULL,             // do not care about re-mapped index buffer
                                                 pnMaterialRemap);

    if (result != TOOTLE_OK)
    {
        return NULL; // uh-oh
    }


    return pnMaterialRemap;
}
