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

#include <com/sun/star/io/XStream.hpp>
#include <com/sun/star/lang/Locale.hpp>
#include <tools/urlobj.hxx>
#include <i18nlangtag/mslangid.hxx>
#include <vcl/svapp.hxx>
#include <sot/storinfo.hxx>
#include <svl/fstathelper.hxx>
#include <svtools/helpopt.hxx>
#include <svl/urihelper.hxx>
#include <unotools/charclass.hxx>
#include <com/sun/star/i18n/UnicodeType.hpp>
#include <unotools/collatorwrapper.hxx>
#include <com/sun/star/i18n/CollatorOptions.hpp>
#include <com/sun/star/i18n/UnicodeScript.hpp>
#include <com/sun/star/i18n/OrdinalSuffix.hpp>
#include <unotools/localedatawrapper.hxx>
#include <unotools/transliterationwrapper.hxx>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/io/XActiveDataSource.hpp>
#include <comphelper/processfactory.hxx>
#include <comphelper/storagehelper.hxx>
#include <comphelper/string.hxx>
#include <editeng/editids.hrc>
#include <sot/storage.hxx>
#include <editeng/udlnitem.hxx>
#include <editeng/wghtitem.hxx>
#include <editeng/escapementitem.hxx>
#include <editeng/svxacorr.hxx>
#include <editeng/unolingu.hxx>
#include "vcl/window.hxx"
#include <helpid.hrc>
#include <com/sun/star/xml/sax/InputSource.hpp>
#include <com/sun/star/xml/sax/Parser.hpp>
#include <com/sun/star/xml/sax/Writer.hpp>
#include <unotools/streamwrap.hxx>
#include <SvXMLAutoCorrectImport.hxx>
#include <SvXMLAutoCorrectExport.hxx>
#include <ucbhelper/content.hxx>
#include <com/sun/star/ucb/XCommandEnvironment.hpp>
#include <com/sun/star/ucb/TransferInfo.hpp>
#include <com/sun/star/ucb/NameClash.hpp>
#include <xmloff/xmltoken.hxx>
#include <vcl/help.hxx>

using namespace ::com::sun::star::ucb;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star;
using namespace ::xmloff::token;
using namespace ::rtl;
using namespace ::utl;

static const int C_NONE             = 0x00;
static const int C_FULL_STOP        = 0x01;
static const int C_EXCLAMATION_MARK = 0x02;
static const int C_QUESTION_MARK    = 0x04;
static const int C_ASTERISK         = 0x2A;
static const sal_Unicode cNonBreakingSpace = 0xA0;

static const sal_Char pXMLImplWrdStt_ExcptLstStr[] = "WordExceptList.xml";
static const sal_Char pXMLImplCplStt_ExcptLstStr[] = "SentenceExceptList.xml";
static const sal_Char pXMLImplAutocorr_ListStr[]   = "DocumentList.xml";

static const sal_Char
    /* also at these beginnings - Brackets and all kinds of begin characters */
    sImplSttSkipChars[] = "\"\'([{\x83\x84\x89\x91\x92\x93\x94",
    /* also at these ends - Brackets and all kinds of begin characters */
    sImplEndSkipChars[] = "\"\')]}\x83\x84\x89\x91\x92\x93\x94";

// These characters are allowed in words: (for FnCptlSttSntnc)
static const sal_Char sImplWordChars[] = "-'";

OUString EncryptBlockName_Imp(const OUString& rName);

TYPEINIT0(SvxAutoCorrect)

typedef SvxAutoCorrectLanguageLists* SvxAutoCorrectLanguageListsPtr;

static inline int IsWordDelim( const sal_Unicode c )
{
    return ' ' == c || '\t' == c || 0x0a == c ||
            cNonBreakingSpace == c || 0x2011 == c || 0x1 == c;
}

static inline int IsLowerLetter( sal_Int32 nCharType )
{
    return CharClass::isLetterType( nCharType ) &&
            0 == ( ::com::sun::star::i18n::KCharacterType::UPPER & nCharType);
}

static inline int IsUpperLetter( sal_Int32 nCharType )
{
    return CharClass::isLetterType( nCharType ) &&
            0 == ( ::com::sun::star::i18n::KCharacterType::LOWER & nCharType);
}

bool lcl_IsUnsupportedUnicodeChar( CharClass& rCC, const OUString& rTxt,
                                   xub_StrLen nStt, xub_StrLen nEnd )
{
    for( ; nStt < nEnd; ++nStt )
    {
        short nScript = rCC.getScript( rTxt, nStt );
        switch( nScript )
        {
            case ::com::sun::star::i18n::UnicodeScript_kCJKRadicalsSupplement:
            case ::com::sun::star::i18n::UnicodeScript_kHangulJamo:
            case ::com::sun::star::i18n::UnicodeScript_kCJKSymbolPunctuation:
            case ::com::sun::star::i18n::UnicodeScript_kHiragana:
            case ::com::sun::star::i18n::UnicodeScript_kKatakana:
            case ::com::sun::star::i18n::UnicodeScript_kHangulCompatibilityJamo:
            case ::com::sun::star::i18n::UnicodeScript_kEnclosedCJKLetterMonth:
            case ::com::sun::star::i18n::UnicodeScript_kCJKCompatibility:
            case ::com::sun::star::i18n::UnicodeScript_k_CJKUnifiedIdeographsExtensionA:
            case ::com::sun::star::i18n::UnicodeScript_kCJKUnifiedIdeograph:
            case ::com::sun::star::i18n::UnicodeScript_kHangulSyllable:
            case ::com::sun::star::i18n::UnicodeScript_kCJKCompatibilityIdeograph:
            case ::com::sun::star::i18n::UnicodeScript_kHalfwidthFullwidthForm:
                return true;
            default: ; //do nothing
        }
    }
    return false;
}

static sal_Bool lcl_IsSymbolChar( CharClass& rCC, const OUString& rTxt,
                                  xub_StrLen nStt, xub_StrLen nEnd )
{
    for( ; nStt < nEnd; ++nStt )
    {
        if( ::com::sun::star::i18n::UnicodeType::PRIVATE_USE ==
                rCC.getType( rTxt, nStt ))
            return sal_True;
    }
    return sal_False;
}

static sal_Bool lcl_IsInAsciiArr( const sal_Char* pArr, const sal_Unicode c )
{
    sal_Bool bRet = sal_False;
    for( ; *pArr; ++pArr )
        if( *pArr == c )
        {
            bRet = sal_True;
            break;
        }
    return bRet;
}

SvxAutoCorrDoc::~SvxAutoCorrDoc()
{
}

// Called by the functions:
//  - FnCptlSttWrd
//  - FnCptlSttSntnc
// after the exchange of characters. Then the words, if necessary, can be inserted
// into the exception list.
void SvxAutoCorrDoc::SaveCpltSttWord( sal_uLong, xub_StrLen, const OUString&,
                                        sal_Unicode )
{
}

LanguageType SvxAutoCorrDoc::GetLanguage( xub_StrLen , sal_Bool ) const
{
    return LANGUAGE_SYSTEM;
}

static const LanguageTag& GetAppLang()
{
    return Application::GetSettings().GetLanguageTag();
}
static LocaleDataWrapper& GetLocaleDataWrapper( sal_uInt16 nLang )
{
    static LocaleDataWrapper aLclDtWrp( GetAppLang() );
    LanguageTag aLcl( nLang );
    const LanguageTag& rLcl = aLclDtWrp.getLoadedLanguageTag();
    if( aLcl != rLcl )
        aLclDtWrp.setLanguageTag( aLcl );
    return aLclDtWrp;
}
static TransliterationWrapper& GetIgnoreTranslWrapper()
{
    static int bIsInit = 0;
    static TransliterationWrapper aWrp( ::comphelper::getProcessComponentContext(),
                ::com::sun::star::i18n::TransliterationModules_IGNORE_KANA |
                ::com::sun::star::i18n::TransliterationModules_IGNORE_WIDTH );
    if( !bIsInit )
    {
        aWrp.loadModuleIfNeeded( GetAppLang().getLanguageType() );
        bIsInit = 1;
    }
    return aWrp;
}
static CollatorWrapper& GetCollatorWrapper()
{
    static int bIsInit = 0;
    static CollatorWrapper aCollWrp( ::comphelper::getProcessComponentContext() );
    if( !bIsInit )
    {
        aCollWrp.loadDefaultCollator( GetAppLang().getLocale(), 0 );
        bIsInit = 1;
    }
    return aCollWrp;
}

static void lcl_ClearTable(boost::ptr_map<LanguageTag, SvxAutoCorrectLanguageLists>& rLangTable)
{
    rLangTable.clear();
}

sal_Bool SvxAutoCorrect::IsAutoCorrectChar( sal_Unicode cChar )
{
    return  cChar == '\0' || cChar == '\t' || cChar == 0x0a ||
            cChar == ' '  || cChar == '\'' || cChar == '\"' ||
            cChar == '*'  || cChar == '_'  || cChar == '%' ||
            cChar == '.'  || cChar == ','  || cChar == ';' ||
            cChar == ':'  || cChar == '?' || cChar == '!' ||
            cChar == '/'  || cChar == '-';
}

sal_Bool SvxAutoCorrect::NeedsHardspaceAutocorr( sal_Unicode cChar )
{
    return cChar == '%' || cChar == ';' || cChar == ':'  || cChar == '?' || cChar == '!' ||
        cChar == '/' /*case for the urls exception*/;
}

long SvxAutoCorrect::GetDefaultFlags()
{
    long nRet = Autocorrect
                    | CptlSttSntnc
                    | CptlSttWrd
                    | ChgOrdinalNumber
                    | ChgToEnEmDash
                    | AddNonBrkSpace
                    | ChgWeightUnderl
                    | SetINetAttr
                    | ChgQuotes
                    | SaveWordCplSttLst
                    | SaveWordWrdSttLst
                    | CorrectCapsLock;
    LanguageType eLang = GetAppLang().getLanguageType();
    switch( eLang )
    {
    case LANGUAGE_ENGLISH:
    case LANGUAGE_ENGLISH_US:
    case LANGUAGE_ENGLISH_UK:
    case LANGUAGE_ENGLISH_AUS:
    case LANGUAGE_ENGLISH_CAN:
    case LANGUAGE_ENGLISH_NZ:
    case LANGUAGE_ENGLISH_EIRE:
    case LANGUAGE_ENGLISH_SAFRICA:
    case LANGUAGE_ENGLISH_JAMAICA:
    case LANGUAGE_ENGLISH_CARRIBEAN:
        nRet &= ~(ChgQuotes|ChgSglQuotes);
        break;
    }
    return nRet;
}


SvxAutoCorrect::SvxAutoCorrect( const OUString& rShareAutocorrFile,
                                const OUString& rUserAutocorrFile )
    : sShareAutoCorrFile( rShareAutocorrFile ),
    sUserAutoCorrFile( rUserAutocorrFile ),
    pLangTable( new boost::ptr_map<LanguageTag, SvxAutoCorrectLanguageLists> ),
    pCharClass( 0 ), bRunNext( false ), eCharClassLang( LANGUAGE_DONTKNOW ),
    cStartDQuote( 0 ), cEndDQuote( 0 ), cStartSQuote( 0 ), cEndSQuote( 0 )
{
    nFlags = SvxAutoCorrect::GetDefaultFlags();

    cEmDash = 0x2014;
    cEnDash = 0x2013;
}

SvxAutoCorrect::SvxAutoCorrect( const SvxAutoCorrect& rCpy )
:   sShareAutoCorrFile( rCpy.sShareAutoCorrFile ),
    sUserAutoCorrFile( rCpy.sUserAutoCorrFile ),

    aSwFlags( rCpy.aSwFlags ),

    pLangTable( new boost::ptr_map<LanguageTag, SvxAutoCorrectLanguageLists> ),
    pCharClass( 0 ), bRunNext( false ),

    nFlags( rCpy.nFlags & ~(ChgWordLstLoad|CplSttLstLoad|WrdSttLstLoad)),
    cStartDQuote( rCpy.cStartDQuote ), cEndDQuote( rCpy.cEndDQuote ),
    cStartSQuote( rCpy.cStartSQuote ), cEndSQuote( rCpy.cEndSQuote ),
    cEmDash( rCpy.cEmDash ), cEnDash( rCpy.cEnDash )
{
}


SvxAutoCorrect::~SvxAutoCorrect()
{
    lcl_ClearTable(*pLangTable);
    delete pLangTable;
    delete pCharClass;
}

void SvxAutoCorrect::_GetCharClass( LanguageType eLang )
{
    delete pCharClass;
    pCharClass = new CharClass( LanguageTag( eLang));
    eCharClassLang = eLang;
}

void SvxAutoCorrect::SetAutoCorrFlag( long nFlag, sal_Bool bOn )
{
    long nOld = nFlags;
    nFlags = bOn ? nFlags | nFlag
                 : nFlags & ~nFlag;

    if( !bOn )
    {
        if( (nOld & CptlSttSntnc) != (nFlags & CptlSttSntnc) )
            nFlags &= ~CplSttLstLoad;
        if( (nOld & CptlSttWrd) != (nFlags & CptlSttWrd) )
            nFlags &= ~WrdSttLstLoad;
        if( (nOld & Autocorrect) != (nFlags & Autocorrect) )
            nFlags &= ~ChgWordLstLoad;
    }
}


    // Two capital letters at the beginning of word?
