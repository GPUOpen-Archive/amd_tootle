/************************************************************************************//**
// Copyright (c) 2006-2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
****************************************************************************************/

// This is a sample application which demonstrates the use of the AMD Triangle Order Optimization tool (Tootle).
// This sample application can read Wavefront .OBJ files (with some restrictions), and will re-order the faces in the
// input mesh to optimize vertex cache performance as well as overdraw.  The re-ordered mesh is emitted in OBJ format
// on standard output.  It can also re-order vertices to optimize for vertex cache prefetch performance.
//
// This sample allows the user to specify an optional viewpoint file.  If a viewpoint file is specified, then the viewpoints
// in the file will be used in place of the default viewpoint set.  Viewpoints must be points on or in the unit sphere.  During
// overdraw measurement, the mesh will be scaled and translated to fit inside this unit sphere, and will be rendered using
// orthographic projections from each viewpoint, looking at the origin of the model
//
// A user may want to use custom viewpoints to optimize a mesh if there are restrictions on the angles from which the object
// can be viewed.  For example, if an object is always viewed from above, and never from below, then it is advantageous to place
// all of the sample viewpoints on the upper hemisphere.
//
// The sample also allows the user to specify a target cluster count.  In general, users will want to let Tootle cluster
// automatically, but manually setting a high cluster count can sometimes provide greater overdraw reductions, at the expense of
// poor vertex cache performance.  Note that the cluster count that is passed to TootleClusterMesh is only a hint.  If the mesh
// is highly disconnected, then the algorithm may need to generate a larger number of clusters
//

// ignore VC++ warnings about fopen, fscanf, etc being deprecated
#if defined( _MSC_VER )
    #if _MSC_VER >= 1400
        #define _CRT_SECURE_NO_DEPRECATE
    #endif
#endif

#include <vector>
#include <list>
#include <string.h>
#include <string>
#include <iostream>
#include <fstream>
#include <cassert>
#include "option.h"
#include "ObjLoader.h"
#include "tootlelib.h"
#include "Timer.h"

//=================================================================================================================================
/// Enumeration for the choice of test cases for tootle.
//=================================================================================================================================
enum TootleAlgorithm
{
    NA_TOOTLE_ALGORITHM,                // Default invalid choice.
    TOOTLE_VCACHE_ONLY,                 // Only perform vertex cache optimization.
    TOOTLE_CLUSTER_VCACHE_OVERDRAW,     // Call the clustering, optimize vertex cache and overdraw individually.
    TOOTLE_FAST_VCACHECLUSTER_OVERDRAW, // Call the functions to optimize vertex cache and overdraw individually.  This is using
    //  the algorithm from SIGGRAPH 2007.
    TOOTLE_OPTIMIZE,                    // Call a single function to optimize vertex cache, cluster and overdraw.
    TOOTLE_FAST_OPTIMIZE                // Call a single function to optimize vertex cache, cluster and overdraw using
    //  a fast algorithm from SIGGRAPH 2007.
};

//=================================================================================================================================
/// A simple structure to store the settings for this sample app
//=================================================================================================================================
struct TootleSettings
{
    const char*           pMeshName ;
    const char*           pViewpointName ;
    unsigned int          nClustering ;
    unsigned int          nCacheSize;
    TootleFaceWinding     eWinding;
    TootleAlgorithm       algorithmChoice;         // five different types of algorithm to test Tootle
    TootleVCacheOptimizer eVCacheOptimizer;        // the choice for vertex cache optimization algorithm, it can be either
    //  TOOTLE_VCACHE_AUTO, TOOTLE_VCACHE_LSTRIPS, TOOTLE_VCACHE_DIRECT3D or
    //  TOOTLE_VCACHE_TIPSY.
    bool                  bOptimizeVertexMemory;   // true if you want to optimize vertex memory location, false to skip
    bool                  bMeasureOverdraw;        // true if you want to measure overdraw, false to skip
};

//=================================================================================================================================
/// A simple structure to hold Tootle statistics
//=================================================================================================================================
struct TootleStats
{
    unsigned int nClusters;
    float        fVCacheIn;
    float        fVCacheOut;
    float        fOverdrawIn;
    float        fOverdrawOut;
    float        fMaxOverdrawIn;
    float        fMaxOverdrawOut;
    double       fOptimizeVCacheTime;
    double       fClusterMeshTime;
    double       fOptimizeOverdrawTime;
    double       fVCacheClustersTime;
    double       fOptimizeVCacheAndClusterMeshTime;
    double       fTootleOptimizeTime;
    double       fTootleFastOptimizeTime;
    double       fMeasureOverdrawTime;
    double       fOptimizeVertexMemoryTime;
};

const float INVALID_TIME = -1;

//=================================================================================================================================
/// Reads a list of camera positions from a viewpoint file.
///
/// \param pFileName   The name of the file to read
/// \param rVertices   A vector which will be filled with vertex positions
/// \param rIndices    A vector which will will be filled with face indices
//=================================================================================================================================
bool LoadViewpoints(const char* pFileName, std::vector<ObjVertex3D>& rViewPoints)
{
    assert(pFileName);

    FILE* pFile = fopen(pFileName, "r");

    if (!pFile)
    {
        return false;
    }

    int iSize;

    if (fscanf(pFile, "%i\n", &iSize) != 1)
    {
        return false;
    }

    for (int i = 0; i < iSize; i++)
    {
        float x, y, z;

        if (fscanf(pFile, "%f %f %f\n", &x, &y, &z) != 3)
        {
            return false;
        }

        ObjVertex3D vert;
        vert.x = x;
        vert.y = y;
        vert.z = z;
        rViewPoints.push_back(vert);
    }

    fclose(pFile);

    return true;
}

//=================================================================================================================================
/// This function reads an obj file and re-emits its vertices and faces in the specified order
/// \param rInput      Input stream from which to read the obj
/// \param rOutput     Output stream on which to emit
/// \param rVertices   A vector containing the vertices that were referenced in the OBJ file
/// \param rIndices    A vector containing the sorted index buffer
/// \param vertexRemap A vector containing the remapped ID of the vertices.  Element i in the array will contain the new output
///                     location of the vertex i.  This is the result of TootleOptimizeVertexMemory().  May be NULL.
/// \param nVertices   The total number of vertices referenced in vertexRemap.
///
/// \return True if successful.  False otherwise
//=================================================================================================================================
bool EmitModifiedObj(std::istream& rInput, std::ostream& rOutput, const std::vector<ObjVertexFinal>& rVertices,
                     const std::vector<unsigned int>& rIndices, const unsigned int* vertexRemap,
                     unsigned int nVertices)
{
    // store the original copy of vertices into an array
    // we need to do this because rVertices contains the reordered vertex by ObjLoader using a hash map.
    std::vector<ObjVertex3D> inputVertices;
    inputVertices.reserve(rVertices.size());    // reserve at least this size, but the total input vertices might be larger.

    unsigned int nCount = 0;

    while (!rInput.eof())
    {
        std::string currentLine;
        std::getline(rInput, currentLine, '\n');
        const char* pLineText = currentLine.c_str();

        //if there is an "f " before the first space, then this is a face line
        if (strstr(pLineText, "f ") == strstr(pLineText, " ") - 1)
        {
            // face line
        }
        else if (strstr(pLineText, "v ") == strstr(pLineText, " ") - 1)        // vertex line
        {
            ObjVertex3D vert;

            // vertex line
            sscanf(pLineText, "v %f %f %f", &vert.x, &vert.y,
                   &vert.z);

            inputVertices.push_back(vert);

            nCount++;
        }
        else
        {
            // not a face line, pass it through
            rOutput << currentLine << std::endl;
        }
    }

    // Create a local copy of the vertex remapping array.
    //  Note that because the input vertices in the OBJ file might be larger than the vertex buffer
    //  in rVertices, vertexRemapping buffer might be larger than vertexRemap.
    std::vector<unsigned int> vertexRemapping;

    // if there is no vertex remapping, create a default one
    if (vertexRemap == NULL)
    {
        vertexRemapping.resize (nCount);

        for (unsigned int i = 0; i < nCount; i++)
        {
            vertexRemapping[ i ] = i;
        }
    }
    else
    {
        vertexRemapping.reserve (nCount);
        if (nVertices >= nCount)
        {
            vertexRemapping.assign (vertexRemap, vertexRemap + nCount);
        }
        else
        {
            vertexRemapping.assign (vertexRemap, vertexRemap + rVertices.size ());
        }

        // It is possible for the input list of vertices to be larger than the one in rVertices, thus,
        //  they are not mapped by TootleOptimizeVertexMemory().
        //  In that case, we will reassign the unmapped vertices to the end of the vertex buffer.

        for (unsigned int i = nVertices; i < nCount; i++)
        {
            vertexRemapping.push_back (i);
        }

        // print out the vertex mapping indexes in the output obj
        const unsigned int NUM_ITEMS_PER_LINE = 50;
        rOutput << "#vertexRemap = ";
        for (unsigned int i = 0; i < nCount; ++i)
        {
            rOutput << vertexRemapping[i] << " ";
            if ((i+1) % NUM_ITEMS_PER_LINE == 0)
            {
                rOutput << std::endl << "#vertexRemap = ";
            }
        }
        rOutput << std::endl;
    }

    // compute the inverse vertex mapping to output the remapped vertices
    std::vector<unsigned int> inverseVertexRemapping (nCount);

    unsigned int nVID;

    for (unsigned int i = 0; i < nCount; i++)
    {
        nVID = vertexRemapping[ i ];
        inverseVertexRemapping[ nVID ] = i;
    }

    // make sure that vertexRemapping and inverseVertexRemapping is consistent
    for (unsigned int i = 0; i < nCount; i++)
    {
        if (inverseVertexRemapping[ vertexRemapping[ i ] ] != i)
        {
            std::cerr << "EmitModifiedObj: inverseVertexRemapping and vertexRemapping is not consistent.\n";

            return false;
        }
    }

    // output the vertices
    for (unsigned int i = 0; i < nCount; i++)
    {
        rOutput << "v ";

        // output the remapped vertices (require an inverse mapping).
        nVID = inverseVertexRemapping[ i ];

        rOutput << inputVertices[ nVID ].x << " " << inputVertices[ nVID ].y << " "
                << inputVertices[ nVID ].z << std::endl;
    }

    // generate a new set of faces, using re-ordered index buffer
    for (unsigned int i = 0; i < rIndices.size(); i += 3)
    {
        rOutput << "f " ;

        for (int j = 0; j < 3; j++)
        {
            const ObjVertexFinal& rVertex = rVertices[ rIndices[ i + j ] ];
            rOutput << 1 + vertexRemapping[ rVertex.nVertexIndex - 1 ];

            if (rVertex.nNormalIndex > 0 && rVertex.nTexcoordIndex)
            {
                rOutput << "/" << rVertex.nTexcoordIndex << "/" << rVertex.nNormalIndex;
            }
            else if (rVertex.nNormalIndex > 0)
            {
                rOutput << "//" << rVertex.nNormalIndex;
            }
            else if (rVertex.nTexcoordIndex > 0)
            {
                rOutput << "/" << rVertex.nTexcoordIndex;
            }

            rOutput << " ";
        }

        rOutput << std::endl;
    }

    return true;
}

