/************************************************************************************//**
// Copyright (c) 2006-2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
****************************************************************************************/
#ifndef RCARRAY_H
#define RCARRAY_H

#include "array.h"

template <class T>
class RCArray: public Array<T>
{
public:
    RCArray(void): count(1) { ; }
    RCArray(int size): Array<T>(size), count(1) { ; }
    RCArray(const RCArray<T>& o): Array<T>(o), count(1) { ; }
    void RCUp(void) { count++; }
    void RCDown(void) { count--; if (count == 0) { delete this; } }
    int RCCount(void) const { return count; }
private:
    int count;
};

#endif