sal_Bool SvxAutoCorrect::FnCptlSttWrd( SvxAutoCorrDoc& rDoc, const OUString& rTxt,
                                    xub_StrLen nSttPos, xub_StrLen nEndPos,
                                    LanguageType eLang )
{
    sal_Bool bRet = sal_False;
    CharClass& rCC = GetCharClass( eLang );

    // Delete all non alphanumeric. Test the characters at the beginning/end of
    // the word ( recognizes: "(min.", "/min.", and so on.)
    for( ; nSttPos < nEndPos; ++nSttPos )
        if( rCC.isLetterNumeric( rTxt, nSttPos ))
            break;
    for( ; nSttPos < nEndPos; --nEndPos )
        if( rCC.isLetterNumeric( rTxt, nEndPos - 1 ))
            break;

    // Is the word a compounded word separated by delimiters?
    // If so, keep track of all delimiters so each constituent
    // word can be checked for two initial capital letters.
    std::deque<xub_StrLen> aDelimiters;

    // Always check for two capitals at the beginning
    // of the entire word, so start at nSttPos.
    aDelimiters.push_back(nSttPos);

    // Find all compound word delimiters
    for (xub_StrLen n = nSttPos; n < nEndPos; ++n)
    {
        if (IsAutoCorrectChar(rTxt[ n ]))
        {
            aDelimiters.push_back( n + 1 ); // Get position of char after delimiter
        }
    }

    // Decide where to put the terminating delimiter.
    // If the last AutoCorrect char was a newline, then the AutoCorrect
    // char will not be included in rTxt.
    // If the last AutoCorrect char was not a newline, then the AutoCorrect
    // character will be the last character in rTxt.
    if (!IsAutoCorrectChar(rTxt[nEndPos-1]))
        aDelimiters.push_back(nEndPos);

    // Iterate through the word and all words that compose it.
    // Two capital letters at the beginning of word?
    for (size_t nI = 0; nI < aDelimiters.size() - 1; ++nI)
    {
        nSttPos = aDelimiters[nI];
        nEndPos = aDelimiters[nI + 1];

        if( nSttPos+2 < nEndPos &&
            IsUpperLetter( rCC.getCharacterType( rTxt, nSttPos )) &&
            IsUpperLetter( rCC.getCharacterType( rTxt, ++nSttPos )) &&
            // Is the third character a lower case
            IsLowerLetter( rCC.getCharacterType( rTxt, nSttPos +1 )) &&
            // Do not replace special attributes
            0x1 != rTxt[ nSttPos ] && 0x2 != rTxt[ nSttPos ])
        {
            // test if the word is in an exception list
            OUString sWord( rTxt.copy( nSttPos - 1, nEndPos - nSttPos + 1 ));
            if( !FindInWrdSttExceptList(eLang, sWord) )
            {
                // Check that word isn't correctly spelled before correcting:
                ::com::sun::star::uno::Reference<
                    ::com::sun::star::linguistic2::XSpellChecker1 > xSpeller =
                    SvxGetSpellChecker();
                if( xSpeller->hasLanguage(eLang) )
                {
                    Sequence< ::com::sun::star::beans::PropertyValue > aEmptySeq;
                    if (!xSpeller->spell(sWord, eLang, aEmptySeq).is())
                    {
                        return false;
                    }
                }
                sal_Unicode cSave = rTxt[ nSttPos ];
                OUString sChar( cSave );
                sChar = rCC.lowercase( sChar );
                if( sChar[0] != cSave && rDoc.ReplaceRange( nSttPos, 1, sChar ))
                {
                    if( SaveWordWrdSttLst & nFlags )
                        rDoc.SaveCpltSttWord( CptlSttWrd, nSttPos, sWord, cSave );
                    bRet = sal_True;
                }
            }
        }
    }
    return bRet;
}


sal_Bool SvxAutoCorrect::FnChgOrdinalNumber(
                                SvxAutoCorrDoc& rDoc, const OUString& rTxt,
                                xub_StrLen nSttPos, xub_StrLen nEndPos,
                                LanguageType eLang )
{
// 1st, 2nd, 3rd, 4 - 0th
// 201th or 201st
// 12th or 12nd
    CharClass& rCC = GetCharClass( eLang );
    sal_Bool bChg = sal_False;

    for( ; nSttPos < nEndPos; ++nSttPos )
        if( !lcl_IsInAsciiArr( sImplSttSkipChars, rTxt[ nSttPos ] ))
            break;
    for( ; nSttPos < nEndPos; --nEndPos )
        if( !lcl_IsInAsciiArr( sImplEndSkipChars, rTxt[ nEndPos - 1 ] ))
            break;


    // Get the last number in the string to check
    xub_StrLen nNumEnd = nEndPos;
    bool foundEnd = false;
    bool validNumber = true;
    xub_StrLen i = nEndPos;

    while ( i > nSttPos )
    {
        i--;
        bool isDigit = rCC.isDigit( rTxt, i );
        if ( foundEnd )
            validNumber |= isDigit;

        if ( isDigit && !foundEnd )
        {
            foundEnd = true;
            nNumEnd = i;
        }
    }

    if ( foundEnd && validNumber ) {
        sal_Int32 nNum = rTxt.copy( nSttPos, nNumEnd - nSttPos + 1 ).toInt32( );

        // Check if the characters after that number correspond to the ordinal suffix
        uno::Reference< i18n::XOrdinalSuffix > xOrdSuffix
                = i18n::OrdinalSuffix::create( comphelper::getProcessComponentContext() );

        uno::Sequence< OUString > aSuffixes = xOrdSuffix->getOrdinalSuffix( nNum, rCC.getLanguageTag().getLocale( ) );
        for ( sal_Int32 nSuff = 0; nSuff < aSuffixes.getLength(); nSuff++ )
        {
            OUString sSuffix( aSuffixes[ nSuff ] );
            OUString sEnd = rTxt.copy( nNumEnd + 1, nEndPos - nNumEnd - 1 );

            if ( sSuffix == sEnd )
            {
                // Check if the ordinal suffix has to be set as super script
                if ( rCC.isLetter( sSuffix ) )
                {
                    // Do the change
                    SvxEscapementItem aSvxEscapementItem( DFLT_ESC_AUTO_SUPER,
                                                        DFLT_ESC_PROP, SID_ATTR_CHAR_ESCAPEMENT );
                    rDoc.SetAttr( nNumEnd + 1 , nEndPos,
                                    SID_ATTR_CHAR_ESCAPEMENT,
                                    aSvxEscapementItem);
                }
            }
        }
    }
    return bChg;
}


sal_Bool SvxAutoCorrect::FnChgToEnEmDash(
                                SvxAutoCorrDoc& rDoc, const OUString& rTxt,
                                xub_StrLen nSttPos, xub_StrLen nEndPos,
                                LanguageType eLang )
{
    sal_Bool bRet = sal_False;
    CharClass& rCC = GetCharClass( eLang );
    if (eLang == LANGUAGE_SYSTEM)
        eLang = GetAppLang().getLanguageType();
    bool bAlwaysUseEmDash = (cEmDash && (eLang == LANGUAGE_RUSSIAN || eLang == LANGUAGE_UKRAINIAN));

    // replace " - " or " --" with "enDash"
    if( cEnDash && 1 < nSttPos && 1 <= nEndPos - nSttPos )
    {
        sal_Unicode cCh = rTxt[ nSttPos ];
        if( '-' == cCh )
        {
            if( ' ' == rTxt[ nSttPos-1 ] &&
                '-' == rTxt[ nSttPos+1 ])
            {
                xub_StrLen n;
                for( n = nSttPos+2; n < nEndPos && lcl_IsInAsciiArr(
                            sImplSttSkipChars,(cCh = rTxt[ n ]));
                        ++n )
                    ;

                // found: " --[<AnySttChars>][A-z0-9]
                if( rCC.isLetterNumeric( OUString(cCh) ) )
                {
                    for( n = nSttPos-1; n && lcl_IsInAsciiArr(
                            sImplEndSkipChars,(cCh = rTxt[ --n ])); )
                        ;

                    // found: "[A-z0-9][<AnyEndChars>] --[<AnySttChars>][A-z0-9]
                    if( rCC.isLetterNumeric( OUString(cCh) ))
                    {
                        rDoc.Delete( nSttPos, nSttPos + 2 );
                        rDoc.Insert( nSttPos, bAlwaysUseEmDash ? OUString(cEmDash) : OUString(cEnDash) );
                        bRet = sal_True;
                    }
                }
            }
        }
        else if( 3 < nSttPos &&
                 ' ' == rTxt[ nSttPos-1 ] &&
                 '-' == rTxt[ nSttPos-2 ])
        {
            xub_StrLen n, nLen = 1, nTmpPos = nSttPos - 2;
            if( '-' == ( cCh = rTxt[ nTmpPos-1 ]) )
            {
                --nTmpPos;
                ++nLen;
                cCh = rTxt[ nTmpPos-1 ];
            }
            if( ' ' == cCh )
            {
                for( n = nSttPos; n < nEndPos && lcl_IsInAsciiArr(
                            sImplSttSkipChars,(cCh = rTxt[ n ]));
                        ++n )
                    ;

                // found: " - [<AnySttChars>][A-z0-9]
                if( rCC.isLetterNumeric( OUString(cCh) ) )
                {
                    cCh = ' ';
                    for( n = nTmpPos-1; n && lcl_IsInAsciiArr(
                            sImplEndSkipChars,(cCh = rTxt[ --n ])); )
                            ;
                    // found: "[A-z0-9][<AnyEndChars>] - [<AnySttChars>][A-z0-9]
                    if( rCC.isLetterNumeric( OUString(cCh) ))
                    {
                        rDoc.Delete( nTmpPos, nTmpPos + nLen );
                        rDoc.Insert( nTmpPos, bAlwaysUseEmDash ? OUString(cEmDash) : OUString(cEnDash) );
                        bRet = sal_True;
                    }
                }
            }
        }
    }

    // Replace [A-z0-9]--[A-z0-9] double dash with "emDash" or "enDash".
    // Finnish and Hungarian use enDash instead of emDash.
    bool bEnDash = (eLang == LANGUAGE_HUNGARIAN || eLang == LANGUAGE_FINNISH);
    if( ((cEmDash && !bEnDash) || (cEnDash && bEnDash)) && 4 <= nEndPos - nSttPos )
    {
        OUString sTmp( rTxt.copy( nSttPos, nEndPos - nSttPos ) );
        sal_Int32 nFndPos = sTmp.indexOf("--");
        if( nFndPos != -1 && nFndPos &&
            nFndPos + 2 < sTmp.getLength() &&
            ( rCC.isLetterNumeric( sTmp, nFndPos - 1 ) ||
              lcl_IsInAsciiArr( sImplEndSkipChars, rTxt[ nFndPos - 1 ] )) &&
            ( rCC.isLetterNumeric( sTmp, nFndPos + 2 ) ||
            lcl_IsInAsciiArr( sImplSttSkipChars, rTxt[ nFndPos + 2 ] )))
        {
            nSttPos = nSttPos + nFndPos;
            rDoc.Delete( nSttPos, nSttPos + 2 );
            rDoc.Insert( nSttPos, (bEnDash ? OUString(cEnDash) : OUString(cEmDash)) );
            bRet = sal_True;
        }
    }
    return bRet;
}


sal_Bool SvxAutoCorrect::FnAddNonBrkSpace(
                                SvxAutoCorrDoc& rDoc, const OUString& rTxt,
                                xub_StrLen, xub_StrLen nEndPos,
                                LanguageType eLang )
{
    bool bRet = false;

    CharClass& rCC = GetCharClass( eLang );

    if ( rCC.getLanguageTag().getLanguage() == "fr" )
    {
        bool bFrCA = (rCC.getLanguageTag().getCountry() == "CA");
        OUString allChars = ":;?!%";
        OUString chars( allChars );
        if ( bFrCA )
            chars = ":";

        sal_Unicode cChar = rTxt[ nEndPos ];
        bool bHasSpace = chars.indexOf( cChar ) != -1;
        bool bIsSpecial = allChars.indexOf( cChar ) != -1;
        if ( bIsSpecial )
        {
            // Get the last word delimiter position
            xub_StrLen nSttWdPos = nEndPos;
            bool bWasWordDelim = false;
            while( nSttWdPos && !(bWasWordDelim = IsWordDelim( rTxt[ --nSttWdPos ])))
                ;

            //See if the text is the start of a protocol string, e.g. have text of
            //"http" see if it is the start of "http:" and if so leave it alone
            sal_Int32 nIndex = nSttWdPos + (bWasWordDelim ? 1 : 0);
            sal_Int32 nProtocolLen = nEndPos - nSttWdPos + 1;
            if (nIndex + nProtocolLen <= rTxt.getLength())
            {
                if (INetURLObject::CompareProtocolScheme(rTxt.copy(nIndex, nProtocolLen)) != INET_PROT_NOT_VALID)
                    return sal_False;
            }

            // Check the presence of "://" in the word
            sal_Int32 nStrPos = rTxt.indexOf( "://", nSttWdPos + 1 );
            if ( nStrPos == -1 && nEndPos > 0 )
            {
                // Check the previous char
                sal_Unicode cPrevChar = rTxt[ nEndPos - 1 ];
                if ( ( chars.indexOf( cPrevChar ) == -1 ) && cPrevChar != '\t' )
                {
                    // Remove any previous normal space
                    xub_StrLen nPos = nEndPos - 1;
                    while ( cPrevChar == ' ' || cPrevChar == cNonBreakingSpace )
                    {
                        if ( nPos == 0 ) break;
                        nPos--;
                        cPrevChar = rTxt[ nPos ];
                    }

                    nPos++;
                    if ( nEndPos - nPos > 0 )
                        rDoc.Delete( nPos, nEndPos );

                    // Add the non-breaking space at the end pos
                    if ( bHasSpace )
                        rDoc.Insert( nPos, OUString(cNonBreakingSpace) );
                    bRunNext = true;
                    bRet = true;
                }
                else if ( chars.indexOf( cPrevChar ) != -1 )
                    bRunNext = true;
            }
        }
        else if ( cChar == '/' && nEndPos > 1 && rTxt.getLength() > (nEndPos - 1) )
        {
            // Remove the hardspace right before to avoid formatting URLs
            sal_Unicode cPrevChar = rTxt[ nEndPos - 1 ];
            sal_Unicode cMaybeSpaceChar = rTxt[ nEndPos - 2 ];
            if ( cPrevChar == ':' && cMaybeSpaceChar == cNonBreakingSpace )
            {
                rDoc.Delete( nEndPos - 2, nEndPos - 1 );
                bRet = true;
            }
        }
    }

    return bRet;
}


