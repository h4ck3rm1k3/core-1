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

#ifndef OOX_DRAWINGML_COLORCHOICECONTEXT_HXX
#define OOX_DRAWINGML_COLORCHOICECONTEXT_HXX

#include "oox/core/contexthandler.hxx"

namespace oox {
namespace drawingml {

class Color;

// ============================================================================

/** Context handler for the different color value elements (a:scrgbClr,
    a:srgbClr, a:hslClr, a:sysClr, a:schemeClr, a:prstClr). */
class ColorValueContext : public ::oox::core::ContextHandler
{
public:
    explicit            ColorValueContext( ::oox::core::ContextHandler& rParent, Color& rColor );

    virtual void SAL_CALL startFastElement(
                            sal_Int32 nElement,
                            const ::com::sun::star::uno::Reference< ::com::sun::star::xml::sax::XFastAttributeList >& rxAttribs )
                        throw (::com::sun::star::xml::sax::SAXException, ::com::sun::star::uno::RuntimeException);

    virtual ::com::sun::star::uno::Reference< ::com::sun::star::xml::sax::XFastContextHandler > SAL_CALL
                        createFastChildContext(
                            sal_Int32 nElement,
                            const ::com::sun::star::uno::Reference< ::com::sun::star::xml::sax::XFastAttributeList >& rxAttribs )
                        throw (::com::sun::star::xml::sax::SAXException, ::com::sun::star::uno::RuntimeException);

private:
    Color&              mrColor;
};

// ============================================================================

/** Context handler for elements that *contain* a color value element
    (a:scrgbClr, a:srgbClr, a:hslClr, a:sysClr, a:schemeClr, a:prstClr). */
class ColorContext : public ::oox::core::ContextHandler
{
public:
    explicit            ColorContext( ::oox::core::ContextHandler& rParent, Color& rColor );

    virtual ::com::sun::star::uno::Reference< ::com::sun::star::xml::sax::XFastContextHandler > SAL_CALL
                        createFastChildContext(
                            sal_Int32 nElement,
                            const ::com::sun::star::uno::Reference< ::com::sun::star::xml::sax::XFastAttributeList >& rxAttribs )
                        throw (::com::sun::star::xml::sax::SAXException, ::com::sun::star::uno::RuntimeException);

private:
    Color&              mrColor;
};

// ============================================================================

} // namespace drawingml
} // namespace oox

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */