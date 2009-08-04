#ifndef EPICS_DBR_TOOLS_H
#define EPICS_DBR_TOOLS_H

#include <string>
#include "cadef.h"

namespace Pds
{

namespace EpicsDbrTools
{
const int iSizeDbrTypes = sizeof(dbf_text)/(sizeof(char*))-2;

template <bool bAssert>
struct Assert {}; // Default to generate compier error

template <>
struct Assert<true> { enum {okay}; };

typedef short int Int16;
enum { iAssert1 = Assert<sizeof(Int16) == 2>::okay };


/*
 * CaDbr Tool classes
 */
template <int iDbrType> struct DbrTypeFromInt { char a[-1 + iDbrType*0]; }; // Default should not be used. Will generate compilation error.
template <> struct DbrTypeFromInt<DBR_STRING> { typedef dbr_string_t TDbr; };
template <> struct DbrTypeFromInt<DBR_SHORT>  { typedef dbr_short_t  TDbr; };
template <> struct DbrTypeFromInt<DBR_FLOAT>  { typedef dbr_float_t  TDbr; };
template <> struct DbrTypeFromInt<DBR_ENUM>   { typedef dbr_enum_t   TDbr; };
template <> struct DbrTypeFromInt<DBR_CHAR>   { typedef dbr_char_t   TDbr; };
template <> struct DbrTypeFromInt<DBR_LONG>   { typedef dbr_long_t   TDbr; };
template <> struct DbrTypeFromInt<DBR_DOUBLE> { typedef dbr_double_t TDbr; };

template <> struct DbrTypeFromInt<DBR_TIME_STRING> { typedef dbr_time_string TDbr; };
template <> struct DbrTypeFromInt<DBR_TIME_SHORT>  { typedef dbr_time_short  TDbr; };
template <> struct DbrTypeFromInt<DBR_TIME_FLOAT>  { typedef dbr_time_float  TDbr; };
template <> struct DbrTypeFromInt<DBR_TIME_ENUM>   { typedef dbr_time_enum   TDbr; };
template <> struct DbrTypeFromInt<DBR_TIME_CHAR>   { typedef dbr_time_char   TDbr; };
template <> struct DbrTypeFromInt<DBR_TIME_LONG>   { typedef dbr_time_long   TDbr; };
template <> struct DbrTypeFromInt<DBR_TIME_DOUBLE> { typedef dbr_time_double TDbr; };

template <> struct DbrTypeFromInt<DBR_CTRL_STRING> { typedef dbr_sts_string  TDbr; };
template <> struct DbrTypeFromInt<DBR_CTRL_SHORT>  { typedef dbr_ctrl_short  TDbr; };
template <> struct DbrTypeFromInt<DBR_CTRL_FLOAT>  { typedef dbr_ctrl_float  TDbr; };
template <> struct DbrTypeFromInt<DBR_CTRL_ENUM>   { typedef dbr_ctrl_enum   TDbr; };
template <> struct DbrTypeFromInt<DBR_CTRL_CHAR>   { typedef dbr_ctrl_char   TDbr; };
template <> struct DbrTypeFromInt<DBR_CTRL_LONG>   { typedef dbr_ctrl_long   TDbr; };
template <> struct DbrTypeFromInt<DBR_CTRL_DOUBLE> { typedef dbr_ctrl_double TDbr; };

template <int iDbrType> struct DbrTypeTraits
{ 
    enum { 
      iDiffDbrOrgToDbrTime = 2*iSizeDbrTypes,
      iDiffDbrOrgToDbrCtrl = 4*iSizeDbrTypes };
      
    enum { 
      iDbrTimeType = iDbrType + iDiffDbrOrgToDbrTime,
      iDbrCtrlType = iDbrType + iDiffDbrOrgToDbrCtrl };
        
    typedef typename DbrTypeFromInt<iDbrType    >::TDbr TDbrOrg;
    typedef typename DbrTypeFromInt<iDbrTimeType>::TDbr TDbrTime;
    typedef typename DbrTypeFromInt<iDbrCtrlType>::TDbr TDbrCtrl;    
}; 

template <class TDbr> struct DbrIntFromType { char a[-1 + (TDbr)0]; }; // Default should not be used. Will generate compilation error.
template <class TDbr> struct DbrIntFromType<const TDbr> : DbrIntFromType<TDbr> {}; // remove const qualifier
template <> struct DbrIntFromType<dbr_string_t> { enum {iTypeId = DBR_STRING }; };
template <> struct DbrIntFromType<dbr_short_t>  { enum {iTypeId = DBR_SHORT  }; };
template <> struct DbrIntFromType<dbr_float_t>  { enum {iTypeId = DBR_FLOAT  }; };
template <> struct DbrIntFromType<dbr_enum_t>   { enum {iTypeId = DBR_ENUM   }; };
template <> struct DbrIntFromType<dbr_char_t>   { enum {iTypeId = DBR_CHAR   }; };
template <> struct DbrIntFromType<dbr_long_t>   { enum {iTypeId = DBR_LONG   }; };
template <> struct DbrIntFromType<dbr_double_t> { enum {iTypeId = DBR_DOUBLE }; };

template <class TDbr> struct DbrIntFromCtrlType { char a[-1 + (TDbr)0]; }; // Default should not be used. Will generate compilation error.
template <class TDbr> struct DbrIntFromCtrlType<const TDbr> : DbrIntFromCtrlType<TDbr> {}; // remove const qualifier
template <> struct DbrIntFromCtrlType<dbr_sts_string>  { enum {iTypeId = DBR_STRING }; };
template <> struct DbrIntFromCtrlType<dbr_ctrl_short>  { enum {iTypeId = DBR_SHORT  }; };
template <> struct DbrIntFromCtrlType<dbr_ctrl_float>  { enum {iTypeId = DBR_FLOAT  }; };
template <> struct DbrIntFromCtrlType<dbr_ctrl_enum>   { enum {iTypeId = DBR_ENUM   }; };
template <> struct DbrIntFromCtrlType<dbr_ctrl_char>   { enum {iTypeId = DBR_CHAR   }; };
template <> struct DbrIntFromCtrlType<dbr_ctrl_long>   { enum {iTypeId = DBR_LONG   }; };
template <> struct DbrIntFromCtrlType<dbr_ctrl_double> { enum {iTypeId = DBR_DOUBLE }; };

extern const char* sDbrPrintfFormat[];

template <class TDbr> 
void printValue( TDbr* pValue ) 
{ 
    enum { iDbrType = DbrIntFromType<TDbr>::iTypeId };
    printf( sDbrPrintfFormat[iDbrType], *pValue );    
}

template <> inline
void printValue( const dbr_string_t* pValue )
{ 
    printf( "%s", (char*) pValue );
}

template <class TCtrl> // Default not to print precision field
void printPrecisionField(TCtrl& pvCtrlVal)  {}

template <> inline 
void printPrecisionField(const dbr_ctrl_double& pvCtrlVal)
{
    printf( "Precision: %d\n", pvCtrlVal.precision );
}

template <> inline 
void printPrecisionField(const dbr_ctrl_float& pvCtrlVal)
{
    printf( "Precision: %d\n", pvCtrlVal.precision );    
}

template <class TCtrl> inline
void printCtrlFields(TCtrl& pvCtrlVal)
{
    printPrecisionField(pvCtrlVal);
    printf( "Units: %s\n", pvCtrlVal.units );        
    
    enum { iDbrType = DbrIntFromCtrlType<TCtrl>::iTypeId };    

    std::string sFieldFmt = sDbrPrintfFormat[iDbrType];
    std::string sOutputString = std::string() +
      "Hi Disp : " + sFieldFmt + "  Lo Disp : " + sFieldFmt + "\n" + 
      "Hi Alarm: " + sFieldFmt + "  Hi Warn : " + sFieldFmt + "\n" +
      "Lo Warn : " + sFieldFmt + "  Lo Alarm: " + sFieldFmt + "\n" +
      "Hi Ctrl : " + sFieldFmt + "  Lo Ctrl : " + sFieldFmt + "\n";
     
    printf( sOutputString.c_str(), 
      pvCtrlVal.upper_disp_limit, pvCtrlVal.lower_disp_limit, 
      pvCtrlVal.upper_alarm_limit, pvCtrlVal.upper_warning_limit, 
      pvCtrlVal.lower_warning_limit, pvCtrlVal.lower_alarm_limit, 
      pvCtrlVal.upper_ctrl_limit, pvCtrlVal.lower_ctrl_limit
      );
    
    return;
}

template <> inline 
void printCtrlFields(const dbr_sts_string& pvCtrlVal) {}

template <> inline 
void printCtrlFields(const dbr_ctrl_enum& pvCtrlVal) 
{
    if ( pvCtrlVal.no_str > 0 )
    {
        printf( "EnumState Num: %d\n", pvCtrlVal.no_str );
        
        for (int iEnumState = 0; iEnumState < pvCtrlVal.no_str; iEnumState++ )
            printf( "EnumState[%d]: %s\n", iEnumState, pvCtrlVal.strs[iEnumState] );
    }
}

} // namespace EpicsDbrTools

} // namespace Pds

#endif
