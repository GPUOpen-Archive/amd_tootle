/************************************************************************************//**
// Copyright (c) 2006-2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
****************************************************************************************/
#ifndef CLOUD_H
#define CLOUD_H

#include "rcarray.h"
#include "vector.h"
#include "color.h"

class Cloud
{
public:
    // default constructor
    Cloud(void) : pv(NULL), pn(NULL), pc(NULL), pvc(NULL)
    {
    }
    // destructor
    ~Cloud()
    {
        if (pv) { pv->RCDown(); }

        if (pn) { pn->RCDown(); }

        if (pc) { pc->RCDown(); }

        if (pvc) { pvc->RCDown(); }

        pv = NULL;
        pn = NULL;
        pc = NULL;
        pvc = NULL;
    }

    /// Allocates the internal arrays used by the cloud.
    // This has been moved out of the constructor because allocating memory inside of a constructor causes memory leaks
    // and makes it harder to trap exceptions thrown by new
    virtual bool CreateArrays()
    {

        try
        {
            pv = new RCArray<Vector3>;
            pn = new RCArray<Vector3>;
            pc = new RCArray<Color>;
            pvc = new RCArray<float>;
        }
        catch (std::bad_alloc&)
        {
            return false;
        }

        return true;
    };

    // vertex access functions
    Vector3& v(int i) const { return (*pv)[i]; }
    Vector3& v(int i) { return (*pv)[i]; }
    // vertex array access functions
    RCArray<Vector3>& v(void) { return *pv; }
    const RCArray<Vector3>& v(void) const { return *pv; }
    void v(RCArray<Vector3>& new_v) { new_v.RCUp(); pv->RCDown(); pv = &new_v; }
    // normal access functions
    const Vector3& n(int i) const { return (*pn)[i]; }
    Vector3& n(int i) { return (*pn)[i]; }
    // normal array access functions
    RCArray<Vector3>& n(void) { return *pn; }
    const RCArray<Vector3>& n(void) const { return *pn; }
    void n(RCArray<Vector3>& new_n) { new_n.RCUp(); pn->RCDown(); pn = &new_n; }
    // color access functions
    Color& c(int i) const { return (*pc)[i]; }
    Color& c(int i) { return (*pc)[i]; }
    // color array access functions
    RCArray<Color>& c(void) { return *pc; }
    const RCArray<Color>& c(void) const { return *pc; }
    void c(RCArray<Color>& new_c) { new_c.RCUp(); pc->RCDown(); pc = &new_c; }
    // vertex confidence access functions
    float& vc(int i) const { return (*pvc)[i]; }
    float& vc(int i) { return (*pvc)[i]; }
    // vertex confidence array access functions
    RCArray<float>& vc(void) { return *pvc; }
    const RCArray<float>& vc(void) const { return *pvc; }
    void vc(RCArray<float>& new_vc)
    { new_vc.RCUp(); pvc->RCDown(); pvc = &new_vc; }
protected:
    RCArray<Vector3>* pv; // vertices
    RCArray<Vector3>* pn; // normals
    RCArray<Color>* pc;   // colors
    RCArray<float>* pvc;  // vertex confidence
private:
    // prevent copy constructors and assignments
	Cloud (const Cloud&);
	Cloud& operator=(const Cloud&);
};

#endif
