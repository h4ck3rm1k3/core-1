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

#ifndef OOX_PPT_SLIDEFRAGMENTHANDLER
#define OOX_PPT_SLIDEFRAGMENTHANDLER

#include <com/sun/star/drawing/XDrawPage.hpp>
#include "oox/helper/propertymap.hxx"
#include "oox/core/fragmenthandler2.hxx"
#include "oox/ppt/slidepersist.hxx"

#include <stack>
#include <vector>
#include <map>

namespace oox { namespace ppt {

class SlideFragmentHandler : public ::oox::core::FragmentHandler2
{
public:
    SlideFragmentHandler( ::oox::core::XmlFilterBase& rFilter, const ::rtl::OUString& rFragmentPath, SlidePersistPtr pPersistPtr, const ShapeLocation eShapeLocation ) throw();
    virtual ~SlideFragmentHandler() throw();

    virtual void finalizeImport();
    virtual ::oox::core::ContextHandlerRef onCreateContext( sal_Int32 aElementToken, const AttributeList& rAttribs );

protected:
    SlidePersistPtr     mpSlidePersistPtr;
    ShapeLocation       meShapeLocation;

private:
    ::rtl::OUString     maSlideName;
    PropertyMap         maSlideProperties;
};

} }

#endif // OOX_PPT_SLIDEFRAGMENTHANDLER

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */