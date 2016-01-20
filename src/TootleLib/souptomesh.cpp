/************************************************************************************//**
// Copyright (c) 2006-2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
****************************************************************************************/
#include "TootlePCH.h"
#include "souptomesh.h"

int SoupToMesh(Soup* soup, Mesh* mesh)
{

    if (soup->v().GetSize() > 0) { mesh->v(soup->v()); }

    if (soup->n().GetSize() > 0) { mesh->n(soup->n()); }

    if (soup->c().GetSize() > 0) { mesh->c(soup->c()); }

    if (soup->vc().GetSize() > 0) { mesh->vc(soup->vc()); }

    if (soup->t().GetSize() > 0) { mesh->t(soup->t()); }

    mesh->ae().Resize(0);
    mesh->vv().Resize(0);
    return 1;
}