//=================================================================================================================================
/// Displays usage instructions, and calls exit()
/// \param nRet  Return code to pass to exit
//=================================================================================================================================
void ShowHelpAndExit(int nRet)
{
    fprintf(stderr,
            "Syntax:\n"
            " TootleSample [-v viewpointfile] [-c clusters] [-s cachesize] [-f] [-a [1-5]] [-o [1-4]] [-m] [-p] in.obj > out.obj\n"
            "  If -a is specified, the argument (below) that follows it will decide on the algorithm to use for Tootle.\n"
            "     1 -> perform vertex cache optimization only.\n"
            "     2 -> call the clustering, optimize vertex cache and overdraw using 3 separate function calls (mix-matching the old and new library).\n"
            "     3 -> call the functions to optimize vertex cache, cluster and overdraw individually (mix-matching the old and new library).\n"
            "     4 -> use a single utility function to optimize vertex cache, cluster and overdraw.\n"
            "     5 -> use a single utility function to optimize vertex cache, cluster and overdraw (SIGGRAPH 2007 version).\n"
            "  If -f is specified, counter-clockwise faces are front facing.  Otherwise, clockwise faces are front facing.\n"
            "  If -m is specified, the algorithm to measure overdraw will be skipped.\n"
            "  If -o is specified, the argument that follows it will decide on the algorithm used for vertex cache optimization.\n"
            "     1 -> the choice of algorithm for vertex cache optimization will depend on the vertex cache size.\n"
            "     2 -> use the D3DXOptimizeFaces to optimize vertex cache.\n"
            "     3 -> use a list like triangle strips to optimize vertex cache (good for cache size <=6).\n"
            "     4 -> use Tipsy algorithm from SIGGRAPH 2007 to optimize vertex cache.\n"
            "   If -p is specified, the algorithm to optimize the vertex memory for prefetch cache will be skipped.\n");

    exit(nRet);
}

//=================================================================================================================================
/// Displays a tootle error message
/// \param result The tootle result that is being displayed
//=================================================================================================================================
void DisplayTootleErrorMessage(TootleResult eResult)
{
    std::cerr << "Tootle returned error: " << std::endl;

    switch (eResult)
    {
        case NA_TOOTLE_RESULT:
            std::cerr << " NA_TOOTLE_RESULT"       << std::endl;
            break;

        case TOOTLE_OK:
            break;

        case TOOTLE_INVALID_ARGS:
            std::cerr << " TOOTLE_INVALID_ARGS"    << std::endl;
            break;

        case TOOTLE_OUT_OF_MEMORY:
            std::cerr << " TOOTLE_OUT_OF_MEMORY"   << std::endl;
            break;

        case TOOTLE_3D_API_ERROR:
            std::cerr << " TOOTLE_3D_API_ERROR"    << std::endl;
            break;

        case TOOTLE_INTERNAL_ERROR:
            std::cerr << " TOOTLE_INTERNAL_ERROR"  << std::endl;
            break;

        case TOOTLE_NOT_INITIALIZED:
            std::cerr << " TOOTLE_NOT_INITIALIZED" << std::endl;
            break;
    }
}

//=================================================================================================================================
/// Convert from the command line argument type to the enum TootleAlgorithm type.
/// \param nArgument  the command line argument (1 to 6)
/// \return One of the type of TootleAlgorithm enum.  It will display how to use the command line argument and exit when error
///          occurs.
//=================================================================================================================================
TootleAlgorithm UIntToTootleAlgorithm(unsigned int nArgument)
{
    switch (nArgument)
    {
        case 1:
            return TOOTLE_VCACHE_ONLY;

        case 2:
            return TOOTLE_CLUSTER_VCACHE_OVERDRAW;

        case 3:
            return TOOTLE_FAST_VCACHECLUSTER_OVERDRAW;

        case 4:
            return TOOTLE_OPTIMIZE;

        case 5:
            return TOOTLE_FAST_OPTIMIZE;

        default:
            ShowHelpAndExit(0);
            return NA_TOOTLE_ALGORITHM;
            break;
    };
}

//=================================================================================================================================
/// Convert from the command line argument type to the enum TootleVCacheOptimizer type.
/// \param nArgument  the command line argument (1 to 4)
/// \return One of the type of TootleVCacheOptimizer enum.  It will display how to use the command line argument and exit
///          when error occurs.
//=================================================================================================================================
TootleVCacheOptimizer UIntToTootleVCacheOptimizer(unsigned int nArgument)
{
    switch (nArgument)
    {
        case 1:
            return TOOTLE_VCACHE_AUTO;

        case 2:
            return TOOTLE_VCACHE_DIRECT3D;

        case 3:
            return TOOTLE_VCACHE_LSTRIPS;

        case 4:
            return TOOTLE_VCACHE_TIPSY;

        default:
            ShowHelpAndExit(0);
            return NA_TOOTLE_VCACHE_OPTIMIZER;
            break;
    };
}

