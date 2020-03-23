////////////////////////////////////////////////////////////////////////////
// TypeSystem.h
// Copyright (C) 2014-2015 Katayama Hirofumi MZ.  All rights reserved.
////////////////////////////////////////////////////////////////////////////
// This file is part of CodeReverse. See file ReadMe.txt and License.txt.
////////////////////////////////////////////////////////////////////////////

#ifndef TYPESYSTEM_H_
#define TYPESYSTEM_H_

#include "Main.h"
#include <memory>
#include <map>
#include <unordered_map>
using std::make_shared;
using std::shared_ptr;

////////////////////////////////////////////////////////////////////////////
// CR_TypeFlags --- type flags

typedef unsigned long CR_TypeFlags;

static const CR_TypeFlags
    TF_VOID         = 0x00000001,
    TF_CHAR         = 0x00000002,
    TF_SHORT        = 0x00000004,
    TF_LONG         = 0x00000008,
    TF_LONGLONG     = 0x00000010,
    TF_INT          = 0x00000020,
    TF_FLOAT        = 0x00000080,
    TF_DOUBLE       = 0x00000100,
    TF_SIGNED       = 0,
    TF_UNSIGNED     = 0x00000200,
    TF_PTR64        = 0x00000400,
    TF_STRUCT       = 0x00000800,
    TF_UNION        = 0x00001000,
    TF_ENUM         = 0x00002000,
    TF_POINTER      = 0x00004000,
    TF_ARRAY        = 0x00008000,
    TF_FUNCTION     = 0x00010000,
    TF_INCOMPLETE   = 0x00020000,
    TF_CDECL        = 0,
    TF_STDCALL      = 0x00040000,
    TF_FASTCALL     = 0x00080000,
    TF_CONST        = 0x00200000,
    TF_COMPLEX      = 0x00400000,
    TF_IMAGINARY    = 0x00800000,
    TF_ATOMIC       = 0x01000000,
    TF_PTR32        = 0x02000000,
    TF_INACCURATE   = 0x04000000,
    TF_VECTOR       = 0x08000000,
    TF_BITFIELD     = 0x10000000,
    TF_ALIAS        = 0x20000000,
    TF_INT128       = 0x80000000;

////////////////////////////////////////////////////////////////////////////
// functions

CR_TypeFlags CrNormalizeTypeFlags(CR_TypeFlags flags);

std::string CrEscapeStringA2A(const std::string& str);
std::string CrEscapeStringW2A(const std::wstring& wstr);

std::string CrUnescapeStringA2A(const std::string& str);
std::wstring CrUnescapeStringA2W(const std::string& str);

std::string CrUnescapeCharA2A(const std::string& str);
std::wstring CrUnescapeCharL2W(const std::string& str);

std::string CrFormatBinary(const std::string& binary);
std::string CrParseBinary(const std::string& code);

std::string CrIndent(const std::string& str);
std::string CrTabToSpace(const std::string& str, size_t tabstop = 4);

////////////////////////////////////////////////////////////////////////////
// IDs

// CR_ID --- ID
typedef std::size_t             CR_ID;

// cr_invalid_id --- invalid ID
static const CR_ID cr_invalid_id = static_cast<CR_ID>(-1);

// CR_TypeID --- type ID
typedef CR_ID                   CR_TypeID;

// CR_FuncID --- function ID
typedef CR_ID                   CR_FuncID;

// CR_StructID --- struct or union ID
typedef CR_ID                   CR_StructID;

// CR_EnumID --- enum ID
typedef CR_ID                   CR_EnumID;

// CR_VarID --- variable ID
typedef CR_ID                   CR_VarID;

// a set of ID 
typedef CR_VecSet<CR_ID>        CR_IDSet;

// a set of type id
typedef CR_VecSet<CR_TypeID>    CR_TypeSet;

////////////////////////////////////////////////////////////////////////////
// CR_TypedValue --- typed value

struct CR_TypedValue {
    void *                          m_ptr;
    size_t                          m_size;
    CR_TypeID                       m_type_id;
    shared_ptr<unsigned long long>  m_addr;
    std::string                     m_text;
    std::string                     m_extra;

    CR_TypedValue() : m_ptr(NULL), m_size(0), m_type_id(cr_invalid_id) { }
    CR_TypedValue(CR_TypeID tid) : m_ptr(NULL), m_size(0), m_type_id(tid)
    { }
    CR_TypedValue(const void *ptr, size_t size);
    virtual ~CR_TypedValue();

    // copy
    CR_TypedValue(const CR_TypedValue& value);
    CR_TypedValue& operator=(const CR_TypedValue& value);

    // move
    CR_TypedValue(CR_TypedValue&& value);
    CR_TypedValue& operator=(CR_TypedValue&& value);

    bool empty() const { return m_size == 0 || m_ptr == NULL; }
    size_t size() const { return m_size; }
    void clear() {
        free(m_ptr);
        m_ptr = NULL;
        m_size = 0;
    }

    template <typename T_VALUE>
    T_VALUE& get() {
        assert(sizeof(T_VALUE) <= m_size);
        return *reinterpret_cast<T_VALUE *>(m_ptr);
    }

    template <typename T_VALUE>
    const T_VALUE& get() const {
        assert(sizeof(T_VALUE) <= m_size);
        return *reinterpret_cast<const T_VALUE *>(m_ptr);
    }

    template <typename T_VALUE>
    CR_TypedValue(CR_TypeID tid, const T_VALUE& value) :
        m_ptr(NULL), m_size(0), m_type_id(tid)
    {
        assign<T_VALUE>(value);
    }

    template <typename T_VALUE>
    void assign(const T_VALUE& v) {
        assign(&v, sizeof(T_VALUE));
        m_text = std::to_string(v);
    }

    template <typename T_VALUE>
    T_VALUE *get_at(size_t byte_index, size_t size = sizeof(T_VALUE)) {
        if (m_ptr) {
            if (byte_index + size <= m_size) {
                return reinterpret_cast<char *>(m_ptr) + byte_index;
            }
        }
        return NULL;
    }

    template <typename T_VALUE>
    const T_VALUE *get_at(size_t byte_index, size_t size = sizeof(T_VALUE)) const {
        if (m_ptr) {
            if (byte_index + size <= m_size) {
                return reinterpret_cast<const char *>(m_ptr) + byte_index;
            }
        }
        return NULL;
    }

    void assign(const void *ptr, size_t size);
};

////////////////////////////////////////////////////////////////////////////
// CR_LogType --- logical type

struct CR_LogType {
    CR_TypeFlags m_flags;           // type flags

    CR_ID        m_sub_id;          // sub ID.
    // m_sub_id means...
    // For TF_ALIAS:                A type ID (CR_TypeID).
    // For TF_POINTER:              A type ID (CR_TypeID).
    // For TF_ARRAY:                A type ID (CR_TypeID).
    // For TF_CONST:                A type ID (CR_TypeID).
    // For TF_CONST | TF_POINTER:   A type ID (CR_TypeID).
    // For TF_VECTOR:               A type ID (CR_TypeID).
    // For TF_FUNCTION:             A function ID (CR_FuncID).
    // For TF_STRUCT:               A struct ID (CR_StructID).
    // For TF_ENUM:                 An enum ID (CR_EnumID).
    // For TF_UNION:                A struct ID (CR_StructID).
    // Otherwise:                   Zero.

    size_t          m_count;        // for TF_ARRAY, TF_STRUCT, TF_UNION
                                    //     TF_VECTOR

    int             m_size;         // the size of type
    int             m_align;        // alignment requirement of type
    int             m_alignas;      // # of alignas(#) if specified
    bool            m_alignas_explicit;
    CR_Location     m_location;         // the location
    bool            m_is_macro;         // is it a macro?

    CR_LogType() : m_flags(0), m_sub_id(0), m_count(0), m_size(0), m_align(0),
                   m_alignas(0), m_alignas_explicit(false), m_is_macro(false) { }

    CR_LogType(CR_TypeFlags flags, int size, const CR_Location& location) :
        m_flags(flags), m_sub_id(0), m_count(0), m_size(size), m_align(size),
        m_alignas(0), m_alignas_explicit(false), m_location(location),
        m_is_macro(false) { }