sal_Bool SvxAutoCorrect::FnSetINetAttr( SvxAutoCorrDoc& rDoc, const OUString& rTxt,
                                    xub_StrLen nSttPos, xub_StrLen nEndPos,
                                    LanguageType eLang )
{
    sal_Int32 nStart(nSttPos);
    sal_Int32 nEnd(nEndPos);

    OUString sURL( URIHelper::FindFirstURLInText( rTxt, nStart, nEnd,
                                                GetCharClass( eLang ) ));
    nSttPos = (xub_StrLen)nStart;
    nEndPos = (xub_StrLen)nEnd;
    sal_Bool bRet = !sURL.isEmpty();
    if( bRet )          // also Attribut setzen:
        rDoc.SetINetAttr( nSttPos, nEndPos, sURL );
    return bRet;
}


sal_Bool SvxAutoCorrect::FnChgWeightUnderl( SvxAutoCorrDoc& rDoc, const OUString& rTxt,
                                        xub_StrLen, xub_StrLen nEndPos,
                                        LanguageType eLang )
{
    // Condition:
    //  at the beginning:   _ or * after Space with the folloeing !Space
    //  at the end:         _ or * before Space (word delimiter?)

    sal_Unicode c, cInsChar = rTxt[ nEndPos ];  // underline or bold
    if( ++nEndPos != rTxt.getLength() &&
        !IsWordDelim( rTxt[ nEndPos ] ) )
        return sal_False;

    --nEndPos;

    sal_Bool bAlphaNum = sal_False;
    xub_StrLen nPos = nEndPos;
    sal_Int32  nFndPos = -1;
    CharClass& rCC = GetCharClass( eLang );

    while( nPos )
    {
        switch( c = rTxt[ --nPos ] )
        {
        case '_':
        case '*':
            if( c == cInsChar )
            {
                if( bAlphaNum && nPos+1 < nEndPos && ( !nPos ||
                    IsWordDelim( rTxt[ nPos-1 ])) &&
                    !IsWordDelim( rTxt[ nPos+1 ]))
                        nFndPos = nPos;
                else
                    // Condition is not satisfied, so cancel
                    nFndPos = -1;
                nPos = 0;
            }
            break;
        default:
            if( !bAlphaNum )
                bAlphaNum = rCC.isLetterNumeric( rTxt, nPos );
        }
    }

    if( -1 != nFndPos )
    {
        // first delete the Character at the end - this allows insertion
        // of an empty hint in SetAttr which would be removed by Delete
        // (fdo#62536, AUTOFMT in Writer)
        rDoc.Delete( nEndPos, nEndPos + 1 );
        rDoc.Delete( nFndPos, nFndPos + 1 );
        // Span the Attribute over the area
        // the end.
        if( '*' == cInsChar )           // Bold
        {
            SvxWeightItem aSvxWeightItem( WEIGHT_BOLD, SID_ATTR_CHAR_WEIGHT );
            rDoc.SetAttr( nFndPos, nEndPos - 1,
                            SID_ATTR_CHAR_WEIGHT,
                            aSvxWeightItem);
        }
        else                            // underline
        {
            SvxUnderlineItem aSvxUnderlineItem( UNDERLINE_SINGLE, SID_ATTR_CHAR_UNDERLINE );
            rDoc.SetAttr( nFndPos, nEndPos - 1,
                            SID_ATTR_CHAR_UNDERLINE,
                            aSvxUnderlineItem);
        }
    }

    return -1 != nFndPos;
}


sal_Bool SvxAutoCorrect::FnCptlSttSntnc( SvxAutoCorrDoc& rDoc,
                                    const OUString& rTxt, sal_Bool bNormalPos,
                                    xub_StrLen nSttPos, xub_StrLen nEndPos,
                                    LanguageType eLang )
{

    if( rTxt.isEmpty() || nEndPos <= nSttPos )
        return sal_False;

    CharClass& rCC = GetCharClass( eLang );
    OUString aText( rTxt );
    const sal_Unicode *pStart = aText.getStr(),
                      *pStr = pStart + nEndPos,
                      *pWordStt = 0,
                      *pDelim = 0;

    sal_Bool bAtStart = sal_False;
    do {
        --pStr;
        if( rCC.isLetter(
                aText, sal::static_int_cast< xub_StrLen >( pStr - pStart ) ) )
        {
            if( !pWordStt )
                pDelim = pStr+1;
            pWordStt = pStr;
        }
        else if( pWordStt &&
                 !rCC.isDigit(
                     aText,
                     sal::static_int_cast< xub_StrLen >( pStr - pStart ) ) )
        {
            if( lcl_IsInAsciiArr( sImplWordChars, *pStr ) &&
                pWordStt - 1 == pStr &&
                // Installation at beginning of paragraph. Replaced < by <= (#i38971#)
                (sal_IntPtr)(pStart + 1) <= (sal_IntPtr)pStr &&
                rCC.isLetter(
                    aText,
                    sal::static_int_cast< xub_StrLen >( pStr-1 - pStart ) ) )
                pWordStt = --pStr;
            else
                break;
        }
    } while( 0 == ( bAtStart = (pStart == pStr)) );

    if (!pWordStt)
        return sal_False;    // no character to be replaced


    if (rCC.isDigit( aText, sal::static_int_cast< xub_StrLen >( pStr - pStart ) ))
        return sal_False; // already ok

    if (IsUpperLetter(
            rCC.getCharacterType(
                aText,
                sal::static_int_cast< xub_StrLen >( pWordStt - pStart ) ) ) )
        return sal_False; // already ok

    //See if the text is the start of a protocol string, e.g. have text of
    //"http" see if it is the start of "http:" and if so leave it alone
    sal_Int32 nIndex = pWordStt - pStart;
    sal_Int32 nProtocolLen = pDelim - pWordStt + 1;
    if (nIndex + nProtocolLen <= rTxt.getLength())
    {
        if (INetURLObject::CompareProtocolScheme(rTxt.copy(nIndex, nProtocolLen)) != INET_PROT_NOT_VALID)
            return sal_False; // already ok
    }

    if (0x1 == *pWordStt || 0x2 == *pWordStt)
        return sal_False; // already ok

    if( *pDelim && 2 >= pDelim - pWordStt &&
        lcl_IsInAsciiArr( ".-)>", *pDelim ) )
        return sal_False;

    if( !bAtStart ) // Still no beginning of a paragraph?
    {
        if ( IsWordDelim( *pStr ) )
        {
            while( 0 == ( bAtStart = (pStart == pStr--) ) && IsWordDelim( *pStr ))
                ;
        }
        // Asian full stop, full width full stop, full width exclamation mark
        // and full width question marks are treated as word delimiters
        else if ( 0x3002 != *pStr && 0xFF0E != *pStr && 0xFF01 != *pStr &&
                  0xFF1F != *pStr )
            return sal_False; // no valid separator -> no replacement
    }

    if( bAtStart )  // at the beginning of a paragraph?
    {
        // Check out the previous paragraph, if it exists.
        // If so, then check to paragraph separator at the end.
        OUString aPrevPara = rDoc.GetPrevPara( bNormalPos );
        if( !aPrevPara.isEmpty() )
        {
            // valid separator -> replace
            OUString sChar( *pWordStt );
            sChar = rCC.titlecase(sChar); //see fdo#56740
            return  !comphelper::string::equals(sChar, *pWordStt) &&
                    rDoc.ReplaceRange( xub_StrLen( pWordStt - pStart ), 1, sChar );
        }

        aText = aPrevPara;
        bAtStart = sal_False;
        pStart = aText.getStr();
        pStr = pStart + aText.getLength();

        do {            // overwrite all blanks
            --pStr;
            if( !IsWordDelim( *pStr ))
                break;
        } while( 0 == ( bAtStart = (pStart == pStr)) );

        if( bAtStart )
            return sal_False;  // no valid separator -> no replacement
    }

    // Found [ \t]+[A-Z0-9]+ until here. Test now on the paragraph separator.
    // all three can happen, but not more than once!
    const sal_Unicode* pExceptStt = 0;
    if( !bAtStart )
    {
        sal_Bool bContinue = sal_True;
        int nFlag = C_NONE;
        do {
            switch( *pStr )
            {
            // Western and Asian full stop
            case '.':
            case 0x3002 :
            case 0xFF0E :
                {
                    if (pStr >= pStart + 2 && *(pStr-2) == '.')
                    {
                        //e.g. text "f.o.o. word": Now currently considering
                        //capitalizing word but second last character of
                        //previous word is a .  So probably last word is an
                        //anagram that ends in . and not truly the end of a
                        //previous sentence, so don't autocapitalize this word
                        return sal_False;
                    }
                    if( nFlag & C_FULL_STOP )
                        return sal_False;  // no valid separator -> no replacement
                    nFlag |= C_FULL_STOP;
                    pExceptStt = pStr;
                }
                break;
            case '!':
            case 0xFF01 :
                {
                    if( nFlag & C_EXCLAMATION_MARK )
                        return sal_False;   // no valid separator -> no replacement
                    nFlag |= C_EXCLAMATION_MARK;
                }
                break;
            case '?':
            case 0xFF1F :
                {
                    if( nFlag & C_QUESTION_MARK)
                        return sal_False;   // no valid separator -> no replacement
                    nFlag |= C_QUESTION_MARK;
                }
                break;
            default:
                if( !nFlag )
                    return sal_False;       // no valid separator -> no replacement
                else
                    bContinue = sal_False;
                break;
            }

            if( bContinue && pStr-- == pStart )
            {
                return sal_False;       // no valid separator -> no replacement
            }
        } while( bContinue );
        if( C_FULL_STOP != nFlag )
            pExceptStt = 0;
    }

    if( 2 > ( pStr - pStart ) )
        return sal_False;

    if( !rCC.isLetterNumeric(
            aText, sal::static_int_cast< xub_StrLen >( pStr-- - pStart ) ) )
    {
        sal_Bool bValid = sal_False, bAlphaFnd = sal_False;
        const sal_Unicode* pTmpStr = pStr;
        while( !bValid )
        {
            if( rCC.isDigit(
                    aText,
                    sal::static_int_cast< xub_StrLen >( pTmpStr - pStart ) ) )
            {
                bValid = sal_True;
                pStr = pTmpStr - 1;
            }
            else if( rCC.isLetter(
                         aText,
                         sal::static_int_cast< xub_StrLen >(
                             pTmpStr - pStart ) ) )
            {
                if( bAlphaFnd )
                {
                    bValid = sal_True;
                    pStr = pTmpStr;
                }
                else
                    bAlphaFnd = sal_True;
            }
            else if( bAlphaFnd || IsWordDelim( *pTmpStr ) )
                break;

            if( pTmpStr == pStart )
                break;

            --pTmpStr;
        }

        if( !bValid )
            return sal_False;       // no valid separator -> no replacement
    }

    sal_Bool bNumericOnly = '0' <= *(pStr+1) && *(pStr+1) <= '9';

    // Search for the beginning of the word
    while( !IsWordDelim( *pStr ))
    {
        if( bNumericOnly &&
            rCC.isLetter(
                aText, sal::static_int_cast< xub_StrLen >( pStr - pStart ) ) )
            bNumericOnly = sal_False;

        if( pStart == pStr )
            break;

        --pStr;
    }

    if( bNumericOnly )      // consists of only numbers, then not
        return sal_False;

    if( IsWordDelim( *pStr ))
        ++pStr;

    OUString sWord;

    // check on the basis of the exception list
    if( pExceptStt )
    {
        sWord = OUString(pStr, pExceptStt - pStr + 1);
        if( FindInCplSttExceptList(eLang, sWord) )
            return sal_False;

        // Delete all non alphanumeric. Test the characters at the
        // beginning/end of the word ( recognizes: "(min.", "/min.", and so on.)
        OUString sTmp( sWord );
        while( !sTmp.isEmpty() &&
                !rCC.isLetterNumeric( sTmp, 0 ) )
            sTmp = sTmp.copy(1);

        // Remove all non alphanumeric characters towards the end up until
        // the last one.
        sal_Int32 nLen = sTmp.getLength();
        while( nLen && !rCC.isLetterNumeric( sTmp, nLen-1 ) )
            --nLen;
        if( nLen + 1 < sTmp.getLength() )
            sTmp = sTmp.copy( 0, nLen + 1 );

        if( !sTmp.isEmpty() && sTmp.getLength() != sWord.getLength() &&
            FindInCplSttExceptList(eLang, sTmp))
            return sal_False;

        if(FindInCplSttExceptList(eLang, sWord, sal_True))
            return sal_False;
    }

    // Ok, then replace
    sal_Unicode cSave = *pWordStt;
    nSttPos = sal::static_int_cast< xub_StrLen >( pWordStt - rTxt.getStr() );
    OUString sChar( cSave );
    sChar = rCC.titlecase(sChar); //see fdo#56740
    sal_Bool bRet = sChar[0] != cSave && rDoc.ReplaceRange( nSttPos, 1, sChar );

    // Parahaps someone wants to have the word
    if( bRet && SaveWordCplSttLst & nFlags )
        rDoc.SaveCpltSttWord( CptlSttSntnc, nSttPos, sWord, cSave );

    return bRet;
}

bool SvxAutoCorrect::FnCorrectCapsLock( SvxAutoCorrDoc& rDoc, const OUString& rTxt,
                                        xub_StrLen nSttPos, xub_StrLen nEndPos,
                                        LanguageType eLang )
{
    if (nEndPos - nSttPos < 2)
        // string must be at least 2-character long.
        return false;

    CharClass& rCC = GetCharClass( eLang );

    // Check the first 2 letters.
    if ( !IsLowerLetter(rCC.getCharacterType(rTxt, nSttPos)) )
        return false;

    if ( !IsUpperLetter(rCC.getCharacterType(rTxt, nSttPos+1)) )
        return false;

    OUString aConverted;
    aConverted += rCC.uppercase(OUString(rTxt[nSttPos]));
    aConverted += rCC.lowercase(OUString(rTxt[nSttPos+1]));

    for( sal_Int32 i = nSttPos+2; i < nEndPos; ++i )
    {
        if ( IsLowerLetter(rCC.getCharacterType(rTxt, i)) )
            // A lowercase letter disqualifies the whole text.
            return false;

        if ( IsUpperLetter(rCC.getCharacterType(rTxt, i)) )
            // Another uppercase letter.  Convert it.
            aConverted += rCC.lowercase(OUString(rTxt[i]));
        else
            // This is not an alphabetic letter.  Leave it as-is.
            aConverted += OUString( rTxt[i] );
    }

    // Replace the word.
    rDoc.Delete(nSttPos, nEndPos);
    rDoc.Insert(nSttPos, aConverted);

    return true;
}


sal_Unicode SvxAutoCorrect::GetQuote( sal_Unicode cInsChar, sal_Bool bSttQuote,
                                        LanguageType eLang ) const
{
    sal_Unicode cRet = bSttQuote ? ( '\"' == cInsChar
                                    ? GetStartDoubleQuote()
                                    : GetStartSingleQuote() )
                                   : ( '\"' == cInsChar
                                    ? GetEndDoubleQuote()
                                    : GetEndSingleQuote() );
    if( !cRet )
    {
        // then through the Language find the right character
        if( LANGUAGE_NONE == eLang )
            cRet = cInsChar;
        else
        {
            LocaleDataWrapper& rLcl = GetLocaleDataWrapper( eLang );
            OUString sRet( bSttQuote
                            ? ( '\"' == cInsChar
                                ? rLcl.getDoubleQuotationMarkStart()
                                : rLcl.getQuotationMarkStart() )
                            : ( '\"' == cInsChar
                                ? rLcl.getDoubleQuotationMarkEnd()
                                : rLcl.getQuotationMarkEnd() ));
            cRet = !sRet.isEmpty() ? sRet[0] : cInsChar;
        }
    }
    return cRet;
}

void SvxAutoCorrect::InsertQuote( SvxAutoCorrDoc& rDoc, xub_StrLen nInsPos,
                                    sal_Unicode cInsChar, sal_Bool bSttQuote,
                                    sal_Bool bIns )
{
    LanguageType eLang = rDoc.GetLanguage( nInsPos, sal_False );
    sal_Unicode cRet = GetQuote( cInsChar, bSttQuote, eLang );

    OUString sChg( cInsChar );
    if( bIns )
        rDoc.Insert( nInsPos, sChg );
    else
        rDoc.Replace( nInsPos, sChg );

    sChg = OUString(cRet);

    if( '\"' == cInsChar )
    {
        if( LANGUAGE_SYSTEM == eLang )
            eLang = GetAppLang().getLanguageType();
        switch( eLang )
        {
        case LANGUAGE_FRENCH:
        case LANGUAGE_FRENCH_BELGIAN:
        case LANGUAGE_FRENCH_CANADIAN:
        case LANGUAGE_FRENCH_SWISS:
        case LANGUAGE_FRENCH_LUXEMBOURG:
            {
                OUString s( cNonBreakingSpace );
                    // UNICODE code for no break space
                if( rDoc.Insert( bSttQuote ? nInsPos+1 : nInsPos, s ))
                {
                    if( !bSttQuote )
                        ++nInsPos;
                }
            }
            break;
        }
    }

    rDoc.Replace( nInsPos, sChg );
}

OUString SvxAutoCorrect::GetQuote( SvxAutoCorrDoc& rDoc, xub_StrLen nInsPos,
                                sal_Unicode cInsChar, sal_Bool bSttQuote )
{
    LanguageType eLang = rDoc.GetLanguage( nInsPos, sal_False );
    sal_Unicode cRet = GetQuote( cInsChar, bSttQuote, eLang );

    OUString sRet = OUString(cRet);

    if( '\"' == cInsChar )
    {
        if( LANGUAGE_SYSTEM == eLang )
            eLang = GetAppLang().getLanguageType();
        switch( eLang )
        {
        case LANGUAGE_FRENCH:
        case LANGUAGE_FRENCH_BELGIAN:
        case LANGUAGE_FRENCH_CANADIAN:
        case LANGUAGE_FRENCH_SWISS:
        case LANGUAGE_FRENCH_LUXEMBOURG:
            if( bSttQuote )
                sRet += " ";
            else
                sRet = " " + sRet;
            break;
        }
    }
    return sRet;
}

sal_uLong
SvxAutoCorrect::DoAutoCorrect( SvxAutoCorrDoc& rDoc, const OUString& rTxt,
                                    xub_StrLen nInsPos, sal_Unicode cChar,
                                    sal_Bool bInsert, Window* pFrameWin )
{
    sal_uLong nRet = 0;
    bool bIsNextRun = bRunNext;
    bRunNext = false;  // if it was set, then it has to be turned off

    do{                                 // only for middle check loop !!
        if( cChar )
        {
            // Prevent double space
            if( nInsPos && ' ' == cChar &&
                IsAutoCorrFlag( IgnoreDoubleSpace ) &&
                ' ' == rTxt[ nInsPos - 1 ])
            {
                nRet = IgnoreDoubleSpace;
                break;
            }

            sal_Bool bSingle = '\'' == cChar;
            sal_Bool bIsReplaceQuote =
                        (IsAutoCorrFlag( ChgQuotes ) && ('\"' == cChar )) ||
                        (IsAutoCorrFlag( ChgSglQuotes ) && bSingle );
            if( bIsReplaceQuote )
            {
                sal_Unicode cPrev;
                sal_Bool bSttQuote = !nInsPos ||
                        IsWordDelim( ( cPrev = rTxt[ nInsPos-1 ])) ||
                        lcl_IsInAsciiArr( "([{", cPrev ) ||
                        ( cEmDash && cEmDash == cPrev ) ||
                        ( cEnDash && cEnDash == cPrev );

                InsertQuote( rDoc, nInsPos, cChar, bSttQuote, bInsert );
                nRet = bSingle ? ChgSglQuotes : ChgQuotes;
                break;
            }

            if( bInsert )
                rDoc.Insert( nInsPos, OUString(cChar) );
            else
                rDoc.Replace( nInsPos, OUString(cChar) );

            // Hardspaces autocorrection
            if ( IsAutoCorrFlag( AddNonBrkSpace ) )
            {
                if ( NeedsHardspaceAutocorr( cChar ) &&
                    FnAddNonBrkSpace( rDoc, rTxt, 0, nInsPos, rDoc.GetLanguage( nInsPos, sal_False ) ) )
                {
                    nRet = AddNonBrkSpace;
                }
                else if ( bIsNextRun && !IsAutoCorrectChar( cChar ) )
                {
                    // Remove the NBSP if it wasn't an autocorrection
                    if ( nInsPos != 0 && NeedsHardspaceAutocorr( rTxt[ nInsPos - 1 ] ) &&
                            cChar != ' ' && cChar != '\t' && cChar != cNonBreakingSpace )
                    {
                        // Look for the last HARD_SPACE
                        sal_Int32 nPos = nInsPos - 1;
                        bool bContinue = true;
                        while ( bContinue )
                        {
                            const sal_Unicode cTmpChar = rTxt[ nPos ];
                            if ( cTmpChar == cNonBreakingSpace )
                            {
                                rDoc.Delete( nPos, nPos + 1 );
                                nRet = AddNonBrkSpace;
                                bContinue = false;
                            }
                            else if ( !NeedsHardspaceAutocorr( cTmpChar ) || nPos == 0 )
                                bContinue = false;
                            nPos--;
                        }
                    }
                }
            }
        }

        if( !nInsPos )
            break;

        sal_Int32 nPos = nInsPos - 1;

        if( IsWordDelim( rTxt[ nPos ]))
            break;

        // Set bold or underline automatically?
        if (('*' == cChar || '_' == cChar) && (nPos+1 < rTxt.getLength()))
        {
            if( IsAutoCorrFlag( ChgWeightUnderl ) &&
                FnChgWeightUnderl( rDoc, rTxt, 0, nPos+1 ) )
                nRet = ChgWeightUnderl;
            break;
        }

        while( nPos && !IsWordDelim( rTxt[ --nPos ]))
            ;

        // Found a Paragraph-start or a Blank, search for the word shortcut in
        // auto.
        sal_Int32 nCapLttrPos = nPos+1;        // on the 1st Character
        if( !nPos && !IsWordDelim( rTxt[ 0 ]))
            --nCapLttrPos;          // Absatz Anfang und kein Blank !

        LanguageType eLang = rDoc.GetLanguage( nCapLttrPos, sal_False );
        if( LANGUAGE_SYSTEM == eLang )
            eLang = MsLangId::getSystemLanguage();
        CharClass& rCC = GetCharClass( eLang );

        // no symbol characters
        if( lcl_IsSymbolChar( rCC, rTxt, nCapLttrPos, nInsPos ))
            break;

        if( IsAutoCorrFlag( Autocorrect ) )
        {
            OUString aPara;
            OUString* pPara = IsAutoCorrFlag(CptlSttSntnc) ? &aPara : 0;

            // since LibO 4.1, '-' is a word separator
            // fdo#67742 avoid "--" to be replaced by "–" if next is "-"
            if( rTxt.getLength() >= 3 &&
                rTxt.match( OUString("---"), rTxt.getLength()-3 ) )
                    break;
            bool bChgWord = rDoc.ChgAutoCorrWord( nCapLttrPos, nInsPos,
                                                    *this, pPara );
            if( !bChgWord )
            {
                sal_Int32 nCapLttrPos1 = nCapLttrPos, nInsPos1 = nInsPos;
                while( nCapLttrPos1 < nInsPos &&
                        lcl_IsInAsciiArr( sImplSttSkipChars, rTxt[ nCapLttrPos1 ] )
                        )
                        ++nCapLttrPos1;
                while( nCapLttrPos1 < nInsPos1 && nInsPos1 &&
                        lcl_IsInAsciiArr( sImplEndSkipChars, rTxt[ nInsPos1-1 ] )
                        )
                        --nInsPos1;

                if( (nCapLttrPos1 != nCapLttrPos || nInsPos1 != nInsPos ) &&
                    nCapLttrPos1 < nInsPos1 &&
                    rDoc.ChgAutoCorrWord( nCapLttrPos1, nInsPos1, *this, pPara ))
                {
                    bChgWord = true;
                    nCapLttrPos = nCapLttrPos1;
                }
            }

            if( bChgWord )
            {
                nRet = Autocorrect;
                if( !aPara.isEmpty() )
                {
                    xub_StrLen nEnd = nCapLttrPos;
                    while( nEnd < aPara.getLength() &&
                            !IsWordDelim( aPara[ nEnd ]))
                        ++nEnd;

                    // Capital letter at beginning of paragraph?
                    if( IsAutoCorrFlag( CptlSttSntnc ) &&
                        FnCptlSttSntnc( rDoc, aPara, sal_False,
                                                nCapLttrPos, nEnd, eLang ) )
                        nRet |= CptlSttSntnc;

                    if( IsAutoCorrFlag( ChgToEnEmDash ) &&
                        FnChgToEnEmDash( rDoc, rTxt, nCapLttrPos, nEnd, eLang ) )
                        nRet |= ChgToEnEmDash;
                }
                break;
            }
        }

        if( ( IsAutoCorrFlag( nRet = ChgOrdinalNumber ) &&
                (nInsPos >= 2 ) &&       // fdo#69762 avoid autocorrect for 2e-3
                ( '-' != cChar || 'E' != toupper(rTxt[nInsPos-1]) || '0' > rTxt[nInsPos-2] || '9' < rTxt[nInsPos-2] ) &&
                FnChgOrdinalNumber( rDoc, rTxt, nCapLttrPos, nInsPos, eLang ) ) ||
            ( IsAutoCorrFlag( nRet = SetINetAttr ) &&
                ( ' ' == cChar || '\t' == cChar || 0x0a == cChar || !cChar ) &&
                FnSetINetAttr( rDoc, rTxt, nCapLttrPos, nInsPos, eLang ) ) )
            ;
        else
        {
            bool bLockKeyOn = pFrameWin && (pFrameWin->GetIndicatorState() & INDICATOR_CAPSLOCK);
            bool bUnsupported = lcl_IsUnsupportedUnicodeChar( rCC, rTxt, nCapLttrPos, nInsPos );

            nRet = 0;
            if ( bLockKeyOn && IsAutoCorrFlag( CorrectCapsLock ) &&
                 FnCorrectCapsLock( rDoc, rTxt, nCapLttrPos, nInsPos, eLang ) )
            {
                // Correct accidental use of cAPS LOCK key (do this only when
                // the caps or shift lock key is pressed).  Turn off the caps
                // lock afterwords.
                nRet |= CorrectCapsLock;
                pFrameWin->SimulateKeyPress( KEY_CAPSLOCK );
            }

            // Capital letter at beginning of paragraph ?
            if( !bUnsupported &&
                IsAutoCorrFlag( CptlSttSntnc ) &&
                FnCptlSttSntnc( rDoc, rTxt, sal_True, nCapLttrPos, nInsPos, eLang ) )
                nRet |= CptlSttSntnc;

            // Two capital letters at beginning of word ??
            if( !bUnsupported &&
                IsAutoCorrFlag( CptlSttWrd ) &&
                FnCptlSttWrd( rDoc, rTxt, nCapLttrPos, nInsPos, eLang ) )
                nRet |= CptlSttWrd;

            if( IsAutoCorrFlag( ChgToEnEmDash ) &&
                FnChgToEnEmDash( rDoc, rTxt, nCapLttrPos, nInsPos, eLang ) )
                nRet |= ChgToEnEmDash;
        }

    } while( false );

    return nRet;
}

SvxAutoCorrectLanguageLists& SvxAutoCorrect::_GetLanguageList(
                                                        LanguageType eLang )
{
    LanguageTag aLanguageTag( eLang);
    if(pLangTable->find(aLanguageTag) == pLangTable->end())
        CreateLanguageFile(aLanguageTag, sal_True);
    return *(pLangTable->find(aLanguageTag)->second);
}

