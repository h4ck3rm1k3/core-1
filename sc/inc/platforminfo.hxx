/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef SC_PLATFORM_INFO_HXX
#define SC_PLATFORM_INFO_HXX

#include <vector>

#include <rtl/ustring.hxx>

#include "scdllapi.h"

namespace sc {

struct SC_DLLPUBLIC OpenclDeviceInfo
{
    size_t mnId;
    OUString maName;
    OUString maVendor;
};

struct SC_DLLPUBLIC OpenclPlatformInfo
{
    void* mnId;
    OUString maVendor;
    OUString maName;
    std::vector<OpenclDeviceInfo> maDevices;
};

SC_DLLPUBLIC std::vector<OpenclPlatformInfo> listAllOpenclPlatforms();

}

#endif
