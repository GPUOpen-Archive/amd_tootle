/************************************************************************************//**
// Copyright (c) 2006-2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
****************************************************************************************/
#ifndef MESH_H
#define MESH_H

#include "soup.h"
#include "Timer.h"
#include "error.h"

typedef Array< Array<UINT> > VTArray;

class Mesh: public Soup
{
public:
    Mesh(void) { ; }
    virtual ~Mesh() { ; }
    int ComputeVT(VTArray& vtOut);
    int ComputeAE(const VTArray& vt);
    int ComputeVV(void);


    // across-edge information
    Array<UINT>& ae(int i) { return ae_[i]; }
    const Array<UINT>& ae(int i) const { return ae_[i]; }
    Array< Array<UINT> >& ae(void) { return ae_; }
    const Array< Array<UINT> >& ae(void) const { return ae_; }

    // neighbor vertex information
    Array<UINT>& vv(int i) { return vv_[i]; }
    const Array<UINT>& vv(int i) const { return vv_[i]; }
    Array< Array<UINT> >& vv(void) { return vv_; }
    const Array< Array<UINT> >& vv(void) const { return vv_; }

protected:
    // across edge info (same as structure as a triangle)
    Array< Array<UINT> > ae_;
    // vertex neighboring vertices
    Array< Array<UINT> > vv_;
private:
    // prevent catastrophic copies
    Mesh(const Mesh& other): Soup() { ; }
    Mesh& operator=(const Mesh& other) { return *this; }
};

inline int
Mesh::ComputeVV(void)
{
    Timer time;
    debugf(("Finding vertex neighbors"));

    if (!vv().Resize(v().GetSize()))
    {
        // out of memory
        return 0;
    }

    for (int i = 0; i < t().GetSize(); i++)
    {
        vv(t(i)[0]).PushBack(t(i)[1]);
        vv(t(i)[1]).PushBack(t(i)[0]);
        vv(t(i)[1]).PushBack(t(i)[2]);
        vv(t(i)[2]).PushBack(t(i)[1]);
        vv(t(i)[2]).PushBack(t(i)[0]);
        vv(t(i)[0]).PushBack(t(i)[2]);
    }

    debugf(("Done in %gs", time.GetElapsed()));
    return 1;
}

inline int
Mesh::ComputeVT(VTArray& vtOut)
{
    Timer time;
    debugf(("Finding vertex faces"));

    // get all faces that use each vertex
    if (!vtOut.Resize(v().GetSize()))
    {
        // out of memory
        return 0;
    }

    for (int f = 0; f < t().GetSize(); f++)
    {
        Soup::Triangle& face = t(f);

        for (int i = 0; i < 3; i++)
        {
            vtOut[face[i]].PushBack(f);
        }
    }

    debugf(("Done in %gs", time.GetElapsed()));
    return 1;
}


inline int
Mesh::ComputeAE(const VTArray& vt)
{
    Timer time;
    debugf(("Finding across edge-info"));
    // clean across-edge info
    ae().Resize(t().GetSize());

    // find across-edge info
    for (int f = 0; f < t().GetSize(); f++)
    {
        for (int i = 0; i < 3; i++)
        {
            int in = (i + 1) % 3;
            // get the vertices in edge
            int v = t(f)[i];
            int vn = t(f)[in];

            // for each face that use v
            for (int j = 0; j < vt[v].GetSize(); j++)
            {
                // check if face has vn too and is not f
                int af = vt[v][j];

                if (af != f)
                {
                    for (int k = 0; k < 3; k++)
                    {
                        if (t(af)[k] == vn)
                        {

                            if (!ae(f).PushBack(af))
                            {
                                debugf(("Out of memory"));
                                return 0;
                            }
                        }
                    }
                }
            }
        }
    }

    debugf(("Done in %gs", time.GetElapsed()));
    return 1;
}

#endif