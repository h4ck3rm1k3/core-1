/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */
#ifndef CONNECTIVITY_STDTYPEDEFS_HXX
#define CONNECTIVITY_STDTYPEDEFS_HXX

#include "sal/config.h"

#include <map>
#include <vector>
#include "sal/types.h"

namespace rtl { class OUString; }

namespace connectivity
{
    typedef ::std::vector< ::rtl::OUString>         TStringVector;
    typedef ::std::vector< sal_Int32>               TIntVector;
    typedef ::std::map<sal_Int32,sal_Int32>         TInt2IntMap;
    typedef ::std::map< ::rtl::OUString,sal_Int32>  TString2IntMap;
    typedef ::std::map< sal_Int32,::rtl::OUString>  TInt2StringMap;
}

#endif // CONNECTIVITY_STDTYPEDEFS_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */