/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef CHART2_DUMMY_XSHAPE_HXX
#define CHART2_DUMMY_XSHAPE_HXX

#include <cppuhelper/implbase6.hxx>

#include <com/sun/star/drawing/XShape.hpp>
#include <com/sun/star/drawing/XShapes.hpp>
#include <com/sun/star/beans/XMultiPropertySet.hpp>
#include <com/sun/star/container/XNamed.hpp>
#include <com/sun/star/container/XChild.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/awt/Point.hpp>
#include <com/sun/star/awt/Size.hpp>
#include <com/sun/star/uno/Type.h>
#include <com/sun/star/uno/Any.h>
#include <com/sun/star/lang/IndexOutOfBoundsException.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/drawing/CircleKind.hpp>
#include <com/sun/star/drawing/DoubleSequence.hpp>
#include <com/sun/star/drawing/FlagSequence.hpp>
#include <com/sun/star/drawing/FillStyle.hpp>
#include <com/sun/star/drawing/LineStyle.hpp>
#include <com/sun/star/drawing/NormalsKind.hpp>
#include <com/sun/star/drawing/PointSequence.hpp>
#include <com/sun/star/drawing/PolygonKind.hpp>
#include <com/sun/star/drawing/PolyPolygonBezierCoords.hpp>
#include <com/sun/star/drawing/ProjectionMode.hpp>
#include <com/sun/star/drawing/ShadeMode.hpp>
#include <com/sun/star/drawing/TextFitToSizeType.hpp>
#include <com/sun/star/drawing/TextHorizontalAdjust.hpp>
#include <com/sun/star/drawing/TextureProjectionMode.hpp>
#include <com/sun/star/drawing/TextVerticalAdjust.hpp>
#include <com/sun/star/text/XText.hpp>
#include <com/sun/star/uno/Any.hxx>
#include <com/sun/star/drawing/PolyPolygonShape3D.hpp>
#include <com/sun/star/drawing/Direction3D.hpp>
#include <com/sun/star/drawing/Position3D.hpp>
#include <com/sun/star/graphic/XGraphic.hpp>
#include <com/sun/star/drawing/HomogenMatrix.hpp>
#include <com/sun/star/drawing/PointSequenceSequence.hpp>

#include "PropertyMapper.hxx"
#include "VLineProperties.hxx"
#include "Stripe.hxx"

#include <rtl/ustring.hxx>

#include <vector>
#include <map>
#include <boost/scoped_ptr.hpp>

#if defined( _WIN32 )
#include "prewin.h"
#include "windows.h"
#include "postwin.h"
#endif


#include <GL/gl.h>
#include <GL/glu.h>
#include <vcl/window.hxx>
#include <vcl/syschild.hxx>
#include <vcl/sysdata.hxx>

#if defined( _WIN32 )
    #include <GL/glu.h>
    #include <GL/glext.h>
    #include <GL/wglext.h>
#elif defined( MACOSX )
    #include "premac.h"
    #include <Cocoa/Cocoa.h>
    #include "postmac.h"
#elif defined( UNX )
    #include <GL/glu.h>
    #include <GL/glext.h>

    namespace unx
    {
        #include <X11/keysym.h>
        #include <X11/X.h>
        #define GLX_GLXEXT_PROTOTYPES 1
        #include <GL/glx.h>
        #include <GL/glxext.h>
    }
#endif
//[mod] by gaowei

#include "OpenGLRender.hxx"

//[mod] by gaowei end


class SystemWindow;
class SystemChildWindow;

using namespace com::sun::star;


namespace chart {

namespace dummy {

class DummyChart;

struct OpenglContext;

class DummyXShape : public cppu::WeakAggImplHelper6<
                    ::com::sun::star::drawing::XShape,
                    com::sun::star::beans::XPropertySet,
                    com::sun::star::beans::XMultiPropertySet,
                    com::sun::star::container::XNamed,
                    com::sun::star::container::XChild,
                    com::sun::star::lang::XServiceInfo >
{
public:
    DummyXShape();

    // XNamed
    virtual OUString SAL_CALL getName(  ) throw(::com::sun::star::uno::RuntimeException);
    virtual void SAL_CALL setName( const OUString& aName ) throw(::com::sun::star::uno::RuntimeException);

    // XShape
    virtual ::com::sun::star::awt::Point SAL_CALL getPosition() throw(::com::sun::star::uno::RuntimeException);
    virtual void SAL_CALL setPosition( const ::com::sun::star::awt::Point& aPosition ) throw(::com::sun::star::uno::RuntimeException);
    virtual ::com::sun::star::awt::Size SAL_CALL getSize() throw(::com::sun::star::uno::RuntimeException);
    virtual void SAL_CALL setSize( const ::com::sun::star::awt::Size& aSize ) throw(::com::sun::star::beans::PropertyVetoException, ::com::sun::star::uno::RuntimeException);

    // XShapeDescriptor
    virtual OUString SAL_CALL getShapeType() throw(::com::sun::star::uno::RuntimeException);

    // XPropertySet
    virtual ::com::sun::star::uno::Reference< ::com::sun::star::beans::XPropertySetInfo > SAL_CALL getPropertySetInfo(  ) throw(::com::sun::star::uno::RuntimeException);
    virtual void SAL_CALL setPropertyValue( const OUString& aPropertyName, const ::com::sun::star::uno::Any& aValue ) throw(::com::sun::star::beans::UnknownPropertyException, ::com::sun::star::beans::PropertyVetoException, ::com::sun::star::lang::IllegalArgumentException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException);
    virtual ::com::sun::star::uno::Any SAL_CALL getPropertyValue( const OUString& PropertyName ) throw(::com::sun::star::beans::UnknownPropertyException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException);
    virtual void SAL_CALL addPropertyChangeListener( const OUString& aPropertyName, const ::com::sun::star::uno::Reference< ::com::sun::star::beans::XPropertyChangeListener >& xListener ) throw(::com::sun::star::beans::UnknownPropertyException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException);
    virtual void SAL_CALL removePropertyChangeListener( const OUString& aPropertyName, const ::com::sun::star::uno::Reference< ::com::sun::star::beans::XPropertyChangeListener >& aListener ) throw(::com::sun::star::beans::UnknownPropertyException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException);
    virtual void SAL_CALL addVetoableChangeListener( const OUString& PropertyName, const ::com::sun::star::uno::Reference< ::com::sun::star::beans::XVetoableChangeListener >& aListener ) throw(::com::sun::star::beans::UnknownPropertyException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException);
    virtual void SAL_CALL removeVetoableChangeListener( const OUString& PropertyName, const ::com::sun::star::uno::Reference< ::com::sun::star::beans::XVetoableChangeListener >& aListener ) throw(::com::sun::star::beans::UnknownPropertyException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException);

    // XMultiPropertySet
    virtual void SAL_CALL setPropertyValues( const ::com::sun::star::uno::Sequence< OUString >& aPropertyNames,
            const ::com::sun::star::uno::Sequence< ::com::sun::star::uno::Any >& aValues )
        throw (::com::sun::star::beans::PropertyVetoException, ::com::sun::star::lang::IllegalArgumentException,
                ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException);

    virtual ::com::sun::star::uno::Sequence< ::com::sun::star::uno::Any > SAL_CALL getPropertyValues(
            const ::com::sun::star::uno::Sequence< OUString >& aPropertyNames )
        throw (::com::sun::star::uno::RuntimeException);

    virtual void SAL_CALL addPropertiesChangeListener( const ::com::sun::star::uno::Sequence< OUString >& aPropertyNames, const ::com::sun::star::uno::Reference< ::com::sun::star::beans::XPropertiesChangeListener >& xListener ) throw (::com::sun::star::uno::RuntimeException);

    virtual void SAL_CALL removePropertiesChangeListener( const ::com::sun::star::uno::Reference< ::com::sun::star::beans::XPropertiesChangeListener >& xListener ) throw (::com::sun::star::uno::RuntimeException);

    virtual void SAL_CALL firePropertiesChangeEvent( const ::com::sun::star::uno::Sequence< OUString >& aPropertyNames,
            const ::com::sun::star::uno::Reference< ::com::sun::star::beans::XPropertiesChangeListener >& xListener )
        throw (::com::sun::star::uno::RuntimeException);


    // XChild
    virtual ::com::sun::star::uno::Reference< ::com::sun::star::uno::XInterface > SAL_CALL getParent(  ) throw(::com::sun::star::uno::RuntimeException);
    virtual void SAL_CALL setParent( const ::com::sun::star::uno::Reference< ::com::sun::star::uno::XInterface >& Parent ) throw(::com::sun::star::lang::NoSupportException, ::com::sun::star::uno::RuntimeException);

    // XServiceInfo
    virtual OUString SAL_CALL getImplementationName() throw(::com::sun::star::uno::RuntimeException);
    virtual ::com::sun::star::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames() throw(::com::sun::star::uno::RuntimeException);
    virtual sal_Bool SAL_CALL supportsService( const OUString& ServiceName ) throw (::com::sun::star::uno::RuntimeException);

    virtual DummyChart* getRootShape();

protected:

    std::map<OUString, uno::Any> maProperties;

private:
    OUString maName;
    com::sun::star::awt::Point maPosition;
    com::sun::star::awt::Size maSize;

    com::sun::star::uno::Reference< com::sun::star::uno::XInterface > mxParent;
    DummyXShape* mpParent;

};

class DummyCube : public DummyXShape
{
public:
    DummyCube(const drawing::Position3D &rPos, const drawing::Direction3D& rSize,
            sal_Int32 nRotateZAngleHundredthDegree, const uno::Reference< beans::XPropertySet > xPropSet,
            const tPropertyNameMap& rPropertyNameMap, bool bRounded );

private:
    sal_Int32 mnRotateZAngleHundredthDegree;
    bool mbRounded;
};

class DummyCylinder : public DummyXShape
{
public:
    DummyCylinder(const drawing::Position3D&, const drawing::Direction3D& rSize,
            sal_Int32 nRotateZAngleHundredthDegree );
private:
    sal_Int32 mnRotateZAngleHundredthDegree;
};

class DummyPyramid : public DummyXShape
{
public:
    DummyPyramid(const drawing::Position3D& rPosition, const drawing::Direction3D& rSize,
            double fTopHeight, bool bRotateZ, uno::Reference< beans::XPropertySet > xPropSet,
            const tPropertyNameMap& rPropertyNameMap );

private:
    double mfTopHeight;
    bool mbRotateZ;
};

class DummyCone : public DummyXShape
{
public:
    DummyCone(const drawing::Position3D& rPosition, const drawing::Direction3D& rSize,
            double fTopHeight, sal_Int32 nRotateZAngleHundredthDegree);

private:
    sal_Int32 mnRotateZAngleHundredthDegree;
    double mfTopHeight;
};

class DummyPieSegment2D : public DummyXShape
{
public:
    DummyPieSegment2D(double fUnitCircleStartAngleDegree, double fUnitCircleWidthAngleDegree,
            double fUnitCircleInnerRadius, double fUnitCircleOuterRadius,
            const drawing::Direction3D& rOffset, const drawing::HomogenMatrix& rUnitCircleToScene);

private:
    double mfUnitCircleStartAngleDegree;
    double mfUnitCircleWidthAngleDegree;
    double mfUnitCircleInnerRadius;
    double mfUnitCircleOuterRadius;

    drawing::Direction3D maOffset;
    drawing::HomogenMatrix maUnitCircleToScene;
};

class DummyPieSegment : public DummyXShape
{
public:
    DummyPieSegment(double fUnitCircleStartAngleDegree, double fUnitCircleWidthAngleDegree,
            double fUnitCircleInnerRadius, double fUnitCircleOuterRadius,
            const drawing::Direction3D& rOffset, const drawing::HomogenMatrix& rUnitCircleToScene,
            double fDepth);

private:
    double mfUnitCircleStartAngleDegree;
    double mfUnitCircleWidthAngleDegree;
    double mfUnitCircleInnerRadius;
    double mfUnitCircleOuterRadius;

    drawing::Direction3D maOffset;
    drawing::HomogenMatrix maUnitCircleToScene;

    double mfDepth;
};

class DummyStripe : public DummyXShape
{
public:
    DummyStripe(const Stripe& rStripe, uno::Reference< beans::XPropertySet > xPropSet,
            const tPropertyNameMap& rPropertyNameMap, sal_Bool bDoubleSided,
            short nRotatedTexture, bool bFlatNormals );

private:
    Stripe maStripe;

    bool mbDoubleSided;
    short mnRotatedTexture;
    bool mbFlatNormals;
};

class DummyArea3D : public DummyXShape
{
public:
    DummyArea3D(const drawing::PolyPolygonShape3D& rShape, double fDepth);

private:
    double mfDepth;
    drawing::PolyPolygonShape3D maShapes;
};

class DummyArea2D : public DummyXShape
{
public:
    DummyArea2D(const drawing::PolyPolygonShape3D& rShape);

private:
    drawing::PolyPolygonShape3D maShapes;
};

class DummySymbol2D : public DummyXShape
{
public:
    DummySymbol2D(const drawing::Position3D& rPosition, const drawing::Direction3D& rSize,
            sal_Int32 nStandardSymbol, sal_Int32 nBorderColor, sal_Int32 nFillColor);

private:
    sal_Int32 mnStandardSymbol;
};

class DummyGraphic2D : public DummyXShape
{
public:
    DummyGraphic2D(const drawing::Position3D& rPosition, const drawing::Direction3D& rSize,
            const uno::Reference< graphic::XGraphic > xGraphic );

private:
    uno::Reference< graphic::XGraphic > mxGraphic;
};

class DummyCircle : public DummyXShape
{
public:
    DummyCircle(const awt::Point& rPosition, const awt::Size& rSize);

};

class DummyLine3D : public DummyXShape
{
public:
    DummyLine3D(const drawing::PolyPolygonShape3D& rPoints, const VLineProperties& rProperties);

private:
    drawing::PolyPolygonShape3D maPoints;
};

class DummyLine2D : public DummyXShape
{
public:
    DummyLine2D(const drawing::PointSequenceSequence& rPoints, const VLineProperties* pProperties);
    DummyLine2D(const awt::Size& rSize, const awt::Point& rPosition);

private:
    drawing::PointSequenceSequence maPoints;
};

class DummyRectangle : public DummyXShape
{
public:
    DummyRectangle();
    DummyRectangle(const awt::Size& rSize);
    DummyRectangle(const awt::Size& rSize, const awt::Point& rPoint, const tNameSequence& rNames,
            const tAnySequence& rValues );
};

class DummyText : public DummyXShape
{
public:
    DummyText(const OUString& rText, const tNameSequence& rNames,
            const tAnySequence& rValues, const uno::Any& rTrans );

private:
    OUString maText;
    uno::Any maTrans;
};


class DummyXShapes : public DummyXShape, public com::sun::star::drawing::XShapes
{
public:

