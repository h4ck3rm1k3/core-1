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

#ifndef __FRAMEWORK_CLASSES_MENUEXTENSIONSUPPLIER_HXX_
#define __FRAMEWORK_CLASSES_MENUEXTENSIONSUPPLIER_HXX_

#include <rtl/ustring.hxx>
#include <framework/fwedllapi.h>

struct FWE_DLLPUBLIC MenuExtensionItem
{
    rtl::OUString aLabel;
    rtl::OUString aURL;
};

typedef MenuExtensionItem ( *pfunc_setMenuExtensionSupplier)();

namespace framework
{

FWE_DLLPUBLIC MenuExtensionItem SAL_CALL GetMenuExtension();

}

#endif // __FRAMEWORK_CLASSES_MENUEXTENSIONSUPPLIER_HXX_

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */