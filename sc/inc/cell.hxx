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

#ifndef SC_CELL_HXX
#define SC_CELL_HXX

#include "scdllapi.h"
#include "global.hxx"

#include "tools/mempool.hxx"

#include <boost/shared_ptr.hpp>

#define USE_MEMPOOL

class ScDocument;
class EditTextObject;
class ScMatrix;
class SvtBroadcaster;
class ScProgress;
class ScPatternAttr;

class ScBaseCell
{
protected:
                    ~ScBaseCell();  // not virtual - not to be called directly.

public:
    explicit        ScBaseCell( CellType eNewType );

    /** Base copy constructor. Does NOT clone cell note or broadcaster! */
                    ScBaseCell( const ScBaseCell& rCell );

    /** Returns a clone of this cell at the same position,
        broadcaster will not be cloned. */
    ScBaseCell*     Clone( ScDocument& rDestDoc, int nCloneFlags = SC_CLONECELL_DEFAULT ) const;

    /** Returns a clone of this cell for the passed document position,
        broadcaster will not be cloned. */
    ScBaseCell*     Clone( ScDocument& rDestDoc, const ScAddress& rDestPos, int nCloneFlags = SC_CLONECELL_DEFAULT ) const;

    inline CellType GetCellType() const { return (CellType)eCellType; }

    /** ScFormulaCell with formula::svEmptyCell result, or ScNoteCell (may have been
        created due to reference to empty cell). */
    bool            HasEmptyData() const;
    bool            HasValueData() const;
    bool            HasStringData() const;
    OUString   GetStringData() const;          // only real strings

private:
    ScBaseCell&     operator=( const ScBaseCell& );

protected:
    sal_uInt8            eCellType;      // enum CellType - sal_uInt8 spart Speicher
};

class ScValueCell : public ScBaseCell
{
public:
#ifdef USE_MEMPOOL
    DECL_FIXEDMEMPOOL_NEWDEL( ScValueCell )
#endif

    explicit        ScValueCell( double fValue );

#if OSL_DEBUG_LEVEL > 0
                    ~ScValueCell();
#endif

    inline void     SetValue( double fValue ) { mfValue = fValue; }
    inline double   GetValue() const { return mfValue; }
    double* GetValuePtr() { return &mfValue; }

private:
    double          mfValue;
};

class ScStringCell : public ScBaseCell
{
public:
#ifdef USE_MEMPOOL
    DECL_FIXEDMEMPOOL_NEWDEL( ScStringCell )
#endif

    explicit        ScStringCell(const OUString& rString);

#if OSL_DEBUG_LEVEL > 0
                    ~ScStringCell();
#endif

    inline void     SetString( const OUString& rString ) { maString = rString; }
    inline const    OUString& GetString() const { return maString; }
    const OUString* GetStringPtr() const { return &maString; }

private:
    OUString   maString;
};

class ScEditCell : public ScBaseCell
{
    EditTextObject* mpData;
    mutable OUString* mpString;        // for faster access to formulas
    ScDocument* mpDoc;           // for EditEngine access with Pool

    void            SetTextObject( const EditTextObject* pObject,
                                    const SfxItemPool* pFromPool );

                    // not implemented
                    ScEditCell( const ScEditCell& );

public:

#ifdef USE_MEMPOOL
    DECL_FIXEDMEMPOOL_NEWDEL( ScEditCell )
#endif

                    ~ScEditCell();              // always because of pData!

    /**
     * Constructor that takes ownership of the passed EditTextObject instance
     * which the caller must not delete afterward!
     *
     * <p>Also ensure that the passed edit text object <i>uses the SfxItemPool
     * instance returned from ScDocument::GetEditPool()</i>.  This is
     * important.</p>
     */
    ScEditCell(EditTextObject* pObject, ScDocument* pDoc);

    /**
     * Constructor.  The caller is responsible for deleting the text object
     * instance passed on to this constructor, since it creates a clone and
     * stores it instead of the original.
     *
     * @param rObject text object to clone from.
     * @param pDoc pointer to the document instance.
     * @param pFromPool pointer to SfxItemPool instance that the new text
     *                  object that is to be stored in the cell instance
     *                  should use.  If it's NULL, it uses the default pool
     *                  for edit cells from the document instance (one
     *                  returned from GetEditPool()).
     */
    ScEditCell(const EditTextObject& rObject, ScDocument* pDoc, const SfxItemPool* pFromPool);
    ScEditCell(const ScEditCell& rCell, ScDocument& rDoc, const ScAddress& rDestPos);

    // for line breaks
    ScEditCell(const OUString& rString, ScDocument* pDoc);

    /**
     * Remove the text data as well as string cache.
     */
    void ClearData();

    /**
     * Set new text data.  This method clones the passed text data and stores
     * the clone; the caller is responsible for deleting the text data
     * instance after the call.
     *
     * @param rObject text object to clone from.
     * @param pFromPool pointer to SfxItemPool instance that the new text
     *                  object that is to be stored in the cell instance
     *                  should use.  If it's NULL, it uses the default pool
     *                  for edit cells from the document instance (one
     *                  returned from GetEditPool()).
     */
    void SetData(const EditTextObject& rObject, const SfxItemPool* pFromPool);

    /**
     * Set new text data. The passed text data instance will be owned by the
     * cell.  The caller must ensure that the text data uses the SfxItemPool
     * instance returned from ScDocument::GetEditPool().
     */
    void SetData(EditTextObject* pObject);

    OUString GetString() const;

    const EditTextObject* GetData() const;

    /** Removes character attribute based on new pattern attributes. */
    void            RemoveCharAttribs( const ScPatternAttr& rAttr );

    /** Update field items if any. */
    void UpdateFields(SCTAB nTab);
};

class ScEditDataArray
{
public:
    class Item
    {
    public:
        explicit Item(SCTAB nTab, SCCOL nCol, SCROW nRow,
                      EditTextObject* pOldData, EditTextObject* pNewData);
        ~Item();

        const EditTextObject* GetOldData() const;
        const EditTextObject* GetNewData() const;
        SCTAB GetTab() const;
        SCCOL GetCol() const;
        SCROW GetRow() const;

    private:
        Item(); // disabled

    private:
        ::boost::shared_ptr<EditTextObject> mpOldData;
        ::boost::shared_ptr<EditTextObject> mpNewData;
        SCTAB mnTab;
        SCCOL mnCol;
        SCROW mnRow;

    };

    ScEditDataArray();
    ~ScEditDataArray();

    void AddItem(SCTAB nTab, SCCOL nCol, SCROW nRow,
                 EditTextObject* pOldData, EditTextObject* pNewData);

    const Item* First();
    const Item* Next();

private:
    ::std::vector<Item>::const_iterator maIter;
    ::std::vector<Item> maArray;
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