    CR_LogType(CR_TypeFlags flags, int size, int align,
               const CR_Location& location) :
        m_flags(flags), m_sub_id(0), m_count(0), m_size(size), m_align(align),
            m_alignas(0), m_alignas_explicit(false), m_location(location),
                m_is_macro(false) { }

    CR_LogType(CR_TypeFlags flags, int size, int align, int alignas_,
               const CR_Location& location) :
        m_flags(flags), m_sub_id(0), m_count(0), m_size(size), m_align(align),
            m_alignas(alignas_), m_alignas_explicit(false),
                m_location(location), m_is_macro(false) { }

    // incomplete comparison 
    bool operator==(const CR_LogType& type) const;
    bool operator!=(const CR_LogType& type) const;
}; // struct CR_LogType

////////////////////////////////////////////////////////////////////////////
// CR_FuncParam --- parameter of function

struct CR_FuncParam {
    CR_TypeID       m_type_id;
    std::string     m_name;

    CR_FuncParam(CR_TypeID tid, const std::string& name) :
        m_type_id(tid), m_name(name) { }
};

////////////////////////////////////////////////////////////////////////////
// CR_LogFunc --- logical function

struct CR_LogFunc {
    bool                        m_ellipsis;
    CR_TypeID                   m_return_type;
    enum Convention {
        LFC_UNKNOWN, LFC_CDECL, LFC_STDCALL, LFC_FASTCALL
    } m_convention;
    std::vector<CR_FuncParam>   m_params;

    CR_LogFunc() : m_ellipsis(false), m_return_type(0),
                   m_convention(LFC_UNKNOWN) { }
}; // struct CR_LogFunc

////////////////////////////////////////////////////////////////////////////
// CR_StructMember --- accessible member

struct CR_StructMember {
    CR_TypeID       m_type_id;
    std::string     m_name;
    int             m_bit_offset;
    int             m_bits;
    CR_StructMember() = default;
    CR_StructMember(CR_TypeID tid, const std::string& name,
        int bit_offset = 0, int bits = -1) :
            m_type_id(tid), m_name(name),
                m_bit_offset(bit_offset), m_bits(bits) { }
};

bool operator==(const CR_StructMember& mem1, const CR_StructMember& mem2);
bool operator!=(const CR_StructMember& mem1, const CR_StructMember& mem2);

// NOTE: CR_AccessMember is same as CR_StructMember except meaning of m_bits.
typedef CR_StructMember CR_AccessMember;

////////////////////////////////////////////////////////////////////////////
// CR_LogStruct --- logical structure or union

struct CR_LogStruct {
    CR_TypeID               m_tid;              // type ID
    bool                    m_is_struct;        // struct or union?
    int                     m_pack;             // pack
    int                     m_align;            // alignment requirement
    int                     m_alignas;          // _Alignas(#)
    bool                    m_alignas_explicit;
    bool                    m_is_complete;      // is it complete?
    std::vector<CR_StructMember>    m_members;  // members

    CR_LogStruct(bool is_struct = true) :
        m_is_struct(is_struct), m_pack(8), m_align(0), m_alignas(0),
        m_alignas_explicit(false), m_is_complete(false) { }

    // incomplete comparison
    bool operator==(const CR_LogStruct& ls) const;
    bool operator!=(const CR_LogStruct& ls) const;

    bool empty() const { return m_members.empty(); }
    size_t size() const { return m_members.size(); }
}; // struct CR_LogStruct

////////////////////////////////////////////////////////////////////////////
// CR_LogEnum

struct CR_LogEnum {
    std::unordered_map<std::string, int>     m_mNameToValue;
    std::unordered_map<int, std::string>     m_mValueToName;

    bool empty() const {
        return m_mNameToValue.empty() && m_mValueToName.empty();
    }
}; // struct CR_LogEnum

////////////////////////////////////////////////////////////////////////////
// CR_LogVar --- logical variable

struct CR_LogVar {
    CR_TypedValue   m_typed_value;      // typed value
    CR_Location     m_location;         // the location
    bool            m_is_macro;         // is it a macro?
    CR_LogVar() : m_is_macro(false) { }
}; // struct CR_LogVar

////////////////////////////////////////////////////////////////////////////
// CR_Name2Name --- name mapping

struct CR_Name2Name {
    std::string     m_from;
    std::string     m_to;
    CR_Location     m_location;         // the location
};

////////////////////////////////////////////////////////////////////////////
// CR_Macro

struct CR_Macro {
    int                         m_num_params;
    std::vector<std::string>    m_params;
    std::string                 m_contents;
    CR_Location                 m_location;
    bool                        m_ellipsis;
    CR_Macro() : m_num_params(0), m_ellipsis(false) { }
};

typedef std::unordered_map<std::string,CR_Macro> CR_MacroSet;

////////////////////////////////////////////////////////////////////////////
// CR_NameScope --- universe of names, types, functions and variables etc.

class CR_NameScope {
public:
    CR_NameScope(shared_ptr<CR_ErrorInfo> error_info, bool is_64bit) :
        m_error_info(error_info), m_is_64bit(is_64bit)
    {
        Init();
    }

    CR_NameScope(const CR_NameScope& ns);
    CR_NameScope& operator=(const CR_NameScope& ns);

    // is 64-bit mode or not?
    bool Is64Bit() const { return m_is_64bit; }

    void Init();    // initialize
    void clear();

    bool LoadFromFiles(
        const std::string& prefix = "",
        const std::string& suffix = ".dat");

    bool LoadMacros(
        const std::string& prefix = "",
        const std::string& suffix = ".dat");

    void FixupLogFuncs(void);

    bool SaveToFiles(
        const std::string& prefix = "",
        const std::string& suffix = ".dat") const;

    CR_TypeID AddType(const std::string& name, const CR_LogType& lt) {
        auto tid = m_types.AddUnique(lt);
        if (!name.empty()) {
            m_mNameToTypeID[name] = tid;
            m_mTypeIDToName[tid] = name;
        }
        return tid;
    }

    CR_TypeID AddType(const std::string& name, CR_TypeFlags flags, int size,
                      const CR_Location& location)
    {
        return AddType(name, CR_LogType(flags, size, location));
    }

    CR_TypeID AddType(const std::string& name, CR_TypeFlags flags, int size,
                      int align, const CR_Location& location)
    {
        return AddType(name, CR_LogType(flags, size, align, location));
    }

    CR_TypeID AddType(const std::string& name, CR_TypeFlags flags, int size,
                      int align, int alignas_,
                      const CR_Location& location)
    {
        return AddType(name, CR_LogType(flags, size, align, alignas_, location));
    }

    // add alias type
    CR_TypeID AddAliasType(const std::string& name, CR_TypeID tid,
                           const CR_Location& location);
    // add alias macro type
    CR_TypeID AddAliasMacroType(const std::string& name, CR_TypeID tid,
                                const CR_Location& location);

    // add a variable
    CR_VarID AddVar(const std::string& name, CR_TypeID tid,
                    const CR_Location& location);
    // add a variable
    CR_VarID AddVar(const std::string& name, CR_TypeID tid,
                    const CR_Location& location, const CR_TypedValue& value);
    // add a variable
    CR_VarID AddVar(const std::string& name, CR_TypeID tid,
                    int value, const CR_Location& location);

    // add a variable
    CR_VarID AddVar(const std::string& name, const CR_LogType& type) {
        auto tid = m_types.AddUnique(type);
        if (!name.empty()) {
            m_mNameToTypeID[name] = tid;
            m_mTypeIDToName[tid] = name;
        }
        return AddVar(name, tid, type.m_location);
    }

    // add a constant type
    CR_TypeID AddConstType(CR_TypeID tid);

    // add a pointer type
    CR_TypeID AddPointerType(CR_TypeID tid, CR_TypeFlags flags,
                             const CR_Location& location);

    CR_TypeID AddArrayType(CR_TypeID tid, int count,
                           const CR_Location& location);

    CR_TypeID AddVectorType(
        const std::string& name, CR_TypeID tid, int vector_size,
        const CR_Location& location);

    // add function type
    CR_TypeID AddFuncType(const CR_LogFunc& lf, const CR_Location& location);

    // add struct type
    CR_TypeID AddStructType(const std::string& name, const CR_LogStruct& ls,
                            int alignas_, const CR_Location& location);

