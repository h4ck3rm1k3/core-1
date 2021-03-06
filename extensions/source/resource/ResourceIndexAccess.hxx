/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef EXTENSIONS_RESOURCE_RESOURCESTRINGINDEXACCESS_HXX
#define EXTENSIONS_RESOURCE_RESOURCESTRINGINDEXACCESS_HXX


#include <boost/shared_ptr.hpp>
#include <com/sun/star/container/XNameAccess.hpp>
#include <com/sun/star/uno/Reference.hxx>
#include <com/sun/star/uno/Sequence.hxx>
#include <com/sun/star/uno/XInterface.hpp>
#include <cppuhelper/implbase1.hxx>

namespace com { namespace sun { namespace star { namespace uno {
    class XComponentContext;
} } } }
class ResMgr;

namespace extensions { namespace resource
{
    /** This class provides access to tools library text resources */
    class ResourceIndexAccess : public cppu::WeakImplHelper1< ::com::sun::star::container::XNameAccess>
    {
        public:
            /** The ctor takes a sequence with one element: the name of the resource, e.g. svt */
            ResourceIndexAccess(::com::sun::star::uno::Sequence< ::com::sun::star::uno::Any> const& rArgs, ::com::sun::star::uno::Reference< ::com::sun::star::uno::XComponentContext> const&);
            // XNameAccess
            // The XNameAccess provides access to two named elements:
            //    "String" returns a XIndexAccess to String resources
            //    "StringList" returns a XIndexAccess to StringList/StringArray resources
            virtual ::com::sun::star::uno::Any SAL_CALL getByName( const OUString& aName ) throw (::com::sun::star::container::NoSuchElementException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException);
            virtual ::com::sun::star::uno::Sequence< OUString > SAL_CALL getElementNames(  ) throw (::com::sun::star::uno::RuntimeException);
            virtual ::sal_Bool SAL_CALL hasByName( const OUString& aName ) throw (::com::sun::star::uno::RuntimeException);
            // XElementAccess
            virtual ::com::sun::star::uno::Type SAL_CALL getElementType(  ) throw (::com::sun::star::uno::RuntimeException)
                { return ::getCppuType(static_cast< ::com::sun::star::uno::Reference< ::com::sun::star::uno::XInterface>*>(0)); };
            virtual ::sal_Bool SAL_CALL hasElements(  ) throw (::com::sun::star::uno::RuntimeException)
                { return static_cast<bool>(m_pResMgr.get()); };

        private:
            // m_pResMgr should never be NULL
            const ::boost::shared_ptr<ResMgr> m_pResMgr;
    };
}}

::com::sun::star::uno::Reference< ::com::sun::star::uno::XInterface> initResourceIndexAccess(::extensions::resource::ResourceIndexAccess*);

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