    virtual ::com::sun::star::uno::Any SAL_CALL queryInterface( const ::com::sun::star::uno::Type & rType ) throw(::com::sun::star::uno::RuntimeException);
    virtual com::sun::star::uno::Any SAL_CALL queryAggregation( const com::sun::star::uno::Type& rType ) throw(::com::sun::star::uno::RuntimeException);
    virtual void SAL_CALL acquire() throw();
    virtual void SAL_CALL release() throw();

    // XShapes
    virtual void SAL_CALL add( const ::com::sun::star::uno::Reference< ::com::sun::star::drawing::XShape >& xShape ) throw(::com::sun::star::uno::RuntimeException);
    virtual void SAL_CALL remove( const ::com::sun::star::uno::Reference< ::com::sun::star::drawing::XShape >& xShape ) throw(::com::sun::star::uno::RuntimeException);

    // XElementAccess
    virtual ::com::sun::star::uno::Type SAL_CALL getElementType() throw(::com::sun::star::uno::RuntimeException);
    virtual sal_Bool SAL_CALL hasElements() throw(::com::sun::star::uno::RuntimeException);

    // XIndexAccess
    virtual sal_Int32 SAL_CALL getCount() throw(::com::sun::star::uno::RuntimeException) ;
    virtual ::com::sun::star::uno::Any SAL_CALL getByIndex( sal_Int32 Index ) throw(::com::sun::star::lang::IndexOutOfBoundsException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException);

private:
    std::vector<com::sun::star::uno::Reference< com::sun::star::drawing::XShape > > maUNOShapes;
    std::vector<DummyXShape*> maShapes;
};

class DummyChart : public DummyXShapes
{
public:
    DummyChart(com::sun::star::uno::Reference< com::sun::star::drawing::XShape > xDrawPage);
    ~DummyChart();
    virtual DummyChart* getRootShape();

    virtual void SAL_CALL setPosition( const ::com::sun::star::awt::Point& aPosition ) throw(::com::sun::star::uno::RuntimeException);
    virtual void SAL_CALL setSize( const ::com::sun::star::awt::Size& aSize ) throw(::com::sun::star::beans::PropertyVetoException, ::com::sun::star::uno::RuntimeException);

private:

    GLWindow GLWin;    /// Holds the information of our new child window

    void createGLContext();

    bool initWindow();
    bool initOpengl();
    boost::scoped_ptr<Window> mpWindow;
    boost::scoped_ptr<SystemChildWindow> pWindow;
public:
    OpenGLRender m_GLRender;
};

class DummyGroup2D : public DummyXShapes
{
public:
    DummyGroup2D(const OUString& rName);
};

class DummyGroup3D : public DummyXShapes
{
public:
    DummyGroup3D(const OUString& rName);
};

}

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
