/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef SC_FORMULAGROUP_HXX
#define SC_FORMULAGROUP_HXX

#include "address.hxx"
#include "types.hxx"

#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

class ScDocument;
class ScTokenArray;

namespace sc {

struct FormulaGroupContext : boost::noncopyable
{
    typedef std::vector<double> DoubleArrayType;
    typedef boost::ptr_vector<DoubleArrayType> ArrayStoreType;

    ArrayStoreType maArrays;
};

/**
 * All the vectorized formula calculation code should be collected here.
 *
 * Abstract base class for formula group interpreters, and a factory.
 */
class SC_DLLPUBLIC FormulaGroupInterpreter
{
    static FormulaGroupInterpreter *msInstance;
 protected:
    FormulaGroupInterpreter() {}
    virtual ~FormulaGroupInterpreter() {}
 public:
    static FormulaGroupInterpreter *getStatic();

    virtual bool interpret(ScDocument& rDoc, const ScAddress& rTopPos, const ScFormulaCellGroupRef& xGroup, ScTokenArray& rCode) = 0;
};

/// Inherit from this for alternate formula group calculation approaches.
class SC_DLLPUBLIC FormulaGroupInterpreterSoftware : public FormulaGroupInterpreter
{
public:
    FormulaGroupInterpreterSoftware() :
        FormulaGroupInterpreter() {}
    virtual ~FormulaGroupInterpreterSoftware() {}

    virtual bool interpret(ScDocument& rDoc, const ScAddress& rTopPos, const ScFormulaCellGroupRef& xGroup, ScTokenArray& rCode);
};

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