//=================================================================================================================================
/// Displays usage instructions, and calls exit()
/// \param nRet  Return code to pass to exit
//=================================================================================================================================
void ParseCommandLine(int argc, char* argv[], TootleSettings* pSettings)
{
    assert(pSettings);

    Option::Definition options[] =
    {
        { 'a', "Algorithm to use for TootleSample (1 to 5)" },
        { 'c', "Number of clusters" },
        { 'f', "Treat counter-clockwise faces as front facing (instead clockwise faces)." },
        { 'h', "Help" },
        { 'm', "Skip measuring overdraw" },
        { 'o', "Algorithm to use to optimize vertex cache (1 to 4)." },
        { 'p', "Skip vertex prefetch cache optimization" },
        { 's', "Post TnL vcache size" },
        { 'v', "Viewpoint file" },
        { 0, NULL },
    };

    Option opt;
    char cOption = opt.Parse(argc, argv, options);

    unsigned int nAlgorithmChoice;
    unsigned int nVCacheOptimizer;

    while (cOption != -1)
    {
        switch (cOption)
        {
            case 'a':
                nAlgorithmChoice = atoi(opt.GetArgument(argc, argv));
                pSettings->algorithmChoice = UIntToTootleAlgorithm(nAlgorithmChoice);
                break;

            case 'c':
                pSettings->nClustering = atoi(opt.GetArgument(argc, argv));
                break;

            case 'f':
                pSettings->eWinding = TOOTLE_CCW;
                break;

            case 'h':
                ShowHelpAndExit(0);
                break;

            case 'm':
                pSettings->bMeasureOverdraw = false;
                break;

            case 'o':
                nVCacheOptimizer = atoi(opt.GetArgument(argc, argv));
                pSettings->eVCacheOptimizer = UIntToTootleVCacheOptimizer(nVCacheOptimizer);
                break;

            case 'p':
                pSettings->bOptimizeVertexMemory = false;
                break;

            case 's':
                pSettings->nCacheSize = atoi(opt.GetArgument(argc, argv));
                break;

            case 'v':
                if (!pSettings->pViewpointName)
                {
                    pSettings->pViewpointName = opt.GetArgument(argc, argv);
                }
                else
                {
                    ShowHelpAndExit(1);
                }

                break;

            // option with no switch.  This is the mesh filename
            case '?':
                if (!pSettings->pMeshName)
                {
                    pSettings->pMeshName = opt.GetArgument(argc, argv);
                }
                else
                {
                    ShowHelpAndExit(1);
                }

                break;

            default:
                ShowHelpAndExit(1);
                break;
        }

        cOption = opt.Parse(argc, argv, options);
    }

    // make sure we got a mesh name
    if (!pSettings->pMeshName)
    {
        ShowHelpAndExit(1);
    }
}

//=================================================================================================================================
/// Displays the vertex cache optimizer selection.
///
/// \param fp               The file pointer.
/// \param eVCacheOptimizer The vertex cache optimizer enum.
/// \param nCacheSize       The hardware vertex cache size.
//=================================================================================================================================
void PrintVCacheOptimizer(FILE* fp, TootleVCacheOptimizer eVCacheOptimizer, unsigned int nCacheSize)
{
    assert(fp);

    switch (eVCacheOptimizer)
    {
        case TOOTLE_VCACHE_AUTO:
            if (nCacheSize <= 6)
            {
                fprintf(fp, "#Vertex Cache Optimizer: AUTO (LStrips)\n");
            }
            else
            {
                fprintf(fp, "#Vertex Cache Optimizer: AUTO (Tipsy)\n");
            }

            break;

        case TOOTLE_VCACHE_DIRECT3D:
            fprintf(fp, "#Vertex Cache Optimizer: Direct3D\n");
            break;

        case TOOTLE_VCACHE_LSTRIPS:
            fprintf(fp, "#Vertex Cache Optimizer: LStrips (a custom algorithm to create a list like triangle strips)\n");
            break;

        case TOOTLE_VCACHE_TIPSY:
            fprintf(fp, "#Vertex Cache Optimizer: Tipsy (an algorithm from SIGGRAPH 2007)\n");
            break;

        case NA_TOOTLE_VCACHE_OPTIMIZER:
        default:
            fprintf(fp, "#Vertex Cache Optimizer: Error input\n");
            break;
    }
}

//=================================================================================================================================
/// Displays the overdraw optimizer selection.
///
/// \param fp                 The file pointer.
/// \param eOverdrawOptimizer The overdraw optimizer enum.
/// \param nClusters          The number of clusters generated.
//=================================================================================================================================
void PrintOverdrawOptimizer(FILE* fp, TootleOverdrawOptimizer eOverdrawOptimizer, unsigned int nClusters)
{
    assert(fp);

    switch (eOverdrawOptimizer)
    {
        case TOOTLE_OVERDRAW_AUTO:
#ifdef _SOFTWARE_ONLY_VERSION
            fprintf(fp, "#Overdraw Optimizer    : TOOTLE_OVERDRAW_AUTO (Software renderer)\n");
#else

            if (nClusters > 225)
            {
                fprintf(fp, "#Overdraw Optimizer    : TOOTLE_OVERDRAW_AUTO (Software renderer)\n");
            }
            else
            {
                fprintf(fp, "#Overdraw Optimizer    : TOOTLE_OVERDRAW_AUTO (Direct3D renderer)\n");
            }

#endif
            break;

        case TOOTLE_OVERDRAW_DIRECT3D:
            fprintf(fp, "#Overdraw Optimizer    : TOOTLE_OVERDRAW_DIRECT3D (Direct3D renderer)\n");
            break;

        case TOOTLE_OVERDRAW_RAYTRACE:
            fprintf(fp, "#Overdraw Optimizer    : TOOTLE_OVERDRAW_RAYTRACE (Software renderer)\n");
            break;

        case TOOTLE_OVERDRAW_FAST:
            fprintf(fp, "#Overdraw Optimizer    : TOOTLE_OVERDRAW_FAST (SIGGRAPH 2007 version)\n");
            break;

        case NA_TOOTLE_OVERDRAW_OPTIMIZER:
        default:
            fprintf(fp, "#Overdraw Optimizer    : Error input\n");
            break;
    }
}

