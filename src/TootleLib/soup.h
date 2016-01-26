/************************************************************************************//**
// Copyright (c) 2006-2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
****************************************************************************************/
#ifndef SOUP_H
#define SOUP_H

#include "cloud.h"

typedef unsigned int UINT;

class Soup: public Cloud
{
public:

    Soup(void) : pt(NULL)
    {
        r = -1;
    }

    virtual ~Soup()
    {
        if (pt)
        {
            pt->RCDown();
            pt = NULL;
        }
    }

    /// Allocates the internal arrays used by the cloud.
    // This has been moved out of the constructor because allocating memory inside of a constructor causes memory leaks
    // and makes it harder to trap exceptions thrown by new
    virtual bool CreateArrays()
    {
        try
        {
            pt = new RCArray<Triangle>;
        }
        catch (std::bad_alloc&)
        {
            return false;
        }

        return Cloud::CreateArrays();
    }


    class Triangle
    {
    public:
        Triangle(int i0 = 0, int i1 = 0, int i2 = 0)
        {
            vi[0] = i0; vi[1] = i1; vi[2] = i2;
        }
        const int& operator[](int c) const { return vi[c]; }
        int& operator[](int c) { return vi[c]; }
        const int& i(int c) const { return vi[c]; }
        int& i(int c) { return vi[c]; }
        int* i(void) { return vi; }
    private:
        int vi[3];
    };

    Triangle& t(size_t i) { return (*pt)[i]; }
    const Triangle& t(size_t i) const { return (*pt)[i]; }
    RCArray<Triangle>& t(void) { return *pt; }
    const RCArray<Triangle>& t(void) const { return *pt; }
    void t(RCArray<Triangle>& new_t)
    {
        new_t.RCUp(); pt->RCDown(); pt = &new_t;
    }

    int ComputeNormals(bool force = false);
    int ComputeTriNormals(Array<Vector3>& tn);
    int ComputeTriCenters(Array<Vector3>& tc);
    int ComputeResolution(float* resolution, bool force = false);

protected:
    float r;
    RCArray<Triangle>* pt;

private:
	Soup (const Soup&);
	Soup& operator=(const Soup&);

};

/// Helper function which creates a soup from a vertex and index buffer
bool MakeSoup(const void* pVB, const unsigned int* pIB, unsigned int nVertices, unsigned int nFaces, unsigned int nVBStride, Soup* pSoup);

#endif