    // add union type
    CR_TypeID AddUnionType(const std::string& name, const CR_LogStruct& ls,
                           int alignas_, const CR_Location& location);

    CR_TypeID AddEnumType(const std::string& name, const CR_LogEnum& le,
                          const CR_Location& location);

    void AddTypeFlags(CR_TypeID tid, CR_TypeFlags flags) {
        LogType(tid).m_flags |= flags;
    }

    bool CompleteStructType(CR_TypeID tid, CR_StructID sid);

    bool CompleteUnionType(CR_TypeID tid, CR_StructID sid);

    bool CompleteType(CR_TypeID tid) {
        return CompleteType(tid, LogType(tid));
    }

    bool CompleteType(CR_TypeID tid, CR_LogType& type);

    void CompleteTypeInfo();

    void SetAlignas(CR_TypeID tid, int alignas_);

    //
    // calculations
    //
    void IntZero(CR_TypedValue& value1) const;
    void IntOne(CR_TypedValue& value1) const;
    bool IsZero(const CR_TypedValue& value1) const;
    bool IsNonZero(const CR_TypedValue& value1) const;

    CR_TypedValue Cast(CR_TypeID tid, const CR_TypedValue& value) const;
    CR_TypedValue StaticCast(CR_TypeID tid, const CR_TypedValue& value) const;
    CR_TypedValue ReinterpretCast(CR_TypeID tid, const CR_TypedValue& value) const;
    CR_TypeID MakeSigned(CR_TypeID tid) const;
    CR_TypeID MakeUnsigned(CR_TypeID tid) const;
    CR_TypeID MakeConst(CR_TypeID tid) const;

    // array[index]
    CR_TypedValue ArrayItem(const CR_TypedValue& array, size_t index) const;
    // struct.member
    CR_TypedValue Dot(const CR_TypedValue& struct_value, const std::string& name) const;
    // p->member
    CR_TypedValue Arrow(const CR_TypedValue& pointer_value, const std::string& name) const;
    // *p
    CR_TypedValue Asterisk(const CR_TypedValue& pointer_value) const;
    // &var
    CR_TypedValue Address(const CR_TypedValue& value) const;

    int GetIntValue(const CR_TypedValue& value) const;
    void SetIntValue(CR_TypedValue& value, int n) const;
    void SetValue(CR_TypedValue& value, CR_TypeID tid, const void *ptr,
                  size_t size) const;

    CR_TypedValue FConstant(const std::string& text, const std::string& extra);
    CR_TypedValue SConstant(const std::string& text, const std::string& extra);
    CR_TypedValue IConstant(const std::string& text, const std::string& extra);

    CR_TypedValue PConstant(CR_TypeID tid, const std::string& text, const std::string& extra);
    CR_TypedValue FConstant(CR_TypeID tid, const std::string& text, const std::string& extra);
    CR_TypedValue SConstant(CR_TypeID tid, const std::string& text, const std::string& extra);
    CR_TypedValue IConstant(CR_TypeID tid, const std::string& text, const std::string& extra);

    CR_TypedValue BiOp(CR_TypedValue& v1, CR_TypedValue& v2) const;
    CR_TypedValue BiOpInt(CR_TypedValue& v1, CR_TypedValue& v2) const;
    int CompareValue(const CR_TypedValue& v1, const CR_TypedValue& v2) const;

    // +
    CR_TypedValue Add(const CR_TypedValue& value1, const CR_TypedValue& value2) const;
    // -
    CR_TypedValue Sub(const CR_TypedValue& value1, const CR_TypedValue& value2) const;
    // *
    CR_TypedValue Mul(const CR_TypedValue& value1, const CR_TypedValue& value2) const;
    // /
    CR_TypedValue Div(const CR_TypedValue& value1, const CR_TypedValue& value2) const;
    // %
    CR_TypedValue Mod(const CR_TypedValue& value1, const CR_TypedValue& value2) const;

    // ~
    CR_TypedValue Not(const CR_TypedValue& value1) const;
    // - (unary)
    CR_TypedValue Minus(const CR_TypedValue& value1) const;

    // &
    CR_TypedValue And(const CR_TypedValue& value1, const CR_TypedValue& value2) const;
    // |
    CR_TypedValue Or(const CR_TypedValue& value1, const CR_TypedValue& value2) const;
    // ^
    CR_TypedValue Xor(const CR_TypedValue& value1, const CR_TypedValue& value2) const;

    // relational
    CR_TypedValue Eq(const CR_TypedValue& value1, const CR_TypedValue& value2) const;
    CR_TypedValue Ne(const CR_TypedValue& value1, const CR_TypedValue& value2) const;
    CR_TypedValue Gt(const CR_TypedValue& value1, const CR_TypedValue& value2) const;
    CR_TypedValue Lt(const CR_TypedValue& value1, const CR_TypedValue& value2) const;
    CR_TypedValue Ge(const CR_TypedValue& value1, const CR_TypedValue& value2) const;
    CR_TypedValue Le(const CR_TypedValue& value1, const CR_TypedValue& value2) const;

    // <<, >>
    CR_TypedValue Shl(const CR_TypedValue& value1, const CR_TypedValue& value2) const;
    CR_TypedValue Shr(const CR_TypedValue& value1, const CR_TypedValue& value2) const;

    // !
    CR_TypedValue LNot(const CR_TypedValue& value1) const;
    // &&
    CR_TypedValue LAnd(const CR_TypedValue& value1, const CR_TypedValue& value2) const;
    // ||
    CR_TypedValue LOr(const CR_TypedValue& value1, const CR_TypedValue& value2) const;

    //
    // getters
    //

    std::string StringFromValue(const CR_TypedValue& value) const;

    // get size of type
    int SizeOfType(CR_TypeID tid) const {
        if (tid == cr_invalid_id)
            return 0;
        auto& type = LogType(tid);
        return type.m_size;
    }

    std::string
    StringOfEnumTag(const std::string& name) const {
        std::string str = "enum ";
        if (name.size()) {
            assert(name.find("enum ") == 0);
            str += name.substr(5);
            str += ' ';
        }
        return str;
    }

    // get string of enum
    std::string StringOfEnum(const std::string& name, CR_EnumID eid) const;

    std::string
    StringOfStructTag(const std::string& name, const CR_LogStruct& s) const;

    // get string of struct or union
    std::string StringOfStruct(const std::string& name, CR_StructID sid) const;

    // get string of type
    std::string StringOfType(CR_TypeID tid, const std::string& name,
                           bool expand = true, bool no_convension = false) const;

    // get string of parameter list
    std::string StringOfParamList(const std::vector<CR_FuncParam>& params) const;

    // add members of struct or union
    void AddStructMembers(std::vector<CR_StructMember>& members,
        CR_StructID sid, const std::string& name = "", int bit_offset = 0) const;

    // add access members of a type
    void AddAccessMembers(std::vector<CR_AccessMember>& members, 
        CR_TypeID tid, const std::string& name,
        int bit_offset = 0, int bits = -1) const;

    CR_TypeID AddConstCharType();
    CR_TypeID AddConstUCharType();
    CR_TypeID AddConstShortType();
    CR_TypeID AddConstUShortType();
    CR_TypeID AddConstIntType();
    CR_TypeID AddConstUIntType();
    CR_TypeID AddConstLongType();
    CR_TypeID AddConstULongType();
    CR_TypeID AddConstLongLongType();
    CR_TypeID AddConstULongLongType();
    CR_TypeID AddConstFloatType();
    CR_TypeID AddConstDoubleType();
    CR_TypeID AddConstLongDoubleType();
    CR_TypeID AddConstStringType();
    CR_TypeID AddConstWStringType();
    CR_TypeID AddVoidPointerType();
    CR_TypeID AddConstVoidPointerType();
    CR_TypeID AddConstCharArray(size_t count);
    CR_TypeID AddConstWCharArray(size_t count);

    bool HasValue(const CR_TypedValue& value) const;

    long long GetLongLongValue(const CR_TypedValue& value) const;
    unsigned long long GetULongLongValue(const CR_TypedValue& value) const;
    long double GetLongDoubleValue(const CR_TypedValue& value) const;

    void SetLongLongValue(CR_TypedValue& value, long long n) const;
    void SetULongLongValue(CR_TypedValue& value, unsigned long long u) const;
    void SetLongDoubleValue(CR_TypedValue& value, long double ld) const;

    //
    // type judgements
    //
    CR_TypeID IsStringType(CR_TypeID tid) const;
    CR_TypeID IsWStringType(CR_TypeID tid) const;

    // is it a function type?
    bool IsFuncType(CR_TypeID tid) const;
    // is it a predefined type?
    bool IsPredefinedType(CR_TypeID tid) const;
    // is it an integer type?
    bool IsIntegralType(CR_TypeID tid) const;
    // is it a floating type?
    bool IsFloatingType(CR_TypeID tid) const;
    // is it an unsigned type?
    bool IsUnsignedType(CR_TypeID tid) const;
    // is it a pointer type?
    bool IsPointerType(CR_TypeID tid) const;
    // is it a constant type?
    bool IsConstantType(CR_TypeID tid) const;
    // is it an array type?
    bool IsArrayType(CR_TypeID tid) const;
    // is it a struct type?
    bool IsStructType(CR_TypeID tid) const;
    // is it a union type?
    bool IsUnionType(CR_TypeID tid) const;
    // is it a struct or union type?
    bool IsStructOrUnionType(CR_TypeID tid) const;
    // is it an enumeration type?
    bool IsEnumType(CR_TypeID tid) const;
    // is it a void type?
    bool IsVoidType(CR_TypeID tid) const;
    // is it a handle type?
    bool IsHandleType(CR_TypeID tid) const;

    //
    // ResolveAlias
    //

    CR_TypeID ResolveAlias(CR_TypeID tid) const;
    CR_TypeID ResolveAliasAndCV(CR_TypeID tid) const;
    CR_TypeID ResolvePointer(CR_TypeID tid) const;

    //
    // accessors
    //

    CR_TypeID TypeIDFromFlags(CR_TypeFlags flags) const;

    CR_TypeID TypeIDFromName(const std::string& name) const;

    std::string NameFromTypeID(CR_TypeID tid) const;

    std::string DecoratedTypeName(CR_TypeID tid) const;

    CR_LogType& LogType(CR_TypeID tid) {
        assert(tid < m_types.size());
        return m_types[tid];
    }

    const CR_LogType& LogType(CR_TypeID tid) const {
        assert(tid < m_types.size());
        return m_types[tid];
    }

    CR_LogVar& LogVar(CR_VarID vid) {
        assert(vid < m_vars.size());
        return m_vars[vid];
    }

    const CR_LogVar& LogVar(CR_VarID vid) const {
        assert(vid < m_vars.size());
        return m_vars[vid];
    }

    std::map<std::string, CR_TypeID>& MapNameToTypeID()
    { return m_mNameToTypeID; }

    const std::map<std::string, CR_TypeID>& MapNameToTypeID() const
    { return m_mNameToTypeID; }

    std::map<CR_TypeID, std::string>& MapTypeIDToName()
    { return m_mTypeIDToName; }

    const std::map<CR_TypeID, std::string>& MapTypeIDToName() const
    { return m_mTypeIDToName; }

    std::map<std::string, CR_VarID>& MapNameToVarID()
    { return m_mNameToVarID; }

    const std::map<std::string, CR_VarID>& MapNameToVarID() const
    { return m_mNameToVarID; }

    std::map<CR_VarID, std::string>& MapVarIDToName()
    { return m_mVarIDToName; }

    const std::map<CR_VarID, std::string>& MapVarIDToName() const
    { return m_mVarIDToName; }

    std::map<std::string, CR_Name2Name>& MapNameToName()
    { return m_mNameToName; }

    const std::map<std::string, CR_Name2Name>& MapNameToName() const
    { return m_mNameToName; }

    CR_LogStruct& LogStruct(CR_StructID sid) {
        assert(sid < m_structs.size());
        return m_structs[sid];
    }

    const CR_LogStruct& LogStruct(CR_StructID sid) const {
        assert(sid < m_structs.size());
        return m_structs[sid];
    }

    CR_LogFunc& LogFunc(CR_FuncID fid) {
        assert(fid < m_funcs.size());
        return m_funcs[fid];
    }

    const CR_LogFunc& LogFunc(CR_FuncID fid) const {
        assert(fid < m_funcs.size());
        return m_funcs[fid];
    }

    CR_LogEnum& LogEnum(CR_EnumID eid) {
        assert(eid < m_enums.size());
        return m_enums[eid];
    }

    const CR_LogEnum& LogEnum(CR_EnumID eid) const {
        assert(eid < m_enums.size());
        return m_enums[eid];
    }

          CR_VecSet<CR_LogType>& LogTypes()       { return m_types; }
    const CR_VecSet<CR_LogType>& LogTypes() const { return m_types; }

          CR_VecSet<CR_LogStruct>& LogStructs()       { return m_structs; }
    const CR_VecSet<CR_LogStruct>& LogStructs() const { return m_structs; }

          CR_VecSet<CR_LogFunc>& LogFuncs()       { return m_funcs; }
    const CR_VecSet<CR_LogFunc>& LogFuncs() const { return m_funcs; }

          CR_VecSet<CR_LogEnum>& LogEnums()       { return m_enums; }
    const CR_VecSet<CR_LogEnum>& LogEnums() const { return m_enums; }

          CR_VecSet<CR_LogVar>& LogVars()       { return m_vars; }
    const CR_VecSet<CR_LogVar>& LogVars() const { return m_vars; }

          CR_MacroSet& Macros()       { return m_macros; }
    const CR_MacroSet& Macros() const { return m_macros; }

          shared_ptr<CR_ErrorInfo>& ErrorInfo()       { return m_error_info; }
    const shared_ptr<CR_ErrorInfo>& ErrorInfo() const { return m_error_info; }

protected:
    shared_ptr<CR_ErrorInfo>            m_error_info;
    bool                                m_is_64bit;
    std::map<std::string, CR_TypeID>    m_mNameToTypeID;
    std::map<CR_TypeID, std::string>    m_mTypeIDToName;
    std::map<std::string, CR_VarID>     m_mNameToVarID;
    std::map<CR_VarID, std::string>     m_mVarIDToName;
    std::map<std::string, CR_TypeID>    m_mNameToFuncTypeID;
    CR_VecSet<CR_LogType>               m_types;
    CR_VecSet<CR_LogFunc>               m_funcs;
    CR_VecSet<CR_LogStruct>             m_structs;
    CR_VecSet<CR_LogEnum>               m_enums;
    CR_VecSet<CR_LogVar>                m_vars;
    std::map<std::string, CR_Name2Name> m_mNameToName;
    CR_MacroSet                         m_macros;

public:
    CR_TypeID                           m_void_type;
    CR_TypeID                           m_char_type;
    CR_TypeID                           m_short_type;
    CR_TypeID                           m_long_type;
    CR_TypeID                           m_long_long_type;
    CR_TypeID                           m_int_type;
    CR_TypeID                           m_uchar_type;
    CR_TypeID                           m_ushort_type;
    CR_TypeID                           m_ulong_type;
    CR_TypeID                           m_ulong_long_type;
    CR_TypeID                           m_uint_type;
    CR_TypeID                           m_float_type;
    CR_TypeID                           m_double_type;
    CR_TypeID                           m_long_double_type;
    CR_TypeID                           m_const_char_type;
    CR_TypeID                           m_const_uchar_type;
    CR_TypeID                           m_const_short_type;
    CR_TypeID                           m_const_ushort_type;
    CR_TypeID                           m_const_int_type;
    CR_TypeID                           m_const_uint_type;
    CR_TypeID                           m_const_long_type;
    CR_TypeID                           m_const_ulong_type;
    CR_TypeID                           m_const_long_long_type;
    CR_TypeID                           m_const_ulong_long_type;
    CR_TypeID                           m_const_float_type;
    CR_TypeID                           m_const_double_type;
    CR_TypeID                           m_const_long_double_type;
    CR_TypeID                           m_const_string_type;
    CR_TypeID                           m_const_wstring_type;
    CR_TypeID                           m_void_ptr_type;
    CR_TypeID                           m_const_void_ptr_type;
}; // class CR_NameScope

#endif  // ndef TYPESYSTEM_H_