void SvxAutoCorrect::SaveCplSttExceptList( LanguageType eLang )
{
    boost::ptr_map<LanguageTag, SvxAutoCorrectLanguageLists>::iterator nTmpVal = pLangTable->find(LanguageTag(eLang));
    if(nTmpVal != pLangTable->end() && nTmpVal->second)
        nTmpVal->second->SaveCplSttExceptList();
#ifdef DBG_UTIL
    else
    {
        SAL_WARN("editeng", "Save an empty list? ");
    }
#endif
}

void SvxAutoCorrect::SaveWrdSttExceptList(LanguageType eLang)
{
    boost::ptr_map<LanguageTag, SvxAutoCorrectLanguageLists>::iterator nTmpVal = pLangTable->find(LanguageTag(eLang));
    if(nTmpVal != pLangTable->end() && nTmpVal->second)
        nTmpVal->second->SaveWrdSttExceptList();
#ifdef DBG_UTIL
    else
    {
        SAL_WARN("editeng", "Save an empty list? ");
    }
#endif
}

    // Adds a single word. The list will immediately be written to the file!
sal_Bool SvxAutoCorrect::AddCplSttException( const OUString& rNew,
                                        LanguageType eLang )
{
    SvxAutoCorrectLanguageLists* pLists = 0;
    // either the right language is present or it will be this in the general list
    boost::ptr_map<LanguageTag, SvxAutoCorrectLanguageLists>::iterator nTmpVal = pLangTable->find(LanguageTag(eLang));
    if(nTmpVal != pLangTable->end())
        pLists = nTmpVal->second;
    else
    {
        LanguageTag aLangTagUndetermined( LANGUAGE_UNDETERMINED);
        nTmpVal = pLangTable->find(aLangTagUndetermined);
        if(nTmpVal != pLangTable->end())
            pLists = nTmpVal->second;
        else if(CreateLanguageFile(aLangTagUndetermined, sal_True))
            pLists = pLangTable->find(aLangTagUndetermined)->second;
    }
    OSL_ENSURE(pLists, "No auto correction data");
    return pLists->AddToCplSttExceptList(rNew);
}

// Adds a single word. The list will immediately be written to the file!
sal_Bool SvxAutoCorrect::AddWrtSttException( const OUString& rNew,
                                         LanguageType eLang )
{
    SvxAutoCorrectLanguageLists* pLists = 0;
    //either the right language is present or it is set in the general list
    boost::ptr_map<LanguageTag, SvxAutoCorrectLanguageLists>::iterator nTmpVal = pLangTable->find(LanguageTag(eLang));
    if(nTmpVal != pLangTable->end())
        pLists = nTmpVal->second;
    else
    {
        LanguageTag aLangTagUndetermined( LANGUAGE_UNDETERMINED);
        nTmpVal = pLangTable->find(aLangTagUndetermined);
        if(nTmpVal != pLangTable->end())
            pLists = nTmpVal->second;
        else if(CreateLanguageFile(aLangTagUndetermined, sal_True))
            pLists = pLangTable->find(aLangTagUndetermined)->second;
    }
    OSL_ENSURE(pLists, "No auto correction file!");
    return pLists->AddToWrdSttExceptList(rNew);
}

sal_Bool SvxAutoCorrect::GetPrevAutoCorrWord( SvxAutoCorrDoc& rDoc,
                                        const OUString& rTxt, xub_StrLen nPos,
                                        OUString& rWord ) const
{
    if( !nPos )
        return sal_False;

    xub_StrLen nEnde = nPos;

    // it must be followed by a blank or tab!
    if( ( nPos < rTxt.getLength() &&
        !IsWordDelim( rTxt[ nPos ])) ||
        IsWordDelim( rTxt[ --nPos ]))
        return sal_False;

    while( nPos && !IsWordDelim( rTxt[ --nPos ]))
        ;

    // Found a Paragraph-start or a Blank, search for the word shortcut in
    // auto.
    xub_StrLen nCapLttrPos = nPos+1;        // on the 1st Character
    if( !nPos && !IsWordDelim( rTxt[ 0 ]))
        --nCapLttrPos;          // Beginning of pargraph and no Blank!

    while( lcl_IsInAsciiArr( sImplSttSkipChars, rTxt[ nCapLttrPos ]) )
        if( ++nCapLttrPos >= nEnde )
            return sal_False;

    if( 3 > nEnde - nCapLttrPos )
        return sal_False;

    LanguageType eLang = rDoc.GetLanguage( nCapLttrPos, sal_False );
    if( LANGUAGE_SYSTEM == eLang )
        eLang = MsLangId::getSystemLanguage();

    SvxAutoCorrect* pThis = (SvxAutoCorrect*)this;
    CharClass& rCC = pThis->GetCharClass( eLang );

    if( lcl_IsSymbolChar( rCC, rTxt, nCapLttrPos, nEnde ))
        return sal_False;

    rWord = rTxt.copy( nCapLttrPos, nEnde - nCapLttrPos );
    return sal_True;
}

sal_Bool SvxAutoCorrect::CreateLanguageFile( const LanguageTag& rLanguageTag, sal_Bool bNewFile )
{
    OSL_ENSURE(pLangTable->find(rLanguageTag) == pLangTable->end(), "Language already exists ");

    OUString sUserDirFile( GetAutoCorrFileName( rLanguageTag, sal_True, sal_False ));
    OUString sShareDirFile( sUserDirFile );

    SvxAutoCorrectLanguageListsPtr pLists = 0;

    Time nMinTime( 0, 2 ), nAktTime( Time::SYSTEM ), nLastCheckTime( Time::EMPTY );

    std::map<LanguageTag, long>::iterator nFndPos = aLastFileTable.find(rLanguageTag);
    if(nFndPos != aLastFileTable.end() &&
       (nLastCheckTime.SetTime(nFndPos->second), nLastCheckTime < nAktTime) &&
       nAktTime - nLastCheckTime < nMinTime)
    {
        // no need to test the file, because the last check is not older then
        // 2 minutes.
        if( bNewFile )
        {
            sShareDirFile = sUserDirFile;
            pLists = new SvxAutoCorrectLanguageLists( *this, sShareDirFile, sUserDirFile );
            LanguageTag aTmp(rLanguageTag);     // this insert() needs a non-const reference
            pLangTable->insert(aTmp, pLists);
            aLastFileTable.erase(nFndPos);
        }
    }
    else if( ( FStatHelper::IsDocument( sUserDirFile ) ||
                FStatHelper::IsDocument( sShareDirFile =
                              GetAutoCorrFileName( rLanguageTag, sal_False, sal_False ) ) ) ||
        ( sShareDirFile = sUserDirFile, bNewFile ))
    {
        pLists = new SvxAutoCorrectLanguageLists( *this, sShareDirFile, sUserDirFile );
        LanguageTag aTmp(rLanguageTag);     // this insert() needs a non-const reference
        pLangTable->insert(aTmp, pLists);
        if (nFndPos != aLastFileTable.end())
            aLastFileTable.erase(nFndPos);
    }
    else if( !bNewFile )
    {
        aLastFileTable[rLanguageTag] = nAktTime.GetTime();
    }
    return pLists != 0;
}

sal_Bool SvxAutoCorrect::PutText( const OUString& rShort, const OUString& rLong,
                                LanguageType eLang )
{
    LanguageTag aLanguageTag( eLang);
    boost::ptr_map<LanguageTag, SvxAutoCorrectLanguageLists>::iterator nTmpVal = pLangTable->find(aLanguageTag);
    if(nTmpVal != pLangTable->end())
        return nTmpVal->second->PutText(rShort, rLong);
    if(CreateLanguageFile(aLanguageTag))
        return pLangTable->find(aLanguageTag)->second->PutText(rShort, rLong);
    return sal_False;
}

sal_Bool SvxAutoCorrect::MakeCombinedChanges( std::vector<SvxAutocorrWord>& aNewEntries,
                                              std::vector<SvxAutocorrWord>& aDeleteEntries,
                                              LanguageType eLang )
{
    LanguageTag aLanguageTag( eLang);
    boost::ptr_map<LanguageTag, SvxAutoCorrectLanguageLists>::iterator nTmpVal = pLangTable->find(aLanguageTag);
    if(nTmpVal != pLangTable->end())
    {
        return nTmpVal->second->MakeCombinedChanges( aNewEntries, aDeleteEntries );
    }
    else if(CreateLanguageFile( aLanguageTag ))
    {
        return pLangTable->find( aLanguageTag )->second->MakeCombinedChanges( aNewEntries, aDeleteEntries );
    }
    return sal_False;

}


    //  - return the replacement text (only for SWG-Format, all other
    //      can be taken from the word list!)
sal_Bool SvxAutoCorrect::GetLongText( const com::sun::star::uno::Reference < com::sun::star::embed::XStorage >&,
                                      const OUString&, const OUString&, OUString& )
{
    return sal_False;
}

    // Text with attribution (only the SWG - SWG format!)
sal_Bool SvxAutoCorrect::PutText( const com::sun::star::uno::Reference < com::sun::star::embed::XStorage >&,
                                  const OUString&, const OUString&, SfxObjectShell&, OUString& )
{
    return sal_False;
}

OUString EncryptBlockName_Imp(const OUString& rName)
{
    OUStringBuffer aName;
    aName.append('#').append(rName);
    for (sal_Int32 nLen = rName.getLength(), nPos = 1; nPos < nLen; ++nPos)
    {
        if (lcl_IsInAsciiArr( "!/:.\\", aName[nPos]))
            aName[nPos] &= 0x0f;
    }
    return aName.makeStringAndClear();
}

/* This code is copied from SwXMLTextBlocks::GeneratePackageName */
static void GeneratePackageName ( const OUString& rShort, OUString& rPackageName )
{
    OString sByte(OUStringToOString(rShort, RTL_TEXTENCODING_UTF7));
    OUStringBuffer aBuf(OStringToOUString(sByte, RTL_TEXTENCODING_ASCII_US));

    for (sal_Int32 nPos = 0; nPos < aBuf.getLength(); ++nPos)
    {
        switch (aBuf[nPos])
        {
            case '!':
            case '/':
            case ':':
            case '.':
            case '\\':
                aBuf[nPos] = '_';
                break;
            default:
                break;
        }
    }

    rPackageName = aBuf.makeStringAndClear();
}

static const SvxAutocorrWord* lcl_SearchWordsInList(
                SvxAutoCorrectLanguageListsPtr pList, const OUString& rTxt,
                sal_Int32& rStt, sal_Int32 nEndPos)
{
    const SvxAutocorrWordList* pAutoCorrWordList = pList->GetAutocorrWordList();
    return pAutoCorrWordList->SearchWordsInList( rTxt, rStt, nEndPos );
}

// the search for the words in the substitution table
const SvxAutocorrWord* SvxAutoCorrect::SearchWordsInList(
                const OUString& rTxt, sal_Int32& rStt, sal_Int32 nEndPos,
                SvxAutoCorrDoc&, LanguageTag& rLang )
{
    const SvxAutocorrWord* pRet = 0;
    LanguageTag aLanguageTag( rLang);
    if( aLanguageTag.isSystemLocale() )
        aLanguageTag.reset( MsLangId::getSystemLanguage());

    /* TODO-BCP47: this is so ugly, should all maybe be a proper fallback
     * list instead? */

    // First search for eLang, then US-English -> English
    // and last in LANGUAGE_UNDETERMINED
    if(pLangTable->find(aLanguageTag) != pLangTable->end() || CreateLanguageFile(aLanguageTag, sal_False))
    {
        //the language is available - so bring it on
        SvxAutoCorrectLanguageLists* pList = pLangTable->find(aLanguageTag)->second;
        pRet = lcl_SearchWordsInList( pList, rTxt, rStt, nEndPos );
        if( pRet )
        {
            rLang = aLanguageTag;
            return pRet;
        }
    }

    // If it still could not be found here, then keep on searching

    LanguageType eLang = aLanguageTag.getLanguageType();
    LanguageType nTmpKey1 = eLang & 0x7ff, // the main language in many cases DE
                 nTmpKey2 = eLang & 0x3ff; // otherwise for example EN
    if(nTmpKey1 != eLang && (pLangTable->find(aLanguageTag.reset(nTmpKey1)) != pLangTable->end() ||
                CreateLanguageFile(aLanguageTag, sal_False)))
    {
        //the language is available - so bring it on
        SvxAutoCorrectLanguageLists* pList = pLangTable->find(aLanguageTag)->second;
        pRet = lcl_SearchWordsInList( pList, rTxt, rStt, nEndPos );
        if( pRet )
        {
            rLang = aLanguageTag;
            return pRet;
        }
    }

    if(nTmpKey2 != eLang && (pLangTable->find(aLanguageTag.reset(nTmpKey2)) != pLangTable->end() ||
                CreateLanguageFile(aLanguageTag, sal_False)))
    {
        //the language is available - so bring it on
        SvxAutoCorrectLanguageLists* pList = pLangTable->find(aLanguageTag)->second;
        pRet = lcl_SearchWordsInList( pList, rTxt, rStt, nEndPos );
        if( pRet )
        {
            rLang = aLanguageTag;
            return pRet;
        }
    }

    if(pLangTable->find(aLanguageTag.reset(LANGUAGE_UNDETERMINED)) != pLangTable->end() ||
            CreateLanguageFile(aLanguageTag, sal_False))
    {
        //the language is available - so bring it on
        SvxAutoCorrectLanguageLists* pList = pLangTable->find(aLanguageTag)->second;
        pRet = lcl_SearchWordsInList( pList, rTxt, rStt, nEndPos );
        if( pRet )
        {
            rLang = aLanguageTag;
            return pRet;
        }
    }
    return 0;
}

sal_Bool SvxAutoCorrect::FindInWrdSttExceptList( LanguageType eLang,
                                             const OUString& sWord )
{
    LanguageTag aLanguageTag( eLang);

    /* TODO-BCP47: again horrible uglyness */

    // First search for eLang, then US-English -> English
    // and last in LANGUAGE_UNDETERMINED
    LanguageType nTmpKey1 = eLang & 0x7ff, // the main language in many cases DE
                 nTmpKey2 = eLang & 0x3ff; // otherwise for example EN
    OUString sTemp(sWord);

    if(pLangTable->find(aLanguageTag) != pLangTable->end() || CreateLanguageFile(aLanguageTag, sal_False))
    {
        //the language is available - so bring it on
        SvxAutoCorrectLanguageLists* pList = pLangTable->find(aLanguageTag)->second;
        OUString _sTemp(sWord);
        if(pList->GetWrdSttExceptList()->find(_sTemp) != pList->GetWrdSttExceptList()->end() )
            return sal_True;
    }

    // If it still could not be found here, then keep on searching
    if(nTmpKey1 != eLang && (pLangTable->find(aLanguageTag.reset(nTmpKey1)) != pLangTable->end() ||
                CreateLanguageFile(aLanguageTag, sal_False)))
    {
        //the language is available - so bring it on
        SvxAutoCorrectLanguageLists* pList = pLangTable->find(aLanguageTag)->second;
        if(pList->GetWrdSttExceptList()->find(sTemp) != pList->GetWrdSttExceptList()->end() )
            return sal_True;
    }

    if(nTmpKey2 != eLang && (pLangTable->find(aLanguageTag.reset(nTmpKey2)) != pLangTable->end() ||
                CreateLanguageFile(aLanguageTag, sal_False)))
    {
        //the language is available - so bring it on
        SvxAutoCorrectLanguageLists* pList = pLangTable->find(aLanguageTag)->second;
        if(pList->GetWrdSttExceptList()->find(sTemp) != pList->GetWrdSttExceptList()->end() )
            return sal_True;
    }

    if(pLangTable->find(aLanguageTag.reset(LANGUAGE_UNDETERMINED)) != pLangTable->end() ||
            CreateLanguageFile(aLanguageTag, sal_False))
    {
        //the language is available - so bring it on
        SvxAutoCorrectLanguageLists* pList = pLangTable->find(aLanguageTag)->second;
        if(pList->GetWrdSttExceptList()->find(sTemp) != pList->GetWrdSttExceptList()->end() )
            return sal_True;
    }
    return sal_False;
}

static sal_Bool lcl_FindAbbreviation(const SvStringsISortDtor* pList, const OUString& sWord)
{
    OUString sAbk('~');
    SvStringsISortDtor::const_iterator it = pList->find( sAbk );
    sal_uInt16 nPos = it - pList->begin();
    if( nPos < pList->size() )
    {
        OUString sLowerWord(sWord.toAsciiLowerCase());
        OUString pAbk;
        for( sal_uInt16 n = nPos;
                n < pList->size() &&
                '~' == ( pAbk = (*pList)[ n ])[ 0 ];
            ++n )
        {
            // ~ and ~. are not allowed!
            if( 2 < pAbk.getLength() && pAbk.getLength() - 1 <= sWord.getLength() )
            {
                OUString sLowerAbk(pAbk.toAsciiLowerCase());
                for (sal_Int32 i = sLowerAbk.getLength(), ii = sLowerWord.getLength(); i;)
                {
                    if( !--i )      // agrees
                        return sal_True;

                    if( sLowerAbk[i] != sLowerWord[--ii])
                        break;
                }
            }
        }
    }
    OSL_ENSURE( !(nPos && '~' == (*pList)[ --nPos ][ 0 ] ),
            "Wrongly sorted exception list?" );
    return sal_False;
}

sal_Bool SvxAutoCorrect::FindInCplSttExceptList(LanguageType eLang,
                                const OUString& sWord, sal_Bool bAbbreviation)
{
    LanguageTag aLanguageTag( eLang);

    /* TODO-BCP47: did I mention terrible horrible uglyness? */

    // First search for eLang, then US-English -> English
    // and last in LANGUAGE_UNDETERMINED
    LanguageType nTmpKey1 = eLang & 0x7ff, // the main language in many cases DE
                 nTmpKey2 = eLang & 0x3ff; // otherwise for example EN
    OUString sTemp( sWord );

    if(pLangTable->find(aLanguageTag) != pLangTable->end() || CreateLanguageFile(aLanguageTag, sal_False))
    {
        //the language is available - so bring it on
        const SvStringsISortDtor* pList = pLangTable->find(aLanguageTag)->second->GetCplSttExceptList();
        if(bAbbreviation ? lcl_FindAbbreviation(pList, sWord) : pList->find(sTemp) != pList->end() )
            return sal_True;
    }

    // If it still could not be found here, then keep on searching
    if(nTmpKey1 != eLang && (pLangTable->find(aLanguageTag.reset(nTmpKey1)) != pLangTable->end() ||
                CreateLanguageFile(aLanguageTag, sal_False)))
    {
        const SvStringsISortDtor* pList = pLangTable->find(aLanguageTag)->second->GetCplSttExceptList();
        if(bAbbreviation ? lcl_FindAbbreviation(pList, sWord) : pList->find(sTemp) != pList->end() )
            return sal_True;
    }

    if(nTmpKey2 != eLang && (pLangTable->find(aLanguageTag.reset(nTmpKey2)) != pLangTable->end() ||
                CreateLanguageFile(aLanguageTag, sal_False)))
    {
        //the language is available - so bring it on
        const SvStringsISortDtor* pList = pLangTable->find(aLanguageTag)->second->GetCplSttExceptList();
        if(bAbbreviation ? lcl_FindAbbreviation(pList, sWord) : pList->find(sTemp) != pList->end() )
            return sal_True;
    }

    if(pLangTable->find(aLanguageTag.reset(LANGUAGE_UNDETERMINED)) != pLangTable->end() ||
            CreateLanguageFile(aLanguageTag, sal_False))
    {
        //the language is available - so bring it on
        const SvStringsISortDtor* pList = pLangTable->find(aLanguageTag)->second->GetCplSttExceptList();
        if(bAbbreviation ? lcl_FindAbbreviation(pList, sWord) : pList->find(sTemp) != pList->end() )
            return sal_True;
    }
    return sal_False;
}

OUString SvxAutoCorrect::GetAutoCorrFileName( const LanguageTag& rLanguageTag,
                                            sal_Bool bNewFile, sal_Bool bTst ) const
{
    OUString sRet, sExt( rLanguageTag.getBcp47() );

    sExt = "_" + sExt + ".dat";
    if( bNewFile )
        ( sRet = sUserAutoCorrFile )  += sExt;
    else if( !bTst )
        ( sRet = sShareAutoCorrFile )  += sExt;
    else
    {
        // test first in the user directory - if not exist, then
        ( sRet = sUserAutoCorrFile ) += sExt;
        if( !FStatHelper::IsDocument( sRet ))
            ( sRet = sShareAutoCorrFile ) += sExt;
    }
    return sRet;
}

SvxAutoCorrectLanguageLists::SvxAutoCorrectLanguageLists(
                SvxAutoCorrect& rParent,
                const OUString& rShareAutoCorrectFile,
                const OUString& rUserAutoCorrectFile)
:   sShareAutoCorrFile( rShareAutoCorrectFile ),
    sUserAutoCorrFile( rUserAutoCorrectFile ),
    aModifiedDate( Date::EMPTY ),
    aModifiedTime( Time::EMPTY ),
    aLastCheckTime( Time::EMPTY ),
    pCplStt_ExcptLst( 0 ),
    pWrdStt_ExcptLst( 0 ),
    pAutocorr_List( 0 ),
    rAutoCorrect(rParent),
    nFlags(0)
{
}

SvxAutoCorrectLanguageLists::~SvxAutoCorrectLanguageLists()
{
    delete pCplStt_ExcptLst;
    delete pWrdStt_ExcptLst;
    delete pAutocorr_List;
}

sal_Bool SvxAutoCorrectLanguageLists::IsFileChanged_Imp()
{
    // Access the file system only every 2 minutes to check the date stamp
    sal_Bool bRet = sal_False;

    Time nMinTime( 0, 2 );
    Time nAktTime( Time::SYSTEM );
    if( aLastCheckTime > nAktTime ||                    // overflow?
        ( nAktTime -= aLastCheckTime ) > nMinTime )     // min time past
    {
        Date aTstDate( Date::EMPTY ); Time aTstTime( Time::EMPTY );
        if( FStatHelper::GetModifiedDateTimeOfFile( sShareAutoCorrFile,
                                            &aTstDate, &aTstTime ) &&
            ( aModifiedDate != aTstDate || aModifiedTime != aTstTime ))
        {
            bRet = sal_True;
            // then remove all the lists fast!
            if( CplSttLstLoad & nFlags && pCplStt_ExcptLst )
                delete pCplStt_ExcptLst, pCplStt_ExcptLst = 0;
            if( WrdSttLstLoad & nFlags && pWrdStt_ExcptLst )
                delete pWrdStt_ExcptLst, pWrdStt_ExcptLst = 0;
            if( ChgWordLstLoad & nFlags && pAutocorr_List )
                delete pAutocorr_List, pAutocorr_List = 0;
            nFlags &= ~(CplSttLstLoad | WrdSttLstLoad | ChgWordLstLoad );
        }
        aLastCheckTime = Time( Time::SYSTEM );
    }
    return bRet;
}

void SvxAutoCorrectLanguageLists::LoadXMLExceptList_Imp(
                                        SvStringsISortDtor*& rpLst,
                                        const sal_Char* pStrmName,
                                        SotStorageRef& rStg)
{
    if( rpLst )
        rpLst->clear();
    else
        rpLst = new SvStringsISortDtor;

    {
        OUString sStrmName( pStrmName, strlen(pStrmName), RTL_TEXTENCODING_MS_1252 );
        OUString sTmp( sStrmName );

        if( rStg.Is() && rStg->IsStream( sStrmName ) )
        {
            SvStorageStreamRef xStrm = rStg->OpenSotStream( sTmp,
                ( STREAM_READ | STREAM_SHARE_DENYWRITE | STREAM_NOCREATE ) );
            if( SVSTREAM_OK != xStrm->GetError())
            {
                xStrm.Clear();
                rStg.Clear();
                RemoveStream_Imp( sStrmName );
            }
            else
            {
                uno::Reference< uno::XComponentContext > xContext =
                    comphelper::getProcessComponentContext();

                xml::sax::InputSource aParserInput;
                aParserInput.sSystemId = sStrmName;

                xStrm->Seek( 0L );
                xStrm->SetBufferSize( 8 * 1024 );
                aParserInput.aInputStream = new utl::OInputStreamWrapper( *xStrm );

                // get filter
                uno::Reference< xml::sax::XDocumentHandler > xFilter = new SvXMLExceptionListImport ( xContext, *rpLst );

                // connect parser and filter
                uno::Reference< xml::sax::XParser > xParser = xml::sax::Parser::create( xContext );
                xParser->setDocumentHandler( xFilter );

                // parse
                try
                {
                    xParser->parseStream( aParserInput );
                }
                catch( const xml::sax::SAXParseException& )
                {
                    // re throw ?
                }
                catch( const xml::sax::SAXException& )
                {
                    // re throw ?
                }
                catch( const io::IOException& )
                {
                    // re throw ?
                }
            }
        }

        // Set time stamp
        FStatHelper::GetModifiedDateTimeOfFile( sShareAutoCorrFile,
                                        &aModifiedDate, &aModifiedTime );
        aLastCheckTime = Time( Time::SYSTEM );
    }

}

void SvxAutoCorrectLanguageLists::SaveExceptList_Imp(
                            const SvStringsISortDtor& rLst,
                            const sal_Char* pStrmName,
                            SotStorageRef &rStg,
                            sal_Bool bConvert )
{
    if( rStg.Is() )
    {
        OUString sStrmName( pStrmName, strlen(pStrmName), RTL_TEXTENCODING_MS_1252 );
        if( rLst.empty() )
        {
            rStg->Remove( sStrmName );
            rStg->Commit();
        }
        else
        {
            SotStorageStreamRef xStrm = rStg->OpenSotStream( sStrmName,
                    ( STREAM_READ | STREAM_WRITE | STREAM_SHARE_DENYWRITE ) );
            if( xStrm.Is() )
            {
                xStrm->SetSize( 0 );
                xStrm->SetBufferSize( 8192 );
                OUString aMime( "text/xml" );
                uno::Any aAny;
                aAny <<= aMime;
                xStrm->SetProperty( OUString("MediaType"), aAny );


                uno::Reference< uno::XComponentContext > xContext =
                    comphelper::getProcessComponentContext();

                uno::Reference < xml::sax::XWriter > xWriter  = xml::sax::Writer::create(xContext);
                uno::Reference < io::XOutputStream> xOut = new utl::OOutputStreamWrapper( *xStrm );
                xWriter->setOutputStream(xOut);

                uno::Reference < xml::sax::XDocumentHandler > xHandler(xWriter, UNO_QUERY_THROW);
                SvXMLExceptionListExport aExp( xContext, rLst, sStrmName, xHandler );

                aExp.exportDoc( XML_BLOCK_LIST );

                xStrm->Commit();
                if( xStrm->GetError() == SVSTREAM_OK )
                {
                    xStrm.Clear();
                    if (!bConvert)
                    {
                        rStg->Commit();
                        if( SVSTREAM_OK != rStg->GetError() )
                        {
                            rStg->Remove( sStrmName );
                            rStg->Commit();
                        }
                    }
                }
            }
        }
    }
}

