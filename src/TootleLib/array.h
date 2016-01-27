/************************************************************************************//**
// Copyright (c) 2006-2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
****************************************************************************************/
#ifndef _ARRAY_H_
#define _ARRAY_H_

#include <vector>
#include "error.h"

template <class T>
class Array: public std::vector<T>
{
public:
    Array(void) { ; }
    Array(int size): std::vector<T>(size) { ; }
    int GetSize(void) const { return static_cast<int>(this->size()); }
    int PushBack(const T& value = T())
    {
        try { this->push_back(value); }
        catch (...) { return 0; }

        return 1;
    }
    int Reserve(int size)
    {
        try { this->reserve(static_cast<size_t> (size)); }
        catch (...) { return 0; }

        return 1;
    }
    int Resize(int size)
    {
        try { this->resize(static_cast<size_t> (size)); }
        catch (...) { return 0; }

        return 1;
    }
};

#endif