//=================================================================================================================================
/// Displays the tootle algorithm selection.
///
/// \param fp               The file pointer
/// \param eVCacheOptimizer The vertex cache optimizer enum.
/// \param eAlgorithmChoice The algorithm choice enum.
/// \param nCacheSize       The hardware vertex cache size.
/// \param nClusters        Total number of clusters generated.
//=================================================================================================================================
void PrintAlgorithm(FILE* fp, TootleVCacheOptimizer eVCacheOptimizer, TootleAlgorithm eAlgorithmChoice, unsigned int nCacheSize,
                    unsigned int nClusters)
{
    assert(fp);

    fprintf(fp, "#Tootle Settings\n");
    fprintf(fp, "#Vertex cache size     : %u\n", nCacheSize);

    switch (eAlgorithmChoice)
    {
        case TOOTLE_VCACHE_ONLY:
            PrintVCacheOptimizer(fp, eVCacheOptimizer, nCacheSize);
            break;

        case TOOTLE_CLUSTER_VCACHE_OVERDRAW:
            PrintVCacheOptimizer(fp, eVCacheOptimizer, nCacheSize);
            fprintf(fp, "#Algorithm             : TootleClusterMesh, TootleVCacheClusters and TootleOptimizeOverdraw\n");
            PrintOverdrawOptimizer(fp, TOOTLE_OVERDRAW_AUTO, nClusters);
            break;

        case TOOTLE_FAST_VCACHECLUSTER_OVERDRAW:
            fprintf(fp, "#Algorithm             : TootleFastOptimizeVCacheAndClusterMesh and TootleOptimizeOverdraw\n");
            PrintOverdrawOptimizer(fp, TOOTLE_OVERDRAW_AUTO, nClusters);
            break;

        case TOOTLE_OPTIMIZE:
            PrintVCacheOptimizer(fp, eVCacheOptimizer, nCacheSize);
            fprintf(fp, "#Algorithm             : TootleOptimize\n");
            PrintOverdrawOptimizer(fp, TOOTLE_OVERDRAW_FAST, nClusters);
            break;

        case TOOTLE_FAST_OPTIMIZE:
            fprintf(fp, "#Algorithm             : TootleFastOptimize\n");
            PrintOverdrawOptimizer(fp, TOOTLE_OVERDRAW_FAST, nClusters);
            break;

        default:
            fprintf(fp, "#Algorithm             : Error input\n");
            break;
    }

    fprintf(fp, "\n");
}


//=================================================================================================================================
/// A helper function to print formatted TOOTLE statistics
/// \param f      A file to print the statistics to
/// \param pStats The statistics to be printed
//=================================================================================================================================
void PrintStats(FILE* fp, TootleStats* pStats)
{
    assert(fp);
    assert(pStats);

    fprintf(fp, "#Tootle Stats\n"
            "#Clusters         : %u\n"
            "#CacheIn/Out      : %.3fx (%.3f/%.3f)\n",
            pStats->nClusters,
            pStats->fVCacheIn / pStats->fVCacheOut,
            pStats->fVCacheIn,
            pStats->fVCacheOut);

    if (pStats->fMeasureOverdrawTime >= 0)
    {
        fprintf(fp, "#OverdrawIn/Out   : %.3fx (%.3f/%.3f)\n"
                "#OverdrawMaxIn/Out: %.3fx (%.3f/%.3f)\n",
                pStats->fOverdrawIn / pStats->fOverdrawOut,
                pStats->fOverdrawIn,
                pStats->fOverdrawOut,
                pStats->fMaxOverdrawIn / pStats->fMaxOverdrawOut,
                pStats->fMaxOverdrawIn,
                pStats->fMaxOverdrawOut);
    }

    fprintf(fp, "\n#Tootle Timings\n");

    // print out the timing result if appropriate.
    if (pStats->fOptimizeVCacheTime >= 0)
    {
        fprintf(fp, "#OptimizeVCache               = %.4lf seconds\n", pStats->fOptimizeVCacheTime);
    }

    if (pStats->fClusterMeshTime >= 0)
    {
        fprintf(fp, "#ClusterMesh                  = %.4lf seconds\n", pStats->fClusterMeshTime);
    }

    if (pStats->fVCacheClustersTime >= 0)
    {
        fprintf(fp, "#VCacheClusters               = %.4lf seconds\n", pStats->fVCacheClustersTime);
    }

    if (pStats->fOptimizeVCacheAndClusterMeshTime >= 0)
    {
        fprintf(fp, "#OptimizeVCacheAndClusterMesh = %.4lf seconds\n", pStats->fOptimizeVCacheAndClusterMeshTime);
    }

    if (pStats->fOptimizeOverdrawTime >= 0)
    {
        fprintf(fp, "#OptimizeOverdraw             = %.4lf seconds\n", pStats->fOptimizeOverdrawTime);
    }

    if (pStats->fTootleOptimizeTime >= 0)
    {
        fprintf(fp, "#TootleOptimize               = %.4lf seconds\n", pStats->fTootleOptimizeTime);
    }

    if (pStats->fTootleFastOptimizeTime >= 0)
    {
        fprintf(fp, "#TootleFastOptimize           = %.4lf seconds\n", pStats->fTootleFastOptimizeTime);
    }

    if (pStats->fMeasureOverdrawTime >= 0)
    {
        fprintf(fp, "#MeasureOverdraw              = %.4lf seconds\n", pStats->fMeasureOverdrawTime);
    }

    if (pStats->fOptimizeVertexMemoryTime >= 0)
    {
        fprintf(fp, "#OptimizeVertexMemory         = %.4lf seconds\n", pStats->fOptimizeVertexMemoryTime);
    }
}

//=================================================================================================================================
/// The main function.
//=================================================================================================================================
int main(int argc, char* argv[])
{
    // initialize settings to defaults
    TootleSettings settings;
    settings.pMeshName             = NULL;
    settings.pViewpointName        = NULL;
    settings.nClustering           = 0;
    settings.nCacheSize            = TOOTLE_DEFAULT_VCACHE_SIZE;
    settings.eWinding              = TOOTLE_CW;
    settings.algorithmChoice       = TOOTLE_OPTIMIZE;
    settings.eVCacheOptimizer      = TOOTLE_VCACHE_AUTO;             // the auto selection as the default to optimize vertex cache
    settings.bOptimizeVertexMemory = true;                           // default value is to optimize the vertex memory
    settings.bMeasureOverdraw      = true;                           // default is to measure overdraw
    
    // parse the command line
    ParseCommandLine(argc, argv, &settings);

    // ***************************************************
    //   Load the mesh
    // ***************************************************

    // read the mesh from the OBJ file
    std::vector<ObjVertexFinal> objVertices;
    std::vector<ObjFace>        objFaces;

    ObjLoader loader;

    if (!loader.LoadGeometry(settings.pMeshName, objVertices, objFaces))
    {
        std::cerr << "Error loading mesh file: " << settings.pMeshName << std::endl;
        return 1;
    }

    // build buffers containing only the vertex positions and indices, since this is what Tootle requires
    std::vector<ObjVertex3D> vertices;
    vertices.resize(objVertices.size());

    for (unsigned int i = 0; i < vertices.size(); i++)
    {
        vertices[i] = objVertices[i].pos;
    }

    std::vector<unsigned int> indices;
    indices.resize(objFaces.size() * 3);

    for (unsigned int i = 0; i < indices.size(); i++)
    {
        indices[i] = objFaces[ i / 3 ].finalVertexIndices[ i % 3 ];
    }

    // ******************************************
    //    Load viewpoints if necessary
    // ******************************************

    // read viewpoints if needed
    std::vector<ObjVertex3D> viewpoints;

    if (settings.pViewpointName != NULL)
    {
        if (!LoadViewpoints(settings.pViewpointName, viewpoints))
        {
            std::cerr << "Unable to load viewpoints from file: " << settings.pViewpointName;
            return 1;
        }
    }

    // if we didn't get any viewpoints, then use a NULL array
    const float* pViewpoints = NULL;
    unsigned int nViewpoints = (unsigned int) viewpoints.size();

    if (viewpoints.size() > 0)
    {
        pViewpoints = (const float*) &viewpoints[0];
    }

    // *****************************************************************
    //   Prepare the mesh and initialize stats variables
    // *****************************************************************

    unsigned int  nFaces    = (unsigned int)  indices.size() / 3;
    unsigned int  nVertices = (unsigned int)  vertices.size();
    float*        pfVB      = (float*)        &vertices[0];
    unsigned int* pnIB      = (unsigned int*) &indices[0];
    unsigned int  nStride   = 3 * sizeof(float);

    TootleStats stats;

    // initialize the timing variables
    stats.fOptimizeVCacheTime               = INVALID_TIME;
    stats.fClusterMeshTime                  = INVALID_TIME;
    stats.fVCacheClustersTime               = INVALID_TIME;
    stats.fOptimizeVCacheAndClusterMeshTime = INVALID_TIME;
    stats.fOptimizeOverdrawTime             = INVALID_TIME;
    stats.fTootleOptimizeTime               = INVALID_TIME;
    stats.fTootleFastOptimizeTime           = INVALID_TIME;
    stats.fMeasureOverdrawTime              = INVALID_TIME;
    stats.fOptimizeVertexMemoryTime         = INVALID_TIME;

    TootleResult result;

    // initialize Tootle
    result = TootleInit();

    if (result != TOOTLE_OK)
    {
        DisplayTootleErrorMessage(result);
        return 1;
    }

    // measure input VCache efficiency
    result = TootleMeasureCacheEfficiency(pnIB, nFaces, settings.nCacheSize, &stats.fVCacheIn);

    if (result != TOOTLE_OK)
    {
        DisplayTootleErrorMessage(result);
        return 1;
    }

    if (settings.bMeasureOverdraw)
    {
        // measure input overdraw.  Note that we assume counter-clockwise vertex winding.
        result = TootleMeasureOverdraw(pfVB, pnIB, nVertices, nFaces, nStride, pViewpoints, nViewpoints, settings.eWinding,
                                       &stats.fOverdrawIn, &stats.fMaxOverdrawIn);

        if (result != TOOTLE_OK)
        {
            DisplayTootleErrorMessage(result);
            return 1;
        }
    }

    // allocate an array to hold the cluster ID for each face
    std::vector<unsigned int> faceClusters;
    faceClusters.resize(nFaces + 1);
    unsigned int nNumClusters;

    Timer timer;
    timer.Reset();

    // **********************************************************************************************************************
    //   Optimize the mesh:
    //
    // The following cases show five examples for developers on how to use the library functions in Tootle.
    // 1. If you are interested in optimizing vertex cache only, see the TOOTLE_VCACHE_ONLY case.
    // 2. If you are interested to optimize vertex cache and overdraw, see either TOOTLE_CLUSTER_VCACHE_OVERDRAW
    //     or TOOTLE_OPTIMIZE cases.  The former uses three separate function calls while the latter uses a single
    //     utility function.
    // 3. To use the algorithm from SIGGRAPH 2007 (v2.0), see TOOTLE_FAST_VCACHECLUSTER_OVERDRAW or TOOTLE_FAST_OPTIMIZE
    //     cases.  The former uses two separate function calls while the latter uses a single utility function.
    //
    // Note the algorithm from SIGGRAPH 2007 (v2.0) is very fast but produces less quality results especially for the
    //  overdraw optimization.  During our experiments with some medium size models, we saw an improvement of 1000x in
    //  running time (from 20+ minutes to less than 1 second) for using v2.0 calls vs v1.2 calls.  The resulting vertex
    //  cache optimization is very similar while the overdraw optimization drops from 3.8x better to 2.5x improvement over
    //  the input mesh.
    //  Developers should always run the overdraw optimization using the fast algorithm from SIGGRAPH initially.
    //  If they require a better result, then re-run the overdraw optimization using the old v1.2 path (TOOTLE_OVERDRAW_AUTO).
    //  Passing TOOTLE_OVERDRAW_AUTO to the algorithm will let the algorithm choose between Direct3D or raytracing path
    //  depending on the total number of clusters (less than 200 clusters, it will use Direct3D path otherwise it will
    //  use raytracing path since the raytracing path will be faster than the Direct3D path at that point).
    //
    // Tips: use v2.0 for fast optimization, and v1.2 to further improve the result by mix-matching the calls.
    // **********************************************************************************************************************

    switch (settings.algorithmChoice)
    {
        case TOOTLE_VCACHE_ONLY:
            // *******************************************************************************************************************
            // Perform Vertex Cache Optimization ONLY
            // *******************************************************************************************************************

            stats.nClusters = 1;

            // Optimize vertex cache
            result = TootleOptimizeVCache(pnIB, nFaces, nVertices, settings.nCacheSize,
                                          pnIB, NULL, settings.eVCacheOptimizer);

            if (result != TOOTLE_OK)
            {
                DisplayTootleErrorMessage(result);
                return 1;
            }

            stats.fOptimizeVCacheTime = timer.GetElapsed();
            break;

        case TOOTLE_CLUSTER_VCACHE_OVERDRAW:
            // *******************************************************************************************************************
            // An example of calling clustermesh, vcacheclusters and optimize overdraw individually.
            // This case demonstrate mix-matching v1.2 clustering with v2.0 overdraw optimization.
            // *******************************************************************************************************************

            // Cluster the mesh, and sort faces by cluster.
            result = TootleClusterMesh(pfVB, pnIB, nVertices, nFaces, nStride, settings.nClustering, pnIB, &faceClusters[0], NULL);

            if (result != TOOTLE_OK)
            {
                DisplayTootleErrorMessage(result);
                return 1;
            }

            stats.fClusterMeshTime = timer.GetElapsed();
            timer.Reset();

            // The last entry of of faceClusters store the total number of clusters.
            stats.nClusters = faceClusters[ nFaces ];

            // Perform vertex cache optimization on the clustered mesh.
            result = TootleVCacheClusters(pnIB, nFaces, nVertices, settings.nCacheSize, &faceClusters[0],
                                          pnIB, NULL, settings.eVCacheOptimizer);

            if (result != TOOTLE_OK)
            {
                DisplayTootleErrorMessage(result);
                return 1;
            }

            stats.fVCacheClustersTime = timer.GetElapsed();
            timer.Reset();

            // Optimize the draw order (using v1.2 path: TOOTLE_OVERDRAW_AUTO, the default path is from v2.0--SIGGRAPH version).
            result = TootleOptimizeOverdraw(pfVB, pnIB, nVertices, nFaces, nStride, pViewpoints, nViewpoints, settings.eWinding,
                                            &faceClusters[0], pnIB, NULL, TOOTLE_OVERDRAW_AUTO);

            if (result != TOOTLE_OK)
            {
                DisplayTootleErrorMessage(result);
                return 1;
            }

            stats.fOptimizeOverdrawTime = timer.GetElapsed();
            break;

        case TOOTLE_FAST_VCACHECLUSTER_OVERDRAW:
            // *******************************************************************************************************************
            // An example of calling v2.0 optimize vertex cache and clustering mesh with v1.2 overdraw optimization.
            // *******************************************************************************************************************

            // Optimize vertex cache and create cluster
            // The algorithm from SIGGRAPH combine the vertex cache optimization and clustering mesh into a single step
            result = TootleFastOptimizeVCacheAndClusterMesh(pnIB, nFaces, nVertices, settings.nCacheSize, pnIB,
                                                            &faceClusters[0], &nNumClusters, TOOTLE_DEFAULT_ALPHA);

            if (result != TOOTLE_OK)
            {
                // an error detected
                DisplayTootleErrorMessage(result);
                return 1;
            }

            stats.fOptimizeVCacheAndClusterMeshTime = timer.GetElapsed();
            timer.Reset();

            stats.nClusters = nNumClusters;

            // In this example, we use TOOTLE_OVERDRAW_AUTO to show that we can mix-match the clustering and
            //  vcache computation from the new library with the overdraw optimization from the old library.
            //  TOOTLE_OVERDRAW_AUTO will choose between using Direct3D or CPU raytracing path.  This path is
            //  much slower than TOOTLE_OVERDRAW_FAST but usually produce 2x better results.
            result = TootleOptimizeOverdraw(pfVB, pnIB, nVertices, nFaces, nStride, NULL, 0,
                                            settings.eWinding, &faceClusters[0], pnIB, NULL, TOOTLE_OVERDRAW_AUTO);

            if (result != TOOTLE_OK)
            {
                // an error detected
                DisplayTootleErrorMessage(result);
                return 1;
            }

            stats.fOptimizeOverdrawTime = timer.GetElapsed();

            break;

        case TOOTLE_OPTIMIZE:
            // *******************************************************************************************************************
            // An example of using a single utility function to perform v1.2 optimizations.
            // *******************************************************************************************************************

            // This function will compute the entire optimization (cluster mesh, vcache per cluster, and optimize overdraw).
            // It will use TOOTLE_OVERDRAW_FAST as the default overdraw optimization
            result = TootleOptimize(pfVB, pnIB, nVertices, nFaces, nStride, settings.nCacheSize,
                                    pViewpoints, nViewpoints, settings.eWinding, pnIB, &nNumClusters, settings.eVCacheOptimizer);

            if (result != TOOTLE_OK)
            {
                DisplayTootleErrorMessage(result);
                return 1;
            }

            stats.fTootleOptimizeTime = timer.GetElapsed();

            stats.nClusters = nNumClusters;
            break;

        case TOOTLE_FAST_OPTIMIZE:
            // *******************************************************************************************************************
            // An example of using a single utility function to perform v2.0 optimizations.
            // *******************************************************************************************************************

            // This function will compute the entire optimization (optimize vertex cache, cluster mesh, and optimize overdraw).
            // It will use TOOTLE_OVERDRAW_FAST as the default overdraw optimization
            result = TootleFastOptimize(pfVB, pnIB, nVertices, nFaces, nStride, settings.nCacheSize,
                                        settings.eWinding, pnIB, &nNumClusters, TOOTLE_DEFAULT_ALPHA);

            if (result != TOOTLE_OK)
            {
                DisplayTootleErrorMessage(result);
                return 1;
            }

            stats.fTootleFastOptimizeTime = timer.GetElapsed();

            stats.nClusters = nNumClusters;

            break;

        default:
            // wrong algorithm choice
            break;
    }

    // measure output VCache efficiency
    result = TootleMeasureCacheEfficiency(pnIB, nFaces, settings.nCacheSize, &stats.fVCacheOut);

    if (result != TOOTLE_OK)
    {
        DisplayTootleErrorMessage(result);
        return 1;
    }

    if (settings.bMeasureOverdraw)
    {
        // measure output overdraw
        timer.Reset();
        result = TootleMeasureOverdraw(pfVB, pnIB, nVertices, nFaces, nStride, pViewpoints, nViewpoints, settings.eWinding,
                                       &stats.fOverdrawOut, &stats.fMaxOverdrawOut);
        stats.fMeasureOverdrawTime = timer.GetElapsed();

        if (result != TOOTLE_OK)
        {
            DisplayTootleErrorMessage(result);
            return 1;
        }
    }

    //-----------------------------------------------------------------------------------------------------
    // PERFORM VERTEX MEMORY OPTIMIZATION (rearrange memory layout for vertices based on the final indices
    //  to exploit vertex cache prefetch).
    //  We want to optimize the vertex memory locations based on the final optimized index buffer that will
    //  be in the output file.
    //  Thus, in this sample code, we recompute a copy of the indices that point to the original vertices
    //  (pnIBTmp) to be passed into the function TootleOptimizeVertexMemory.  If we use the array pnIB, we
    //  will optimize for the wrong result since the array pnIB is based on the rehashed vertex location created
    //  by the function ObjLoader.
    //-----------------------------------------------------------------------------------------------------
    timer.Reset();

    std::vector<unsigned int> pnVertexRemapping;
    unsigned int nReferencedVertices = 0;          // The actual total number of vertices referenced by the indices

    if (settings.bOptimizeVertexMemory)
    {
        std::vector<unsigned int> pnIBTmp;
        pnIBTmp.resize(nFaces * 3);

        // compute the indices to be optimized for (the original pointed by the obj file).
        for (unsigned int i = 0; i < indices.size(); i += 3)
        {
            for (int j = 0; j < 3; j++)
            {
                const ObjVertexFinal& rVertex = objVertices[ pnIB[ i + j ] ];
                pnIBTmp[ i + j ] = rVertex.nVertexIndex - 1; // index is off by 1

                // compute the max vertices
                if (rVertex.nVertexIndex > nReferencedVertices)
                {
                    nReferencedVertices = rVertex.nVertexIndex;
                }
            }
        }

        pnVertexRemapping.resize(nReferencedVertices);

        // For this sample code, we are just going to use vertexRemapping array result.  This is to support general obj
        //  file input and output.
        //  In fact, we are sending the wrong vertex buffer here (it should be based on the original file instead of the
        //  rehashed vertices).  But, it is ok because we do not request the reordered vertex buffer as an output.
        result = TootleOptimizeVertexMemory(pfVB, &pnIBTmp[0], nReferencedVertices, nFaces, nStride, NULL, NULL,
                                            &pnVertexRemapping[0]);

        if (result != TOOTLE_OK)
        {
            DisplayTootleErrorMessage(result);
            return 1;
        }

        stats.fOptimizeVertexMemoryTime = timer.GetElapsed();
    }

    // clean up tootle
    TootleCleanup();

    // print tootle statistics to stdout and stderr
    // display the current test case
    PrintAlgorithm(stderr, settings.eVCacheOptimizer, settings.algorithmChoice, settings.nCacheSize, stats.nClusters);
    PrintAlgorithm(stdout, settings.eVCacheOptimizer, settings.algorithmChoice, settings.nCacheSize, stats.nClusters);

    PrintStats(stdout, &stats);
    PrintStats(stderr, &stats);

    // emit a modified .OBJ file
    std::ifstream inputStream(settings.pMeshName);

    bool bResult;

    if (settings.bOptimizeVertexMemory)
    {
        bResult = EmitModifiedObj(inputStream, std::cout, objVertices, indices, &pnVertexRemapping[0], nReferencedVertices);
    }
    else
    {
        bResult = EmitModifiedObj(inputStream, std::cout, objVertices, indices, NULL, 0);
    }

    if (bResult)
    {
        return 1;
    }

    return 0;
}