SvxAutocorrWordList* SvxAutoCorrectLanguageLists::LoadAutocorrWordList()
{
    if( pAutocorr_List )
        pAutocorr_List->DeleteAndDestroyAll();
    else
        pAutocorr_List = new SvxAutocorrWordList();

    try
    {
        uno::Reference < embed::XStorage > xStg = comphelper::OStorageHelper::GetStorageFromURL( sShareAutoCorrFile, embed::ElementModes::READ );
        OUString aXMLWordListName( pXMLImplAutocorr_ListStr, strlen(pXMLImplAutocorr_ListStr), RTL_TEXTENCODING_MS_1252 );
        uno::Reference < io::XStream > xStrm = xStg->openStreamElement( aXMLWordListName, embed::ElementModes::READ );
        uno::Reference< uno::XComponentContext > xContext = comphelper::getProcessComponentContext();

        xml::sax::InputSource aParserInput;
        aParserInput.sSystemId = aXMLWordListName;
        aParserInput.aInputStream = xStrm->getInputStream();

        // get parser
        uno::Reference< xml::sax::XParser > xParser = xml::sax::Parser::create(xContext);
        SAL_INFO("editeng", "AutoCorrect Import" );
        uno::Reference< xml::sax::XDocumentHandler > xFilter = new SvXMLAutoCorrectImport( xContext, pAutocorr_List, rAutoCorrect, xStg );

        // connect parser and filter
        xParser->setDocumentHandler( xFilter );

        // parse
        xParser->parseStream( aParserInput );
    }
    catch ( const uno::Exception& )
    {
    }

    // Set time stamp
    FStatHelper::GetModifiedDateTimeOfFile( sShareAutoCorrFile,
                                    &aModifiedDate, &aModifiedTime );
    aLastCheckTime = Time( Time::SYSTEM );

    return pAutocorr_List;
}

void SvxAutoCorrectLanguageLists::SetAutocorrWordList( SvxAutocorrWordList* pList )
{
    if( pAutocorr_List && pList != pAutocorr_List )
        delete pAutocorr_List;
    pAutocorr_List = pList;
    if( !pAutocorr_List )
    {
        OSL_ENSURE( !this, "No valid list" );
        pAutocorr_List = new SvxAutocorrWordList();
    }
    nFlags |= ChgWordLstLoad;
}

const SvxAutocorrWordList* SvxAutoCorrectLanguageLists::GetAutocorrWordList()
{
    if( !( ChgWordLstLoad & nFlags ) || IsFileChanged_Imp() )
        SetAutocorrWordList( LoadAutocorrWordList() );
    return pAutocorr_List;
}

SvStringsISortDtor* SvxAutoCorrectLanguageLists::GetCplSttExceptList()
{
    if( !( CplSttLstLoad & nFlags ) || IsFileChanged_Imp() )
        SetCplSttExceptList( LoadCplSttExceptList() );
    return pCplStt_ExcptLst;
}

sal_Bool SvxAutoCorrectLanguageLists::AddToCplSttExceptList(const OUString& rNew)
{
    sal_Bool aRet = sal_False;
    if( !rNew.isEmpty() && GetCplSttExceptList()->insert( rNew ).second )
    {
        MakeUserStorage_Impl();
        SotStorageRef xStg = new SotStorage( sUserAutoCorrFile, STREAM_READWRITE, sal_True );

        SaveExceptList_Imp( *pCplStt_ExcptLst, pXMLImplCplStt_ExcptLstStr, xStg );

        xStg = 0;
        // Set time stamp
        FStatHelper::GetModifiedDateTimeOfFile( sUserAutoCorrFile,
                                            &aModifiedDate, &aModifiedTime );
        aLastCheckTime = Time( Time::SYSTEM );
        aRet = sal_True;
    }
    return aRet;
}

sal_Bool SvxAutoCorrectLanguageLists::AddToWrdSttExceptList(const OUString& rNew)
{
    sal_Bool aRet = sal_False;
    SvStringsISortDtor* pExceptList = LoadWrdSttExceptList();
    if( !rNew.isEmpty() && pExceptList && pExceptList->insert( rNew ).second )
    {
        MakeUserStorage_Impl();
        SotStorageRef xStg = new SotStorage( sUserAutoCorrFile, STREAM_READWRITE, sal_True );

        SaveExceptList_Imp( *pWrdStt_ExcptLst, pXMLImplWrdStt_ExcptLstStr, xStg );

        xStg = 0;
        // Set time stamp
        FStatHelper::GetModifiedDateTimeOfFile( sUserAutoCorrFile,
                                            &aModifiedDate, &aModifiedTime );
        aLastCheckTime = Time( Time::SYSTEM );
        aRet = sal_True;
    }
    return aRet;
}

SvStringsISortDtor* SvxAutoCorrectLanguageLists::LoadCplSttExceptList()
{
    SotStorageRef xStg = new SotStorage( sShareAutoCorrFile, STREAM_READ | STREAM_SHARE_DENYNONE, sal_True );
    OUString sTemp ( pXMLImplCplStt_ExcptLstStr );
    if( xStg.Is() && xStg->IsContained( sTemp ) )
        LoadXMLExceptList_Imp( pCplStt_ExcptLst, pXMLImplCplStt_ExcptLstStr, xStg );

    return pCplStt_ExcptLst;
}

void SvxAutoCorrectLanguageLists::SaveCplSttExceptList()
{
    MakeUserStorage_Impl();
    SotStorageRef xStg = new SotStorage( sUserAutoCorrFile, STREAM_READWRITE, sal_True );

    SaveExceptList_Imp( *pCplStt_ExcptLst, pXMLImplCplStt_ExcptLstStr, xStg );

    xStg = 0;

    // Set time stamp
    FStatHelper::GetModifiedDateTimeOfFile( sUserAutoCorrFile,
                                            &aModifiedDate, &aModifiedTime );
    aLastCheckTime = Time( Time::SYSTEM );
}

void SvxAutoCorrectLanguageLists::SetCplSttExceptList( SvStringsISortDtor* pList )
{
    if( pCplStt_ExcptLst && pList != pCplStt_ExcptLst )
        delete pCplStt_ExcptLst;

    pCplStt_ExcptLst = pList;
    if( !pCplStt_ExcptLst )
    {
        OSL_ENSURE( !this, "No valid list" );
        pCplStt_ExcptLst = new SvStringsISortDtor;
    }
    nFlags |= CplSttLstLoad;
}

SvStringsISortDtor* SvxAutoCorrectLanguageLists::LoadWrdSttExceptList()
{
    SotStorageRef xStg = new SotStorage( sShareAutoCorrFile, STREAM_READ | STREAM_SHARE_DENYNONE, sal_True );
    OUString sTemp ( pXMLImplWrdStt_ExcptLstStr );
    if( xStg.Is() && xStg->IsContained( sTemp ) )
        LoadXMLExceptList_Imp( pWrdStt_ExcptLst, pXMLImplWrdStt_ExcptLstStr, xStg );
    return pWrdStt_ExcptLst;
}

void SvxAutoCorrectLanguageLists::SaveWrdSttExceptList()
{
    MakeUserStorage_Impl();
    SotStorageRef xStg = new SotStorage( sUserAutoCorrFile, STREAM_READWRITE, sal_True );

    SaveExceptList_Imp( *pWrdStt_ExcptLst, pXMLImplWrdStt_ExcptLstStr, xStg );

    xStg = 0;
    // Set time stamp
    FStatHelper::GetModifiedDateTimeOfFile( sUserAutoCorrFile,
                                            &aModifiedDate, &aModifiedTime );
    aLastCheckTime = Time( Time::SYSTEM );
}

void SvxAutoCorrectLanguageLists::SetWrdSttExceptList( SvStringsISortDtor* pList )
{
    if( pWrdStt_ExcptLst && pList != pWrdStt_ExcptLst )
        delete pWrdStt_ExcptLst;
    pWrdStt_ExcptLst = pList;
    if( !pWrdStt_ExcptLst )
    {
        OSL_ENSURE( !this, "No valid list" );
        pWrdStt_ExcptLst = new SvStringsISortDtor;
    }
    nFlags |= WrdSttLstLoad;
}

SvStringsISortDtor* SvxAutoCorrectLanguageLists::GetWrdSttExceptList()
{
    if( !( WrdSttLstLoad & nFlags ) || IsFileChanged_Imp() )
        SetWrdSttExceptList( LoadWrdSttExceptList() );
    return pWrdStt_ExcptLst;
}

void SvxAutoCorrectLanguageLists::RemoveStream_Imp( const OUString& rName )
{
    if( sShareAutoCorrFile != sUserAutoCorrFile )
    {
        SotStorageRef xStg = new SotStorage( sUserAutoCorrFile, STREAM_READWRITE, sal_True );
        if( xStg.Is() && SVSTREAM_OK == xStg->GetError() &&
            xStg->IsStream( rName ) )
        {
            xStg->Remove( rName );
            xStg->Commit();

            xStg = 0;
        }
    }
}

void SvxAutoCorrectLanguageLists::MakeUserStorage_Impl()
{
    // The conversion needs to happen if the file is already in the user
    // directory and is in the old format. Additionally it needs to
    // happen when the file is being copied from share to user.

    sal_Bool bError = sal_False, bConvert = sal_False, bCopy = sal_False;
    INetURLObject aDest;
    INetURLObject aSource;

    if (sUserAutoCorrFile != sShareAutoCorrFile )
    {
        aSource = INetURLObject ( sShareAutoCorrFile );
        aDest = INetURLObject ( sUserAutoCorrFile );
        if ( SotStorage::IsOLEStorage ( sShareAutoCorrFile ) )
        {
            aDest.SetExtension ( OUString("bak") );
            bConvert = sal_True;
        }
        bCopy = sal_True;
    }
    else if ( SotStorage::IsOLEStorage ( sUserAutoCorrFile ) )
    {
        aSource = INetURLObject ( sUserAutoCorrFile );
        aDest = INetURLObject ( sUserAutoCorrFile );
        aDest.SetExtension ( OUString("bak") );
        bCopy = bConvert = sal_True;
    }
    if (bCopy)
    {
        try
        {
            OUString sMain(aDest.GetMainURL( INetURLObject::DECODE_TO_IURI ));
            sal_Unicode cSlash = '/';
            sal_Int32 nSlashPos = sMain.lastIndexOf(cSlash);
            sMain = sMain.copy(0, nSlashPos);
            ::ucbhelper::Content aNewContent( sMain, uno::Reference< XCommandEnvironment >(), comphelper::getProcessComponentContext() );
            Any aAny;
            TransferInfo aInfo;
            aInfo.NameClash = NameClash::OVERWRITE;
            aInfo.NewTitle  = aDest.GetName();
            aInfo.SourceURL = aSource.GetMainURL( INetURLObject::DECODE_TO_IURI );
            aInfo.MoveData  = sal_False;
            aAny <<= aInfo;
            aNewContent.executeCommand( OUString (  "transfer"  ), aAny);
        }
        catch (...)
        {
            bError = sal_True;
        }
    }
    if (bConvert && !bError)
    {
        SotStorageRef xSrcStg = new SotStorage( aDest.GetMainURL( INetURLObject::DECODE_TO_IURI ), STREAM_READ, sal_True );
        SotStorageRef xDstStg = new SotStorage( sUserAutoCorrFile, STREAM_WRITE, sal_True );

        if( xSrcStg.Is() && xDstStg.Is() )
        {
            OUString sXMLWord     ( pXMLImplWrdStt_ExcptLstStr );
            OUString sXMLSentence ( pXMLImplCplStt_ExcptLstStr );
            SvStringsISortDtor  *pTmpWordList = NULL;

            if (xSrcStg->IsContained( sXMLWord ) )
                LoadXMLExceptList_Imp( pTmpWordList, pXMLImplWrdStt_ExcptLstStr, xSrcStg );

            if (pTmpWordList)
            {
                SaveExceptList_Imp( *pTmpWordList, pXMLImplWrdStt_ExcptLstStr, xDstStg, sal_True );
                pTmpWordList->clear();
                pTmpWordList = NULL;
            }


            if (xSrcStg->IsContained( sXMLSentence ) )
                LoadXMLExceptList_Imp( pTmpWordList, pXMLImplCplStt_ExcptLstStr, xSrcStg );

            if (pTmpWordList)
            {
                SaveExceptList_Imp( *pTmpWordList, pXMLImplCplStt_ExcptLstStr, xDstStg, sal_True );
                pTmpWordList->clear();
            }

            GetAutocorrWordList();
            MakeBlocklist_Imp( *xDstStg );
            sShareAutoCorrFile = sUserAutoCorrFile;
            xDstStg = 0;
            try
            {
                ::ucbhelper::Content aContent ( aDest.GetMainURL( INetURLObject::DECODE_TO_IURI ), uno::Reference < XCommandEnvironment >(), comphelper::getProcessComponentContext() );
                aContent.executeCommand ( OUString( "delete" ), makeAny ( sal_Bool (sal_True ) ) );
            }
            catch (...)
            {
            }
        }
    }
    else if( bCopy && !bError )
        sShareAutoCorrFile = sUserAutoCorrFile;
}

sal_Bool SvxAutoCorrectLanguageLists::MakeBlocklist_Imp( SvStorage& rStg )
{
    OUString sStrmName( pXMLImplAutocorr_ListStr, strlen(pXMLImplAutocorr_ListStr), RTL_TEXTENCODING_MS_1252 );
    sal_Bool bRet = sal_True, bRemove = !pAutocorr_List || pAutocorr_List->empty();
    if( !bRemove )
    {
        SvStorageStreamRef refList = rStg.OpenSotStream( sStrmName,
                    ( STREAM_READ | STREAM_WRITE | STREAM_SHARE_DENYWRITE ) );
        if( refList.Is() )
        {
            refList->SetSize( 0 );
            refList->SetBufferSize( 8192 );
            OUString aPropName( "MediaType" );
            OUString aMime( "text/xml" );
            uno::Any aAny;
            aAny <<= aMime;
            refList->SetProperty( aPropName, aAny );

            uno::Reference< uno::XComponentContext > xContext =
                comphelper::getProcessComponentContext();

            uno::Reference < xml::sax::XWriter > xWriter = xml::sax::Writer::create(xContext);
            uno::Reference < io::XOutputStream> xOut = new utl::OOutputStreamWrapper( *refList );
            xWriter->setOutputStream(xOut);

            uno::Reference<xml::sax::XDocumentHandler> xHandler(xWriter, uno::UNO_QUERY);
            SvXMLAutoCorrectExport aExp( xContext, pAutocorr_List, sStrmName, xHandler );

            aExp.exportDoc( XML_BLOCK_LIST );

            refList->Commit();
            bRet = SVSTREAM_OK == refList->GetError();
            if( bRet )
            {
                refList.Clear();
                rStg.Commit();
                if( SVSTREAM_OK != rStg.GetError() )
                {
                    bRemove = sal_True;
                    bRet = sal_False;
                }
            }
        }
        else
            bRet = sal_False;
    }

    if( bRemove )
    {
        rStg.Remove( sStrmName );
        rStg.Commit();
    }

    return bRet;
}

sal_Bool SvxAutoCorrectLanguageLists::MakeCombinedChanges( std::vector<SvxAutocorrWord>& aNewEntries, std::vector<SvxAutocorrWord>& aDeleteEntries )
{
    // First get the current list!
    GetAutocorrWordList();

    MakeUserStorage_Impl();
    SotStorageRef xStorage = new SotStorage( sUserAutoCorrFile, STREAM_READWRITE, sal_True );

    sal_Bool bRet = xStorage.Is() && SVSTREAM_OK == xStorage->GetError();

    if( bRet )
    {
        for ( sal_uInt32 i=0; i < aDeleteEntries.size(); i++ )
        {
            SvxAutocorrWord aWordToDelete = aDeleteEntries[i];
            SvxAutocorrWord *pFoundEntry = pAutocorr_List->FindAndRemove( &aWordToDelete );
            if( pFoundEntry )
            {
                if( !pFoundEntry->IsTextOnly() )
                {
                    OUString aName( aWordToDelete.GetShort() );
                    if (xStorage->IsOLEStorage())
                        aName = EncryptBlockName_Imp(aName);
                    else
                        GeneratePackageName ( aWordToDelete.GetShort(), aName );

                    if( xStorage->IsContained( aName ) )
                    {
                        xStorage->Remove( aName );
                        bRet = xStorage->Commit();
                    }
                }
                delete pFoundEntry;
            }
        }

        for ( sal_uInt32 i=0; i < aNewEntries.size(); i++ )
        {
            SvxAutocorrWord *pWordToAdd = new SvxAutocorrWord( aNewEntries[i].GetShort(), aNewEntries[i].GetLong(), sal_True );
            SvxAutocorrWord *pRemoved = pAutocorr_List->FindAndRemove( pWordToAdd );
            if( pRemoved )
            {
                if( !pRemoved->IsTextOnly() )
                {
                    // Still have to remove the Storage
                    OUString sStorageName( pWordToAdd->GetShort() );
                    if (xStorage->IsOLEStorage())
                        sStorageName = EncryptBlockName_Imp(sStorageName);
                    else
                        GeneratePackageName ( pWordToAdd->GetShort(), sStorageName);

                    if( xStorage->IsContained( sStorageName ) )
                        xStorage->Remove( sStorageName );
                }
                delete pRemoved;
            }
            bRet = pAutocorr_List->Insert( pWordToAdd );

            if ( !bRet )
            {
                delete pWordToAdd;
                break;
            }
        }

        if ( bRet )
        {
            bRet = MakeBlocklist_Imp( *xStorage );
        }
    }
    return bRet;
}

sal_Bool SvxAutoCorrectLanguageLists::PutText( const OUString& rShort, const OUString& rLong )
{
    // First get the current list!
    GetAutocorrWordList();

    MakeUserStorage_Impl();
    SotStorageRef xStg = new SotStorage( sUserAutoCorrFile, STREAM_READWRITE, sal_True );

    sal_Bool bRet = xStg.Is() && SVSTREAM_OK == xStg->GetError();

    // Update the word list
    if( bRet )
    {
        SvxAutocorrWord* pNew = new SvxAutocorrWord( rShort, rLong, sal_True );
        SvxAutocorrWord *pRemove = pAutocorr_List->FindAndRemove( pNew );
        if( pRemove )
        {
            if( !pRemove->IsTextOnly() )
            {
                // Still have to remove the Storage
                OUString sStgNm( rShort );
                if (xStg->IsOLEStorage())
                    sStgNm = EncryptBlockName_Imp(sStgNm);
                else
                    GeneratePackageName ( rShort, sStgNm);

                if( xStg->IsContained( sStgNm ) )
                    xStg->Remove( sStgNm );
            }
            delete pRemove;
        }

        if( pAutocorr_List->Insert( pNew ) )
        {
            bRet = MakeBlocklist_Imp( *xStg );
            xStg = 0;
        }
        else
        {
            delete pNew;
            bRet = sal_False;
        }
    }
    return bRet;
}

sal_Bool SvxAutoCorrectLanguageLists::PutText( const OUString& rShort,
                                               SfxObjectShell& rShell )
{
    // First get the current list!
    GetAutocorrWordList();

    MakeUserStorage_Impl();

    sal_Bool bRet = sal_False;
    OUString sLong;
    try
    {
        uno::Reference < embed::XStorage > xStg = comphelper::OStorageHelper::GetStorageFromURL( sUserAutoCorrFile, embed::ElementModes::READWRITE );
        bRet = rAutoCorrect.PutText( xStg, sUserAutoCorrFile, rShort, rShell, sLong );
        xStg = 0;

        // Update the word list
        if( bRet )
        {
            SvxAutocorrWord* pNew = new SvxAutocorrWord( rShort, sLong, sal_False );
            if( pAutocorr_List->Insert( pNew ) )
            {
                SotStorageRef xStor = new SotStorage( sUserAutoCorrFile, STREAM_READWRITE, sal_True );
                MakeBlocklist_Imp( *xStor );
            }
            else
                delete pNew;
        }
    }
    catch ( const uno::Exception& )
    {
    }

    return bRet;
}

// Delete an entry
sal_Bool SvxAutoCorrectLanguageLists::DeleteText( const OUString& rShort )
{
    // First get the current list!
    GetAutocorrWordList();

    MakeUserStorage_Impl();

    SotStorageRef xStg = new SotStorage( sUserAutoCorrFile, STREAM_READWRITE, sal_True );
    sal_Bool bRet = xStg.Is() && SVSTREAM_OK == xStg->GetError();
    if( bRet )
    {
        SvxAutocorrWord aTmp( rShort, rShort );
        SvxAutocorrWord *pFnd = pAutocorr_List->FindAndRemove( &aTmp );
        if( pFnd )
        {
            if( !pFnd->IsTextOnly() )
            {
                OUString aName( rShort );
                if (xStg->IsOLEStorage())
                    aName = EncryptBlockName_Imp(aName);
                else
                    GeneratePackageName ( rShort, aName );
                if( xStg->IsContained( aName ) )
                {
                    xStg->Remove( aName );
                    bRet = xStg->Commit();
                }

            }
            delete pFnd;
            MakeBlocklist_Imp( *xStg );
            xStg = 0;
        }
        else
            bRet = sal_False;
    }
    return bRet;
}

// Keep the list sorted ...
bool CompareSvxAutocorrWordList::operator()( SvxAutocorrWord* const& lhs, SvxAutocorrWord* const& rhs ) const
{
    CollatorWrapper& rCmp = ::GetCollatorWrapper();
    return rCmp.compareString( lhs->GetShort(), rhs->GetShort() ) < 0;
}

SvxAutocorrWordList::~SvxAutocorrWordList()
{
    DeleteAndDestroyAll();
}

void SvxAutocorrWordList::DeleteAndDestroyAll()
{
    for( SvxAutocorrWordList_Hash::const_iterator it = maHash.begin(); it != maHash.end(); ++it )
        delete it->second;
    maHash.clear();

    for( SvxAutocorrWordList_Set::const_iterator it2 = maSet.begin(); it2 != maSet.end(); ++it2 )
        delete *it2;
    maSet.clear();
}

// returns true if inserted
bool SvxAutocorrWordList::Insert(SvxAutocorrWord *pWord) const
{
    if ( maSet.empty() ) // use the hash
    {
        OUString aShort( pWord->GetShort() );
        return maHash.insert( std::pair<OUString, SvxAutocorrWord *>( aShort, pWord ) ).second;
    }
    else
        return maSet.insert( pWord ).second;
}

void SvxAutocorrWordList::LoadEntry(const OUString& sWrong, const OUString& sRight, sal_Bool bOnlyTxt)
{
    SvxAutocorrWord* pNew = new SvxAutocorrWord( sWrong, sRight, bOnlyTxt );
    if( !Insert( pNew ) )
        delete pNew;
}

bool SvxAutocorrWordList::empty() const
{
    return maHash.empty() && maSet.empty();
}

SvxAutocorrWord *SvxAutocorrWordList::FindAndRemove(SvxAutocorrWord *pWord)
{
    SvxAutocorrWord *pMatch = NULL;

    if ( maSet.empty() ) // use the hash
    {
        SvxAutocorrWordList_Hash::iterator it = maHash.find( pWord->GetShort() );
        if( it != maHash.end() )
        {
            pMatch = it->second;
            maHash.erase (it);
        }
    }
    else
    {
        SvxAutocorrWordList_Set::iterator it = maSet.find( pWord );
        if( it != maSet.end() )
        {
            pMatch = *it;
            maSet.erase (it);
        }
    }
    return pMatch;
}

// return the sorted contents - defer sorting until we have to.
SvxAutocorrWordList::Content SvxAutocorrWordList::getSortedContent() const
{
    Content aContent;

    // convert from hash to set permanantly
    if ( maSet.empty() )
    {
        // This beasty has some O(N log(N)) in a terribly slow ICU collate fn.
        for( SvxAutocorrWordList_Hash::const_iterator it = maHash.begin(); it != maHash.end(); ++it )
            maSet.insert( it->second );
        maHash.clear();
    }
    for( SvxAutocorrWordList_Set::const_iterator it = maSet.begin(); it != maSet.end(); ++it )
        aContent.push_back( *it );

    return aContent;
}

const SvxAutocorrWord* SvxAutocorrWordList::WordMatches(const SvxAutocorrWord *pFnd,
                                      const OUString &rTxt,
                                      sal_Int32 &rStt,
                                      sal_Int32 nEndPos) const
{
    const OUString& rChk = pFnd->GetShort();
    xub_StrLen left_wildcard = ( rChk[0] == C_ASTERISK ) ? 1 : 0; // "*word" pattern?
    xub_StrLen nSttWdPos = nEndPos;
    if( nEndPos >= rChk.getLength() - left_wildcard)
    {

        bool bWasWordDelim = false;
        xub_StrLen nCalcStt = nEndPos - rChk.getLength() + left_wildcard;
        if( ( !nCalcStt || nCalcStt == rStt || left_wildcard ||
              ( nCalcStt < rStt &&
                IsWordDelim( rTxt[ nCalcStt - 1 ] ))) )
        {
            TransliterationWrapper& rCmp = GetIgnoreTranslWrapper();
            OUString sWord = rTxt.copy(nCalcStt, rChk.getLength() - left_wildcard);
            if( (!left_wildcard && rCmp.isEqual( rChk, sWord )) || (left_wildcard && rCmp.isEqual( rChk.copy(1), sWord) ))
            {
                rStt = nCalcStt;
                if (!left_wildcard) return pFnd;
                // get the first word delimiter position before the matching "*word" pattern
                while( rStt && !(bWasWordDelim = IsWordDelim( rTxt[ --rStt ])))
                    ;
                if (bWasWordDelim) rStt++;
                SvxAutocorrWord* pNew = new SvxAutocorrWord(rTxt.copy(rStt, nEndPos - rStt), rTxt.copy(rStt, nEndPos - rStt - rChk.getLength() + 1) + pFnd->GetLong());
                if( Insert( pNew ) ) return pNew; else delete pNew;
            }
        }
        // match "word*" patterns, eg. "i18n*"
        if ( rChk[ rChk.getLength() - 1] == C_ASTERISK )
        {
            OUString sTmp( rChk.copy( 0, rChk.getLength() - 1 ) );
            // Get the last word delimiter position
            bool not_suffix;
            while( nSttWdPos && !(bWasWordDelim = IsWordDelim( rTxt[ --nSttWdPos ])))
                ;
            // search the first occurance with a left word delimitation
            sal_Int32 nFndPos = -1;
            do {
                nFndPos = rTxt.indexOf( sTmp, nFndPos + 1);
                not_suffix = (bWasWordDelim && (nSttWdPos >= nFndPos + sTmp.getLength()));
            } while ( nFndPos != -1 && (!(!nFndPos || IsWordDelim( rTxt[ nFndPos - 1 ])) || not_suffix));
            if ( nFndPos != -1 )
            {
                // store matching pattern and its replacement as a new list item, eg. "i18ns" -> "internationalizations"
                OUString aShort = rTxt.copy(nFndPos, nEndPos - nFndPos + ((rTxt[nEndPos] != 0x20) ? 1: 0));
                OUString aLong = pFnd->GetLong() + rTxt.copy(nFndPos + sTmp.getLength(), nEndPos - nFndPos - sTmp.getLength());
                SvxAutocorrWord* pNew = new SvxAutocorrWord(aShort, aLong);
                if( Insert( pNew ) )
                {
                    rStt = nFndPos;
                    return pNew;
                } else delete pNew;
            }
        }
    }
    return NULL;
}

const SvxAutocorrWord* SvxAutocorrWordList::SearchWordsInList(const OUString& rTxt, sal_Int32& rStt,
                                                              sal_Int32 nEndPos) const
{
    for( SvxAutocorrWordList_Hash::const_iterator it = maHash.begin(); it != maHash.end(); ++it )
    {
        if( const SvxAutocorrWord *aTmp = WordMatches( it->second, rTxt, rStt, nEndPos ) )
            return aTmp;
    }

    for( SvxAutocorrWordList_Set::const_iterator it2 = maSet.begin(); it2 != maSet.end(); ++it2 )
    {
        if( const SvxAutocorrWord *aTmp = WordMatches( *it2, rTxt, rStt, nEndPos ) )
            return aTmp;
    }
    return 0;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
