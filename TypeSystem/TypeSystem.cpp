////////////////////////////////////////////////////////////////////////////
// TypeSystem.cpp
// Copyright (C) 2014-2015 Katayama Hirofumi MZ.  All rights reserved.
////////////////////////////////////////////////////////////////////////////
// This file is part of CodeReverse. See file ReadMe.txt and License.txt.
////////////////////////////////////////////////////////////////////////////

#define _CRT_SECURE_NO_WARNINGS
#include "TypeSystem.h"
#include "TextToText.hpp"
#include "StringAssortNew.h"
#include <iostream>
#include <iomanip>
#include <fstream> 
#include <sstream>
#include <cctype>
#include <cmath>

CR_TypeFlags CrNormalizeTypeFlags(CR_TypeFlags flags) {
    if (flags & TF_INT) {
        // remove "int" if wordy
        if (flags & TF_SHORT)
            flags &= ~TF_INT;
        else if (flags & TF_LONG)
            flags &= ~TF_INT;
        else if (flags & TF_LONGLONG)
            flags &= ~TF_INT;
        else if (flags & TF_INT128)
            flags &= ~TF_INT;
    }
    if ((flags & TF_UNSIGNED) &&
        !(flags & (TF_CHAR | TF_SHORT | TF_LONG | TF_LONGLONG |
                   TF_INT128 | TF_INT)))
    {
        // add "int" for single "unsigned"
        flags |= TF_INT;
    }
    // add "int" if no type specified
    if (flags == 0)
        flags = TF_INT;
    // remove storage class specifiers
    return flags & ~TF_INCOMPLETE;
} // CrNormalizeTypeFlags

std::string CrEscapeStringA2A(const std::string& str) {
    std::string ret;
    size_t count = 0;
    const size_t siz = str.size();
    for (size_t i = 0; i < siz; ++i) {
        char ch = str[i];
        switch (ch) {
        case '\'': case '\"': case '\?': case '\\':
            ret += '\\';
            ret += ch;
            count += 2;
            break;
        case '\a':
            ret += '\\';
            ret += 'a';
            count += 2;
            break;
        case '\b':
            ret += '\\';
            ret += 'b';
            count += 2;
            break;
        case '\f':
            ret += '\\';
            ret += 'f';
            count += 2;
            break;
        case '\r':
            ret += '\\';
            ret += 'r';
            count += 2;
            break;
        case '\t':
            ret += '\\';
            ret += 't';
            count += 2;
            break;
        case '\v':
            ret += '\\';
            ret += 'v';
            count += 2;
            break;
        default:
            if (ch < 0x20) {
                int n = static_cast<unsigned char>(ch);
                std::stringstream ss;
                ss << "\\x" << std::hex <<
                    std::setfill('0') << std::setw(2) << n;
                ret += ss.str();
                ret += "\" \"";
                count += 4 + 3;
            } else {
                ret += ch;
                count++;
            }
        }
    }
    ret.resize(count);
    return "\"" + ret + "\"";
}

std::string CrEscapeStringW2A(const std::wstring& wstr) {
    std::string ret;
    size_t count = 0;
    const size_t siz = wstr.size();
    for (size_t i = 0; i < siz; ++i) {
        wchar_t ch = wstr[i];
        switch (ch) {
        case L'\'': case L'\"': case L'\?': case L'\\':
            ret += '\\';
            ret += char(ch);
            count += 2;
            break;
        case L'\a':
            ret += '\\';
            ret += 'a';
            count += 2;
            break;
        case L'\b':
            ret += '\\';
            ret += 'b';
            count += 2;
            break;
        case L'\f':
            ret += '\\';
            ret += 'f';
            count += 2;
            break;
        case L'\r':
            ret += '\\';
            ret += 'r';
            count += 2;
            break;
        case L'\t':
            ret += '\\';
            ret += 't';
            count += 2;
            break;
        case L'\v':
            ret += '\\';
            ret += 'v';
            count += 2;
            break;
        default:
            if (ch < 0x20 || ch >= 0x100) {
                int n = static_cast<unsigned char>(ch);
                std::stringstream ss;
                ss << "\\x" << std::hex <<
                    std::setfill('0') << std::setw(4) << n;
                ret += ss.str();
                ret += "\" \"";
                count += 6 + 3;
            } else {
                ret += char(ch);
                count++;
            }
        }
    }
    ret.resize(count);
    return "\"" + ret + "\"";
}

std::string CrUnescapeStringA2A(const std::string& str) {
    std::string ret;
    size_t siz = str.size();
    bool inside = false;
    for (size_t i = 0; i < siz; ++i) {
        char ch = str[i];
        if (ch == '\"') {
            if (inside) {
                if (++i < siz && str[i] == '\"') {
                    ret += '\"';
                } else {
                    --i;
                    inside = false;
                }
            } else {
                inside = true;
            }
            continue;
        }
        if (!inside) {
            if (!isspace(ch)) {
                //is_valid = false;
            }
            continue;
        }
        if (ch != '\\') {
            ret += ch;
            continue;
        }
        if (++i >= siz) {
            return ret;
        }
        ch = str[i];
        switch (ch) {
        case '\'': case '\"': case '\?': case '\\':
            ret += ch;
            break;
        case 'a': ret += '\a'; break;
        case 'b': ret += '\b'; break;
        case 'f': ret += '\f'; break;
        case 'n': ret += '\n'; break;
        case 'r': ret += '\r'; break;
        case 't': ret += '\t'; break;
        case 'v': ret += '\v'; break;
        case 'x':
            {
                std::string hex;
                if (++i < siz && isxdigit(str[i])) {
                    hex += str[i];
                    if (++i < siz && isxdigit(str[i])) {
                        hex += str[i];
                    } else {
                        --i;
                    }
                } else {
                    --i;
                    //is_valid = false; // invalid escape sequence
                }
                auto n = std::stoul(hex, NULL, 16);
                ret += static_cast<char>(n);
            }
            break;
        default:
            if ('0' <= ch && ch <= '7') {
                std::string oct;
                oct += ch;
                if (++i < siz && '0' <= str[i] && str[i] <= '7') {
                    oct += str[i];
                    if (++i < siz && '0' <= str[i] && str[i] <= '7') {
                        oct += str[i];
                    } else {
                        --i;
                    }
                } else {
                    --i;
                }
                auto n = std::stoul(oct, NULL, 8);
                ret += static_cast<char>(n);
            }
        }
    }
    return ret;
}

std::wstring CrUnescapeStringA2W(const std::string& str) {
    std::wstring ret;
    size_t siz = str.size();
    bool inside = false;
    for (size_t i = 0; i < siz; ++i) {
        char ch = str[i];
        if (ch == '\"') {
            if (inside) {
                if (++i < siz && str[i] == '\"') {
                    ret += L'\"';
                } else {
                    --i;
                    inside = false;
                }
            } else {
                inside = true;
            }
            continue;
        }
        if (!inside) {
            if (!isspace(ch)) {
                //is_valid = false;
            }
            continue;
        }
        if (ch != L'\\') {
            ret += wchar_t(ch);
            continue;
        }
        if (++i >= siz) {
            return ret;
        }
        ch = str[i];
        switch (ch) {
        case '\'': case '\"': case '\?': case '\\':
            ret += wchar_t(ch);
            break;
        case 'a': ret += L'\a'; break;
        case 'b': ret += L'\b'; break;
        case 'f': ret += L'\f'; break;
        case 'n': ret += L'\n'; break;
        case 'r': ret += L'\r'; break;
        case 't': ret += L'\t'; break;
        case 'v': ret += L'\v'; break;
        case 'x':
            {
                std::string hex;
                if (++i < siz && isxdigit(str[i])) {
                    hex += str[i];
                    if (++i < siz && isxdigit(str[i])) {
                        hex += str[i];
                        if (++i < siz && isxdigit(str[i])) {
                            hex += str[i];
                            if (++i < siz && isxdigit(str[i])) {
                                hex += str[i];
                            } else {
                                --i;
                            }
                        } else {
                            --i;
                        }
                    } else {
                        --i;
                    }
                } else {
                    --i;
                    //is_valid = false; // invalid escape sequence
                }
                auto n = std::stoul(hex, NULL, 16);
                ret += static_cast<wchar_t>(n);
            }
            break;
        default:
            if ('0' <= ch && ch <= '7') {
                std::string oct;
                oct += ch;
                if (++i < siz && '0' <= str[i] && str[i] <= '7') {
                    oct += str[i];
                    if (++i < siz && '0' <= str[i] && str[i] <= '7') {
                        oct += str[i];
                        if (++i < siz && '0' <= str[i] && str[i] <= '7') {
                            oct += str[i];
                            if (++i < siz && '0' <= str[i] && str[i] <= '7') {
                                oct += str[i];
                                if (++i < siz && '0' <= str[i] && str[i] <= '7') {
                                    oct += str[i];
                                } else {
                                    --i;
                                }
                            } else {
                                --i;
                            }
                        } else {
                            --i;
                        }
                    } else {
                        --i;
                    }
                } else {
                    --i;
                }
                auto n = std::stoul(oct, NULL, 8);
                ret += static_cast<wchar_t>(n);
            }
        }
    }
    return ret;
}

std::string CrUnescapeCharA2A(const std::string& str) {
    std::string ret;
    size_t siz = str.size();
    assert(siz && str[0] == '\'');
    for (size_t i = 1; i < siz; ++i) {
        char ch = str[i];
        if (ch == '\'') {
            break;
        }
        if (ch == '\\') {
            ch = str[++i];
            if (i == siz) {
                break;
            }
            switch (ch) {
            case '\'': case '\"': case '\?': case '\\':
                ret += wchar_t(ch);
                break;
            case 'a': ret += '\a'; break;
            case 'b': ret += '\b'; break;
            case 'f': ret += '\f'; break;
            case 'n': ret += '\n'; break;
            case 'r': ret += '\r'; break;
            case 't': ret += '\t'; break;
            case 'v': ret += '\v'; break;
            case 'x':
                {
                    std::string hex;
                    if (++i < siz && isxdigit(str[i])) {
                        hex += str[i];
                        if (++i < siz && isxdigit(str[i])) {
                            hex += str[i];
                        } else {
                            --i;
                        }
                    } else {
                        --i;
                    }
                    auto n = std::stoul(hex, NULL, 16);
                    ret += static_cast<char>(n);
                }
                break;

            default:
                if ('0' <= ch && ch <= '7') {
                    std::string oct;
                    oct += ch;
                    if (++i < siz && '0' <= str[i] && str[i] <= '7') {
                        oct += str[i];
                        if (++i < siz && '0' <= str[i] && str[i] <= '7') {
                            oct += str[i];
                        } else {
                            --i;
                        }
                    } else {
                        --i;
                    }
                    auto n = std::stoul(oct, NULL, 8);
                    ret += static_cast<char>(n);
                }
            }
        } else {
            ret += ch;
        }
    }
    return ret;
}

std::wstring CrUnescapeCharL2W(const std::string& str) {
    MAnsiToWide a2w(str.data(), str.size());
    std::wstring wstr(a2w.data(), a2w.size());

    std::wstring ret;
    size_t siz = wstr.size();
    assert(siz && wstr[0] == '\'');
    for (size_t i = 1; i < siz; ++i) {
        wchar_t ch = wstr[i];
        if (ch == L'\'') {
            break;
        }
        if (ch == L'\\') {
            ch = wstr[++i];
            if (i == siz) {
                break;
            }
            switch (ch) {
            case L'\'': case L'\"': case L'\?': case L'\\':
                ret += wchar_t(ch);
                break;
            case L'a': ret += L'\a'; break;
            case L'b': ret += L'\b'; break;
            case L'f': ret += L'\f'; break;
            case L'n': ret += L'\n'; break;
            case L'r': ret += L'\r'; break;
            case L't': ret += L'\t'; break;
            case L'v': ret += L'\v'; break;
            case L'x':
                {
                    std::wstring hex;
                    if (++i < siz && iswxdigit(str[i])) {
                        hex += str[i];
                        if (++i < siz && iswxdigit(str[i])) {
                            hex += str[i];
                            if (++i < siz && iswxdigit(str[i])) {
                                hex += str[i];
                                if (++i < siz && iswxdigit(str[i])) {
                                    hex += str[i];
                                } else {
                                    --i;
                                }
                            } else {
                                --i;
                            }
                        } else {
                            --i;
                        }
                    } else {
                        --i;
                    }
                    auto n = std::stoul(hex, NULL, 16);
                    ret += static_cast<wchar_t>(n);
                }
                break;

            default:
                if (L'0' <= ch && ch <= L'7') {
                    std::wstring oct;
                    oct += ch;
                    if (++i < siz && L'0' <= str[i] && str[i] <= L'7') {
                        oct += str[i];
                        if (++i < siz && L'0' <= str[i] && str[i] <= L'7') {
                            oct += str[i];
                            if (++i < siz && L'0' <= str[i] && str[i] <= L'7') {
                                oct += str[i];
                                if (++i < siz && L'0' <= str[i] && str[i] <= L'7') {
                                    oct += str[i];
                                    if (++i < siz && L'0' <= str[i] && str[i] <= L'7') {
                                        oct += str[i];
                                    } else {
                                        --i;
                                    }
                                } else {
                                    --i;
                                }
                            } else {
                                --i;
                            }
                        } else {
                            --i;
                        }
                    } else {
                        --i;
                    }
                    auto n = std::stoul(oct, NULL, 8);
                    ret += static_cast<wchar_t>(n);
                }
            }
        } else {
            ret += ch;
        }
    }
    return ret;
}

std::string CrFormatBinary(const std::string& binary) {
    std::string ret("0x");
    const size_t siz = binary.size();
    for (size_t i = 0; i < siz; ++i) {
        static const char hex[] = "0123456789ABCDEF";
        unsigned char b = binary[i];
        ret += hex[(b >> 4) & 0xF];
        ret += hex[b & 0xF];
    }
    return ret;
}

std::string CrParseBinary(const std::string& code) {
    std::string ret;
    assert(code.size() % 2 == 0);
    if (code.find("0x") == 0) {
        char ch;
        const size_t siz = code.size();
        for (size_t i = 2; i < siz; ) {
            if (isdigit(code[i])) {
                ch = code[i] - '0';
            } else if (isupper(code[i])) {
                ch = code[i] - ('A' - 10);
            } else {
                ch = code[i] - ('a' - 10);
            }
            ++i;
            ch <<= 4;
            if (isdigit(code[i])) {
                ch |= code[i] - '0';
            } else if (isupper(code[i])) {
                ch |= code[i] - ('A' - 10);
            } else {
                ch |= code[i] - ('a' - 10);
            }
            ++i;
            ret += ch;
        }
    }
    return ret;
}

std::string CrIndent(const std::string& str) {
    std::string text(str);
    katahiromz::replace_string(text, "\n", "\n\t");
    if (text.rfind("\n\t") == text.size() - 2) {
        text.resize(text.size() - 1);
    }
    return "\t" + text;
}

std::string CrTabToSpace(const std::string& str, size_t tabstop/* = 4*/) {
    std::string tab(tabstop, ' ');
    std::string text(str);
    katahiromz::replace_string(text, std::string("\t"), tab);
    return text;
}

////////////////////////////////////////////////////////////////////////////
// CR_TypedValue

CR_TypedValue::CR_TypedValue(const void *ptr, size_t size) :
    m_ptr(NULL), m_size(0), m_type_id(cr_invalid_id)
{
    assign(ptr, size);
}

CR_TypedValue::CR_TypedValue(const CR_TypedValue& value) :
    m_ptr(NULL), m_size(0), m_type_id(cr_invalid_id)
{
    m_type_id = value.m_type_id;
    m_text = value.m_text;
    m_extra = value.m_extra;
    m_addr = value.m_addr;
    assign(value.m_ptr, value.m_size);
}

CR_TypedValue& CR_TypedValue::operator=(const CR_TypedValue& value) {
    if (this != &value) {
        m_type_id = value.m_type_id;
        m_text = value.m_text;
        m_extra = value.m_extra;
        m_addr = value.m_addr;
        assign(value.m_ptr, value.m_size);
    }
    return *this;
}

CR_TypedValue::CR_TypedValue(CR_TypedValue&& value) : m_ptr(NULL), m_size(0) {
    if (this != &value) {
        std::swap(m_ptr, value.m_ptr);
        std::swap(m_size, value.m_size);
        m_type_id = value.m_type_id;
        m_addr = std::move(value.m_addr);
        std::swap(m_text, value.m_text);
        std::swap(m_extra, value.m_extra);
    }
}

CR_TypedValue& CR_TypedValue::operator=(CR_TypedValue&& value) {
    if (this != &value) {
        std::swap(m_ptr, value.m_ptr);
        std::swap(m_size, value.m_size);
        m_type_id = value.m_type_id;
        m_addr = std::move(value.m_addr);
        std::swap(m_text, value.m_text);
        std::swap(m_extra, value.m_extra);
    }
    return *this;
}

void CR_TypedValue::assign(const void *ptr, size_t size) {
    if (ptr == NULL || size == 0) {
        free(m_ptr);
        m_ptr = NULL;
        m_size = size;
    } else if (ptr == m_ptr) {
        if (m_size < size) {
            m_ptr = realloc(m_ptr, size + 1);
            if (m_ptr == NULL) {
                m_size = 0;
                throw std::bad_alloc();
            }
        }
        m_size = size;
    } else {
        m_ptr = realloc(m_ptr, size + 1);
        if (m_ptr) {
            memmove(m_ptr, ptr, size);
            m_size = size;
        } else {
            m_size = 0;
            throw std::bad_alloc();
        }
    }
}

/*virtual*/ CR_TypedValue::~CR_TypedValue() {
    free(m_ptr);
    m_ptr = NULL;
    m_size = 0;
}

////////////////////////////////////////////////////////////////////////////
// CR_LogType

bool CR_LogType::operator==(const CR_LogType& type) const {
    return m_flags == type.m_flags &&
           m_sub_id == type.m_sub_id &&
           m_count == type.m_count &&
           m_alignas == type.m_alignas;
}

bool CR_LogType::operator!=(const CR_LogType& type) const {
    return m_flags != type.m_flags ||
           m_sub_id != type.m_sub_id ||
           m_count != type.m_count ||
           m_alignas != type.m_alignas;
}

////////////////////////////////////////////////////////////////////////////
// CR_AccessMember

bool operator==(const CR_AccessMember& mem1, const CR_AccessMember& mem2) {
    return
        mem1.m_type_id      == mem2.m_type_id &&
        mem1.m_name         == mem2.m_name &&
        mem1.m_bit_offset   == mem2.m_bit_offset &&
        mem1.m_bits         == mem2.m_bits;

}

bool operator!=(const CR_AccessMember& mem1, const CR_AccessMember& mem2) {
    return
        mem1.m_type_id      != mem2.m_type_id ||
        mem1.m_name         != mem2.m_name ||
        mem1.m_bit_offset   != mem2.m_bit_offset ||
        mem1.m_bits         != mem2.m_bits;
}

////////////////////////////////////////////////////////////////////////////
// CR_LogStruct

bool CR_LogStruct::operator==(const CR_LogStruct& ls) const {
    return m_is_struct  == ls.m_is_struct &&
           m_pack       == ls.m_pack &&
           m_align      == ls.m_align &&
           m_members    == ls.m_members;
}

bool CR_LogStruct::operator!=(const CR_LogStruct& ls) const {
    return m_is_struct  != ls.m_is_struct ||
           m_pack       != ls.m_pack ||
           m_align      != ls.m_align ||
           m_members    != ls.m_members;
}

////////////////////////////////////////////////////////////////////////////
// CR_NameScope

CR_NameScope::CR_NameScope(const CR_NameScope& ns) :
    m_error_info(ns.m_error_info),
    m_is_64bit(ns.m_is_64bit),
    m_mNameToTypeID(ns.m_mNameToTypeID),
    m_mTypeIDToName(ns.m_mTypeIDToName),
    m_mNameToVarID(ns.m_mNameToVarID),
    m_mVarIDToName(ns.m_mVarIDToName),
    m_mNameToFuncTypeID(ns.m_mNameToFuncTypeID),
    m_types(ns.m_types),
    m_funcs(ns.m_funcs),
    m_structs(ns.m_structs),
    m_enums(ns.m_enums),
    m_vars(ns.m_vars),
    m_void_type(ns.m_void_type),
    m_char_type(ns.m_char_type),
    m_short_type(ns.m_short_type),
    m_long_type(ns.m_long_type),
    m_long_long_type(ns.m_long_long_type),
    m_int_type(ns.m_int_type),
    m_uchar_type(ns.m_uchar_type),
    m_ushort_type(ns.m_ushort_type),
    m_ulong_type(ns.m_ulong_type),
    m_ulong_long_type(ns.m_ulong_long_type),
    m_uint_type(ns.m_uint_type),
    m_float_type(ns.m_float_type),
    m_double_type(ns.m_double_type),
    m_long_double_type(ns.m_long_double_type),
    m_const_char_type(ns.m_const_char_type),
    m_const_uchar_type(ns.m_const_uchar_type),
    m_const_short_type(ns.m_const_short_type),
    m_const_ushort_type(ns.m_const_ushort_type),
    m_const_int_type(ns.m_const_int_type),
    m_const_uint_type(ns.m_const_uint_type),
    m_const_long_type(ns.m_const_long_type),
    m_const_ulong_type(ns.m_const_ulong_type),
    m_const_long_long_type(ns.m_const_long_long_type),
    m_const_ulong_long_type(ns.m_const_ulong_long_type),
    m_const_float_type(ns.m_const_float_type),
    m_const_double_type(ns.m_const_double_type),
    m_const_long_double_type(ns.m_const_long_double_type),
    m_const_string_type(ns.m_const_string_type),
    m_const_wstring_type(ns.m_const_wstring_type),
    m_void_ptr_type(ns.m_void_ptr_type),
    m_const_void_ptr_type(ns.m_const_void_ptr_type)
    { }

CR_NameScope& CR_NameScope::operator=(const CR_NameScope& ns) {
    m_error_info = ns.m_error_info;
    m_is_64bit = ns.m_is_64bit;
    m_mNameToTypeID = ns.m_mNameToTypeID;
    m_mTypeIDToName = ns.m_mTypeIDToName;
    m_mNameToVarID = ns.m_mNameToVarID;
    m_mVarIDToName = ns.m_mVarIDToName;
    m_mNameToFuncTypeID = ns.m_mNameToFuncTypeID;
    m_types = ns.m_types;
    m_funcs = ns.m_funcs;
    m_structs = ns.m_structs;
    m_enums = ns.m_enums;
    m_vars = ns.m_vars;
    m_void_type = ns.m_void_type;
    m_char_type = ns.m_char_type;
    m_short_type = ns.m_short_type;
    m_long_type = ns.m_long_type;
    m_long_long_type = ns.m_long_long_type;
    m_int_type = ns.m_int_type;
    m_uchar_type = ns.m_uchar_type;
    m_ushort_type = ns.m_ushort_type;
    m_ulong_type = ns.m_ulong_type;
    m_ulong_long_type = ns.m_ulong_long_type;
    m_uint_type = ns.m_uint_type;
    m_float_type = ns.m_float_type;
    m_double_type = ns.m_double_type;
    m_long_double_type = ns.m_long_double_type;
    m_const_char_type = ns.m_const_char_type;
    m_const_uchar_type = ns.m_const_uchar_type;
    m_const_short_type = ns.m_const_short_type;
    m_const_ushort_type = ns.m_const_ushort_type;
    m_const_int_type = ns.m_const_int_type;
    m_const_uint_type = ns.m_const_uint_type;
    m_const_long_type = ns.m_const_long_type;
    m_const_ulong_type = ns.m_const_ulong_type;
    m_const_long_long_type = ns.m_const_long_long_type;
    m_const_ulong_long_type = ns.m_const_ulong_long_type;
    m_const_float_type = ns.m_const_float_type;
    m_const_double_type = ns.m_const_double_type;
    m_const_long_double_type = ns.m_const_long_double_type;
    m_const_string_type = ns.m_const_string_type;
    m_const_wstring_type = ns.m_const_wstring_type;
    m_void_ptr_type = ns.m_void_ptr_type;
    m_const_void_ptr_type = ns.m_const_void_ptr_type;
    return *this;
}

void CR_NameScope::Init() {
    CR_Location location("(predefined)", 0);

    m_void_type = AddType("void", TF_VOID, 0, location);

    m_char_type = AddType("char", TF_CHAR, sizeof(char), location);
    m_short_type = AddType("short", TF_SHORT, sizeof(short), location);
    m_long_type = AddType("long", TF_LONG, sizeof(long), location);
    m_long_long_type = AddType("long long", TF_LONGLONG, sizeof(long long), location);
    AddAliasType("__int64", m_long_long_type, location);

    m_int_type = AddType("int", TF_INT, sizeof(int), location);

    m_uchar_type = AddType("unsigned char", TF_UNSIGNED | TF_CHAR, sizeof(char), location);
    m_ushort_type = AddType("unsigned short", TF_UNSIGNED | TF_SHORT, sizeof(short), location);
    m_ulong_type = AddType("unsigned long", TF_UNSIGNED | TF_LONG, sizeof(long), location);
    m_ulong_long_type = AddType("unsigned long long", TF_UNSIGNED | TF_LONGLONG, sizeof(long long), location);
    AddAliasType("unsigned __int64", m_ulong_long_type, location);

    #ifdef __GNUC__
        AddType("__int128", TF_INT128, 128 / 8, location);
        AddType("unsigned __int128", TF_UNSIGNED | TF_INT128, 128 / 8, location);
    #endif

    m_uint_type = AddType("unsigned int", TF_UNSIGNED | TF_INT, sizeof(int), location);

    m_float_type = AddType("float", TF_FLOAT, sizeof(float), location);
    m_double_type = AddType("double", TF_DOUBLE, sizeof(double), location);

    #ifdef __GNUC__
        if (m_is_64bit)
            m_long_double_type = AddType("long double", TF_LONG | TF_DOUBLE, 16, 16, 16, location);
        else
            m_long_double_type = AddType("long double", TF_LONG | TF_DOUBLE, 12, 4, 4, location);
    #else
        m_long_double_type = AddType("long double", TF_LONG | TF_DOUBLE, sizeof(long double), location);
    #endif

    m_const_char_type = AddConstCharType();
    m_const_uchar_type = AddConstUCharType();
    m_const_short_type = AddConstShortType();
    m_const_ushort_type = AddConstUShortType();
    m_const_int_type = AddConstIntType();
    m_const_uint_type = AddConstUIntType();
    m_const_long_type = AddConstLongType();
    m_const_ulong_type = AddConstULongType();
    m_const_long_long_type = AddConstLongLongType();
    m_const_ulong_long_type = AddConstULongLongType();
    m_const_float_type = AddConstFloatType();
    m_const_double_type = AddConstDoubleType();
    m_const_long_double_type = AddConstLongDoubleType();
    m_const_string_type = AddConstStringType();
    m_const_wstring_type = AddConstWStringType();
    m_void_ptr_type = AddVoidPointerType();
    m_const_void_ptr_type = AddConstVoidPointerType();
}

CR_TypeID CR_NameScope::AddAliasType(
    const std::string& name, CR_TypeID tid, const CR_Location& location)
{
    assert(!name.empty());
    CR_LogType type1;
    auto& type2 = LogType(tid);
    if (type2.m_flags & TF_INCOMPLETE) {
        type1.m_flags = TF_ALIAS | TF_INCOMPLETE;
        type1.m_alignas = type2.m_alignas;
    } else {
        type1.m_flags = TF_ALIAS;
        type1.m_size = type2.m_size;
        type1.m_align = type2.m_align;
        type1.m_alignas = type2.m_alignas;
    }
    #ifdef __GNUC__
        if (type2.m_flags & TF_INACCURATE) {
            type1.m_flags |= TF_INACCURATE;
        }
    #endif
    type1.m_sub_id = tid;
    type1.m_count = type2.m_count;
    type1.m_location = location;
    tid = m_types.insert(type1);
    m_mNameToTypeID[name] = tid;
    m_mTypeIDToName[tid] = name;
    return tid;
}

CR_TypeID CR_NameScope::AddAliasMacroType(
    const std::string& name, CR_TypeID tid, const CR_Location& location)
{
    assert(!name.empty());
    CR_LogType type1;
    auto& type2 = LogType(tid);
    if (type2.m_flags & TF_INCOMPLETE) {
        type1.m_flags = TF_ALIAS | TF_INCOMPLETE;
        type1.m_alignas = type2.m_alignas;
    } else {
        type1.m_flags = TF_ALIAS;
        type1.m_size = type2.m_size;
        type1.m_align = type2.m_align;
        type1.m_alignas = type2.m_alignas;
    }
    #ifdef __GNUC__
        if (type2.m_flags & TF_INACCURATE) {
            type1.m_flags |= TF_INACCURATE;
        }
    #endif
    type1.m_sub_id = tid;
    type1.m_count = type2.m_count;
    type1.m_location = location;
    type1.m_is_macro = true;
    tid = m_types.insert(type1);
    m_mNameToTypeID[name] = tid;
    m_mTypeIDToName[tid] = name;
    return tid;
}

CR_VarID CR_NameScope::AddVar(
    const std::string& name, CR_TypeID tid, const CR_Location& location)
{
    if (tid == cr_invalid_id) {
        return cr_invalid_id;
    }
    CR_LogVar var;
    var.m_typed_value.m_type_id = tid;
    var.m_typed_value.m_size = SizeOfType(tid);
    var.m_location = location;
    auto vid = m_vars.insert(var);
    m_mVarIDToName.emplace(vid, name);
    if (name.size()) {
        m_mNameToVarID.emplace(name, vid);
    }
    return vid;
}

CR_VarID CR_NameScope::AddVar(
    const std::string& name, CR_TypeID tid,
    const CR_Location& location, const CR_TypedValue& value)
{
    if (tid == cr_invalid_id) {
        return cr_invalid_id;
    }
    CR_LogVar var;
    var.m_typed_value = value;
    var.m_location = location;
    auto vid = m_vars.insert(var);
    m_mVarIDToName.emplace(vid, name);
    if (name.size()) {
        m_mNameToVarID.emplace(name, vid);
    }
    return vid;
}

CR_VarID CR_NameScope::AddVar(
    const std::string& name, CR_TypeID tid, int value,
    const CR_Location& location)
{
    if (tid == cr_invalid_id) {
        return cr_invalid_id;
    }
    CR_LogVar var;
    var.m_typed_value = CR_TypedValue(tid);
    var.m_typed_value.assign<int>(value);
    var.m_location = location;
    auto vid = m_vars.insert(var);
    m_mVarIDToName.emplace(vid, name);
    if (name.size()) {
        m_mNameToVarID.emplace(name, vid);
    }
    return vid;
}

CR_TypeID CR_NameScope::AddConstType(CR_TypeID tid) {
    if (tid == cr_invalid_id) {
        return tid;
    }
    CR_LogType type1;
    auto& type2 = LogType(tid);
    if (type2.m_flags & TF_CONST) {
        // it is already a constant
        return tid;
    }
    if (type2.m_flags & (TF_ARRAY | TF_VECTOR)) {
        // A constant of array is an array of constant
        tid = AddConstType(type2.m_sub_id);
        tid = AddArrayType(tid, int(type2.m_count), type2.m_location);
        return tid;
    }
    if (type2.m_flags & TF_POINTER) {
        tid = AddConstType(type2.m_sub_id);
        tid = AddPointerType(tid, TF_CONST, type2.m_location);
        return tid;
    }
    if (type2.m_flags & TF_INCOMPLETE) {
        type1.m_flags = TF_CONST | TF_INCOMPLETE;
    } else {
        type1.m_flags = TF_CONST;
        type1.m_size = type2.m_size;
        type1.m_align = type2.m_align;
    }
    if (type2.m_flags & TF_BITFIELD) {
        type1.m_flags |= TF_BITFIELD;
    }
    type1.m_sub_id = tid;
    auto tid2 = m_types.AddUnique(type1);
    auto name = NameFromTypeID(tid);
    if (name.size()) {
        name = std::string("const ") + name;
        m_mNameToTypeID[name] = tid2;
        m_mTypeIDToName[tid2] = name;
    }
    return tid2;
}

CR_TypeID CR_NameScope::AddPointerType(
    CR_TypeID tid, CR_TypeFlags flags, const CR_Location& location)
{
    if (tid == cr_invalid_id) {
        return tid;
    }
    CR_LogType type1;
    type1.m_flags = TF_POINTER | flags;
    if (Is64Bit()) {
        type1.m_flags &= ~TF_PTR64;
    } else {
        type1.m_flags &= ~TF_PTR32;
    }
    type1.m_sub_id = tid;
    if (flags & TF_PTR64) {
        type1.m_align = type1.m_size = 8;
    } else if (flags & TF_PTR32) {
        type1.m_align = type1.m_size = 4;
    } else {
        type1.m_align = type1.m_size = (Is64Bit() ? 8 : 4);
    }
    type1.m_location = location;
    auto tid2 = m_types.AddUnique(type1);
    auto type2 = LogType(tid);
    auto name = NameFromTypeID(tid);
    if (!name.empty()) {
        if (!(type2.m_flags & TF_FUNCTION)) {
            if (flags & TF_CONST) {
                if (flags & TF_PTR32) {
                    name += "* __ptr32 const ";
                } else if (flags & TF_PTR64) {
                    name += "* __ptr64 const ";
                } else {
                    name += "* const ";
                }
            } else {
                if (flags & TF_PTR32) {
                    name += "* __ptr32 ";
                } else if (flags & TF_PTR64) {
                    name += "* __ptr64 ";
                } else {
                    name += "* ";
                }
            }
        }
        m_mNameToTypeID[name] = tid2;
        m_mTypeIDToName[tid2] = name;
    }
    return tid2;
} // AddPointerType

CR_TypeID CR_NameScope::AddArrayType(
    CR_TypeID tid, int count, const CR_Location& location)
{
    if (tid == cr_invalid_id) {
        return tid;
    }
    CR_LogType type1;
    auto& type2 = LogType(tid);
    if (type2.m_flags & TF_INCOMPLETE) {
        type1.m_flags = TF_ARRAY | TF_INCOMPLETE;
    } else {
        type1.m_flags = TF_ARRAY;
        type1.m_size = type2.m_size * count;
        type1.m_align = type2.m_align;
        if (type1.m_alignas < type2.m_alignas) {
            type1.m_alignas = type2.m_alignas;
            type1.m_align = type2.m_alignas;
        }
    }
    if (type2.m_flags & TF_BITFIELD) {
        type1.m_flags |= TF_BITFIELD;
    }
    type1.m_sub_id = tid;
    type1.m_count = count;
    type1.m_location = location;
    if (count) {
        tid = m_types.AddUnique(type1);
    } else {
        tid = m_types.insert(type1);
    }
    m_mTypeIDToName[tid] = "";
    return tid;
} // AddArrayType

CR_TypeID CR_NameScope::AddVectorType(
    const std::string& name, CR_TypeID tid, int vector_size,
    const CR_Location& location)
{
    if (tid == cr_invalid_id) {
        return tid;
    }
    CR_LogType type1;
    auto& type2 = LogType(tid);
    if (type2.m_flags & TF_INCOMPLETE) {
        type1.m_flags = TF_VECTOR | TF_INCOMPLETE;
    } else {
        type1.m_flags = TF_VECTOR;
        type1.m_count = vector_size / type2.m_size;
    }
    type1.m_size = vector_size;
    type1.m_align = vector_size;
    type1.m_alignas = vector_size;
    if (type2.m_flags & TF_BITFIELD) {
        type1.m_flags |= TF_BITFIELD;
    }
    type1.m_sub_id = tid;
    type1.m_location = location;
    tid = m_types.AddUnique(type1);
    m_mNameToTypeID[name] = tid;
    m_mTypeIDToName[tid] = name;
    return tid;
} // AddVectorSize

CR_TypeID CR_NameScope::AddFuncType(
    const CR_LogFunc& lf, const CR_Location& location)
{
    CR_LogFunc func(lf);
    if (func.m_params.size() == 1 && func.m_params[0].m_type_id == 0) {
        // parameter list is void
        func.m_params.clear();
    }
    auto fid = m_funcs.insert(func);
    CR_LogType type1;
    type1.m_flags = TF_FUNCTION;
    type1.m_sub_id = fid;
    type1.m_size = 1;
    type1.m_align = 1;
    type1.m_location = location;
    CR_TypeID tid1 = m_types.AddUnique(type1);
    m_mTypeIDToName[tid1] = "";
    return tid1;
} // AddFuncType

CR_TypeID CR_NameScope::AddStructType(
    const std::string& name, const CR_LogStruct& ls,
    int alignas_, const CR_Location& location)
{
    CR_LogType type1;
    if (name.empty()) {     // name is empty
        CR_StructID sid = m_structs.insert(ls);
        type1.m_flags = TF_STRUCT | TF_INCOMPLETE;
        type1.m_sub_id = sid;
        type1.m_count = ls.m_members.size();
        type1.m_alignas = alignas_;
        type1.m_location = location;
        CR_TypeID tid2 = m_types.AddUnique(type1);
        LogStruct(sid).m_tid = tid2;
        m_mTypeIDToName[tid2] = name;
        CompleteStructType(tid2, sid);
        return tid2;
    }
    auto it = m_mNameToTypeID.find("struct " + name);
    if (it == m_mNameToTypeID.end()) {  // name not found
        CR_StructID sid = m_structs.insert(ls);
        type1.m_flags = TF_STRUCT | TF_INCOMPLETE;
        type1.m_sub_id = sid;
        type1.m_count = ls.m_members.size();
        type1.m_alignas = alignas_;
        type1.m_location = location;
        CR_TypeID tid2 = m_types.AddUnique(type1);
        LogStruct(sid).m_tid = tid2;
        m_mNameToTypeID["struct " + name] = tid2;
        m_mTypeIDToName[tid2] = "struct " + name;
        CompleteStructType(tid2, sid);
        return tid2;
    } else {    // name was found
        CR_TypeID tid2 = it->second;
        assert(m_types[tid2].m_flags & TF_STRUCT);
        if (ls.m_members.size()) {
            // overwrite the definition if type list not empty
            auto& type1 = LogType(tid2);
            type1.m_count = ls.m_members.size();
            type1.m_alignas = alignas_;
            type1.m_location = location;
            CR_StructID sid = type1.m_sub_id;
            LogStruct(sid) = ls;
            LogStruct(sid).m_tid = tid2;
            CompleteStructType(tid2, sid);
        }
        return tid2;
    }
} // AddStructType

CR_TypeID CR_NameScope::AddUnionType(
    const std::string& name, const CR_LogStruct& ls,
    int alignas_, const CR_Location& location)
{
    CR_LogType type1;
    if (name.empty()) { // name is empty
        CR_StructID sid = m_structs.insert(ls);
        type1.m_flags = TF_UNION | TF_INCOMPLETE;
        type1.m_sub_id = sid;
        type1.m_count = ls.m_members.size();
        type1.m_alignas = alignas_;
        type1.m_location = location;
        CR_TypeID tid1 = m_types.AddUnique(type1);
        LogStruct(sid).m_tid = tid1;
        m_mTypeIDToName[tid1] = name;
        CompleteUnionType(tid1, sid);
        return tid1;
    }
    auto it = m_mNameToTypeID.find("union " + name);
    if (it == m_mNameToTypeID.end()) {  // name not found
        CR_StructID sid = m_structs.insert(ls);
        type1.m_flags = TF_UNION | TF_INCOMPLETE;
        type1.m_sub_id = sid;
        type1.m_count = ls.m_members.size();
        type1.m_alignas = alignas_;
        type1.m_location = location;
        CR_TypeID tid1 = m_types.AddUnique(type1);
        LogStruct(sid).m_tid = tid1;
        m_mNameToTypeID["union " + name] = tid1;
        m_mTypeIDToName[tid1] = "union " + name;
        CompleteUnionType(tid1, sid);
        return tid1;
    } else {    // name was found
        CR_TypeID tid2 = it->second;
        assert(m_types[tid2].m_flags & TF_UNION);
        if (ls.m_members.size()) {
            // overwrite the definition if type list not empty
            auto& type1 = LogType(tid2);
            type1.m_count = ls.m_members.size();
            type1.m_alignas = alignas_;
            type1.m_location = location;
            CR_StructID sid = type1.m_sub_id;
            LogStruct(sid) = ls;
            LogStruct(sid).m_tid = tid2;
            CompleteUnionType(tid2, sid);
        }
        return tid2;
    }
} // AddUnionType

CR_TypeID CR_NameScope::AddEnumType(
    const std::string& name, const CR_LogEnum& le, const CR_Location& location)
{
    CR_LogType type1;
    if (name.empty()) {     // name is empty
        CR_EnumID eid = m_enums.insert(le);
        type1.m_flags = TF_ENUM;
        #ifdef __GNUC__
            type1.m_flags |= TF_INACCURATE;
        #endif
        type1.m_sub_id = eid;
        type1.m_size = type1.m_align = 4;
        type1.m_location = location;
        CR_TypeID tid1 = m_types.AddUnique(type1);
        m_mTypeIDToName[tid1] = name;
        return tid1;
    }
    auto it = m_mNameToTypeID.find("enum " + name);
    if (it == m_mNameToTypeID.end()) {  // name not found
        CR_EnumID eid = m_enums.insert(le);
        type1.m_flags = TF_ENUM;
        #ifdef __GNUC__
            type1.m_flags |= TF_INACCURATE;
        #endif
        type1.m_sub_id = eid;
        type1.m_size = type1.m_align = 4;
        type1.m_location = location;
        CR_TypeID tid1 = m_types.AddUnique(type1);
        m_mNameToTypeID["enum " + name] = tid1;
        m_mTypeIDToName[tid1] = "enum " + name;
        return tid1;
    } else {    // name was found
        CR_TypeID tid1 = it->second;
        auto& type1 = LogType(tid1);
        assert(type1.m_flags & TF_ENUM);
        CR_EnumID eid = type1.m_sub_id;
        if (!le.empty()) {
            // overwrite the definition if not empty
            m_enums[eid] = le;
            type1.m_size = type1.m_align = 4;
        }
        return tid1;
    }
} // AddEnumType

bool CR_NameScope::CompleteStructType(CR_TypeID tid, CR_StructID sid) {
    const int bits_of_one_byte = 8;

    auto& ls = LogStruct(sid);
    auto& type1 = LogType(tid);
    if ((type1.m_flags & TF_INCOMPLETE) == 0) {
        ls.m_is_complete = true;
        return true;
    }

    // check completeness for each field
    const size_t siz = ls.m_members.size();
    bool is_complete = true;
    for (std::size_t i = 0; i < siz; ++i) {
        auto tid2 = ls.m_members[i].m_type_id;
        auto& type2 = LogType(tid2);
        if (type2.m_flags & TF_INCOMPLETE) {
            if (!CompleteType(tid2, type2)) {
                auto& name = ls.m_members[i].m_name;
                m_error_info->add_warning(
                    type2.m_location, "'" + name + "' has incomplete type");
                is_complete = false;
            }
        }
        if (ls.m_members[i].m_bits != -1 || (type2.m_flags & TF_BITFIELD)) {
            type1.m_flags |= TF_BITFIELD;
        }
        #ifdef __GNUC__
            if ((type1.m_flags & TF_BITFIELD) ||
                (type2.m_flags & TF_INACCURATE))
            {
                type1.m_flags |= TF_INACCURATE;
            }
        #endif
    }

    // calculate alignment and size
    int byte_offset = 0, prev_item_size = 0;
    int bits_remain = 0, max_align = 1;
    if (is_complete) {
        for (std::size_t i = 0; i < siz; ++i) {
            auto tid2 = ls.m_members[i].m_type_id;
            auto& type2 = LogType(tid2);
            int item_size = type2.m_size;           // size of type
            int item_align = type2.m_align;         // alignment requirement
            if (type1.m_alignas < type2.m_alignas) {
                type1.m_alignas = type2.m_alignas;
            }
            int bits = ls.m_members[i].m_bits;      // bits of bitfield
            if (bits != -1) {
                // the bits specified as bitfield
                assert(bits <= item_size * bits_of_one_byte);
                if (ls.m_pack < item_align) {
                    item_align = ls.m_pack;
                }
                if (prev_item_size == item_size || bits_remain == 0) {
                    // bitfield continuous
                    ls.m_members[i].m_bit_offset =
                        byte_offset * bits_of_one_byte + bits_remain;
                    bits_remain += bits;
                } else {
                    // bitfield discontinuous
                    int bytes =
                        (bits_remain + bits_of_one_byte - 1) / bits_of_one_byte;
                    byte_offset += bytes;
                    if (type2.m_alignas) {
                        int alignas_ = type2.m_alignas;
                        byte_offset = (byte_offset + alignas_ - 1) / alignas_ * alignas_;
                    } else if (ls.m_pack < item_align) {
                        assert(ls.m_pack);
                        byte_offset = (byte_offset + ls.m_pack - 1) / ls.m_pack * ls.m_pack;
                    } else {
                        assert(item_align);
                        byte_offset = (byte_offset + item_align - 1) / item_align * item_align;
                    }
                    ls.m_members[i].m_bit_offset =
                        byte_offset * bits_of_one_byte;
                    bits_remain = bits;
                }
            } else {
                // not bitfield
                if (bits_remain) {
                    // the previous was bitfield
                    int prev_size_bits = prev_item_size * bits_of_one_byte;
                    assert(prev_size_bits);
                    byte_offset += ((bits_remain + prev_size_bits - 1)
                                    / prev_size_bits * prev_size_bits) / bits_of_one_byte;
                    bits_remain = 0;
                }
                if (prev_item_size) {
                    // add padding
                    if (type2.m_alignas) {
                        int alignas_ = type2.m_alignas;
                        byte_offset = (byte_offset + alignas_ - 1) / alignas_ * alignas_;
                    } else if (ls.m_pack < item_align) {
                        assert(ls.m_pack);
                        byte_offset = (byte_offset + ls.m_pack - 1) / ls.m_pack * ls.m_pack;
                    } else {
                        assert(item_align);
                        byte_offset = (byte_offset + item_align - 1) / item_align * item_align;
                    }
                }
                ls.m_members[i].m_bit_offset = byte_offset * bits_of_one_byte;
                byte_offset += item_size;
            }
            if (max_align < item_align) {
                max_align = item_align;
            }
            prev_item_size = item_size;
        }
    }

    // alignment requirement and tail padding
    if (bits_remain) {
        int prev_size_bits = prev_item_size * bits_of_one_byte;
        assert(prev_size_bits);
        byte_offset += ((bits_remain + prev_size_bits - 1)
                        / prev_size_bits * prev_size_bits) / bits_of_one_byte;
    }
    if (type1.m_alignas) {
        int alignas_;
        if (type1.m_alignas >= max_align) {
            alignas_ = type1.m_alignas;
        } else {
            alignas_ = max_align;
        }
        ls.m_align = alignas_;
        type1.m_align = alignas_;
        byte_offset = (byte_offset + alignas_ - 1) / alignas_ * alignas_;
    } else if (ls.m_pack > max_align) {
        ls.m_align = max_align;
        type1.m_align = max_align;
        assert(ls.m_pack);
        byte_offset = (byte_offset + max_align - 1) / max_align * max_align;
    } else {
        ls.m_align = ls.m_pack;
        type1.m_align = ls.m_pack;
        assert(max_align);
        byte_offset = (byte_offset + ls.m_pack - 1) / ls.m_pack * ls.m_pack;
    }

    // total size
    type1.m_size = byte_offset;

    // complete
    if (is_complete && ls.m_members.size()) {
        type1.m_flags &= ~TF_INCOMPLETE;
        ls.m_is_complete = true;
    }
    return is_complete;
} // CompleteStructType

bool CR_NameScope::CompleteUnionType(CR_TypeID tid, CR_StructID sid) {
    auto& ls = LogStruct(sid);
    auto& type1 = LogType(tid);

    if ((type1.m_flags & TF_INCOMPLETE) == 0) {
        ls.m_is_complete = true;
        return true;
    }

    // check completeness for each field
    const size_t siz = ls.m_members.size();
    bool is_complete = true;
    for (std::size_t i = 0; i < siz; ++i) {
        auto tid2 = ls.m_members[i].m_type_id;
        auto& type2 = LogType(tid2);
        if (type2.m_flags & TF_INCOMPLETE) {
            if (!CompleteType(tid2, type2)) {
                auto& name = ls.m_members[i].m_name;
                m_error_info->add_warning(
                    type2.m_location, "'" + name + "' has incomplete type");
                is_complete = false;
            }
            #ifdef __GNUC__
                if (type2.m_flags & TF_INACCURATE) {
                    type1.m_flags |= TF_INACCURATE;
                }
            #endif
        }
    }

    // calculate alignment and size
    int item_size, item_align, max_size = 0, max_align = 1;
    for (std::size_t i = 0; i < siz; ++i) {
        auto tid2 = ls.m_members[i].m_type_id;
        auto& type2 = LogType(tid2);
        item_size = type2.m_size;
        item_align = type2.m_align;
        if (type1.m_alignas < type2.m_alignas) {
            type1.m_alignas = type2.m_alignas;
        }
        if (max_size < item_size) {
            max_size = item_size;
        }
        if (max_align < item_align) {
            max_align = item_align;
        }
    }

    // alignment requirement
    if (type1.m_alignas) {
        int alignas_;
        if (type1.m_alignas >= max_align) {
            alignas_ = type1.m_alignas;
        } else {
            alignas_ = max_align;
        }
        ls.m_align = alignas_;
        type1.m_align = alignas_;
    } else if (ls.m_pack > max_align) {
        ls.m_align = max_align;
        type1.m_align = max_align;
    } else {
        ls.m_align = ls.m_pack;
        type1.m_align = ls.m_pack;
    }

    type1.m_size = max_size;

    // complete
    if (is_complete && ls.m_members.size()) {
        type1.m_flags &= ~TF_INCOMPLETE;
        ls.m_is_complete = true;
    }
    return is_complete;
} // CompleteUnionType

bool CR_NameScope::CompleteType(CR_TypeID tid, CR_LogType& type) {
    if ((type.m_flags & TF_INCOMPLETE) == 0)
        return true;
    if (type.m_flags & TF_ALIAS) {
        if (CompleteType(type.m_sub_id)) {
            auto& type2 = LogType(type.m_sub_id);
            type.m_size = type2.m_size;
            type.m_align = type2.m_align;
            type.m_alignas = type2.m_alignas;
            if (type2.m_alignas) {
                type.m_align = type2.m_alignas;
                if (type.m_alignas < type2.m_alignas) {
                    type.m_alignas = type2.m_alignas;
                }
            }
            #ifdef __GNUC__
                if (type2.m_flags & TF_INACCURATE) {
                    type.m_flags |= TF_INACCURATE;
                }
            #endif
            type.m_flags &= ~TF_INCOMPLETE;
            return true;
        }
        return false;
    }
    if (type.m_flags & TF_ARRAY) {
        if (CompleteType(type.m_sub_id)) {
            auto& type2 = LogType(type.m_sub_id);
            type.m_size = type2.m_size * static_cast<int>(type.m_count);
            type.m_align = type2.m_align;
            if (type.m_alignas < type2.m_alignas) {
                type.m_alignas = type2.m_alignas;
                type.m_align = type2.m_alignas;
            }
            #ifdef __GNUC__
                if (type2.m_flags & TF_INACCURATE) {
                    type.m_flags |= TF_INACCURATE;
                }
            #endif
            type.m_flags &= ~TF_INCOMPLETE;
            return true;
        }
        return false;
    }
    if (type.m_flags & TF_VECTOR) {
        if (CompleteType(type.m_sub_id)) {
            auto& type2 = LogType(type.m_sub_id);
            if (type2.m_size) {
                type.m_count = type.m_size / type2.m_size;
            }
            if (type.m_alignas < type2.m_alignas) {
                type.m_alignas = type2.m_alignas;
                type.m_align = type2.m_alignas;
            }
            #ifdef __GNUC__
                if (type2.m_flags & TF_INACCURATE) {
                    type.m_flags |= TF_INACCURATE;
                }
            #endif
            type.m_flags &= ~TF_INCOMPLETE;
            return true;
        }
        return false;
    }
    if (type.m_flags & TF_CONST) {
        if (CompleteType(type.m_sub_id)) {
            auto& type2 = LogType(type.m_sub_id);
            type.m_size = type2.m_size;
            type.m_align = type2.m_align;
            type.m_alignas = type2.m_alignas;
            if (type2.m_alignas) {
                type.m_align = type2.m_alignas;
                if (type.m_alignas < type2.m_alignas) {
                    type.m_alignas = type2.m_alignas;
                }
            }
            #ifdef __GNUC__
                if (type2.m_flags & TF_INACCURATE) {
                    type.m_flags |= TF_INACCURATE;
                }
            #endif
            type.m_flags &= ~TF_INCOMPLETE;
            return true;
        }
    }
    if (type.m_flags & (TF_STRUCT | TF_UNION)) {
        auto& ls = LogStruct(type.m_sub_id);
        for (auto mem : ls.m_members) {
            CompleteType(mem.m_type_id);
        }
        if (type.m_flags & TF_STRUCT) {
            return CompleteStructType(tid, type.m_sub_id);
        } else if (type.m_flags & TF_UNION) {
            return CompleteUnionType(tid, type.m_sub_id);
        }
    }
    return false;
} // CompleteType

void CR_NameScope::CompleteTypeInfo() {
    for (CR_TypeID tid = 0; tid < m_types.size(); ++tid) {
        CompleteType(tid);
    }
}

std::string CR_NameScope::StringFromValue(const CR_TypedValue& value) const {
    auto tid = value.m_type_id;
    if (tid == cr_invalid_id) {
        return "";
    }
    if (IsIntegralType(tid)) {
        return value.m_text + value.m_extra;
    } else if (IsFloatingType(tid)) {
        if (value.m_text.find("INF") != std::string::npos ||
            value.m_text.find("NAN") != std::string::npos)
        {
            return value.m_text;
        }
        return value.m_text + value.m_extra;
    } else {
        return value.m_text;
    }
}

std::string CR_NameScope::StringOfEnum(
    const std::string& name, CR_EnumID eid) const
{
    if (eid == cr_invalid_id) {
        return "";  // invalid ID
    }
    auto& e = m_enums[eid];
    std::string str = StringOfEnumTag(name);
    if (!e.empty()) {
        str += "{\n";
        std::vector<std::string> array;
        for (auto it : e.m_mNameToValue) {
            array.emplace_back(it.first + " = " + std::to_string(it.second));
        }
        str += CrIndent(katahiromz::join(array, ", "));
        str += "}";
    }
    return str;
} // StringOfEnum

std::string
CR_NameScope::StringOfStructTag(
    const std::string& name, const CR_LogStruct& s) const
{
    std::string str;

    if (s.m_is_struct)
        str += "struct ";
    else
        str += "union ";

    auto& type = LogType(s.m_tid);
    if (type.m_alignas && type.m_alignas_explicit) {
        str += "_Alignas(";
        str += std::to_string(type.m_alignas);
        str += ") ";
    }

    if (name.size()) {
        if (s.m_is_struct) {
            assert(name.find("struct ") == 0);
            str += name.substr(7);
        } else {
            assert(name.find("union ") == 0);
            str += name.substr(6);
        }
        str += ' ';
    }
    return str;
} // StringOfStructTag

std::string CR_NameScope::StringOfStruct(
    const std::string& name, CR_StructID sid) const
{
    auto& s = LogStruct(sid);
    std::string str = StringOfStructTag(name, s);
    if (!s.empty()) {
        str += "{\n";
        std::string inner;
        const std::size_t siz = s.m_members.size();
        for (std::size_t i = 0; i < siz; i++) {
            inner += StringOfType(s.m_members[i].m_type_id, s.m_members[i].m_name, false);
            if (s.m_members[i].m_bits != -1) {
                inner += " : ";
                inner += std::to_string(s.m_members[i].m_bits);
            }
            inner += ";\n";
        }
        str += CrIndent(inner);
        str += "}";
    }
    return str;
} // StringOfStruct

std::string CR_NameScope::StringOfType(
    CR_TypeID tid, const std::string& name,
    bool expand/* = true*/, bool no_convension/* = false*/) const
{
    auto& type = LogType(tid);
    auto type_name = NameFromTypeID(tid);
    if (type.m_flags & TF_ALIAS) {
        // if type was alias
        if (expand || type_name.empty()) {
            return StringOfType(type.m_sub_id, name, false);
        } else {
            return type_name + " " + name;
        }
    }
    if (type.m_flags & (TF_STRUCT | TF_UNION)) {
        // if type was struct or union
        if (expand || type_name.empty()) {
            return StringOfStruct(type_name, type.m_sub_id) + " " + name;
        } else {
            return type_name + " " + name;
        }
    }
    if (type.m_flags & TF_ENUM) {
        // if type was enum
        if (expand || type_name.empty()) {
            return StringOfEnum(type_name, type.m_sub_id) + " " + name;
        } else {
            return type_name + " " + name;
        }
    }
    if (type.m_flags & (TF_ARRAY | TF_VECTOR)) {
        // if type was array or vector
        if (type.m_count) {
            std::string s = "[";
            s += std::to_string(type.m_count);
            s += ']';
            return StringOfType(type.m_sub_id, name + s, false);
        } else {
            return StringOfType(type.m_sub_id, name + "[]", false);
        }
    }
    if (type.m_flags & TF_FUNCTION) {
        // if type was function
        auto& func = LogFunc(type.m_sub_id);
        auto rettype = StringOfType(func.m_return_type, "", false);
        auto paramlist = StringOfParamList(func.m_params);
        std::string convension;
        if (!no_convension) {
            if (type.m_flags & TF_STDCALL)
                convension = "__stdcall ";
            else if (type.m_flags & TF_FASTCALL)
                convension = "__fastcall ";
            else
                convension = "__cdecl ";
        }
        if (func.m_ellipsis)
            paramlist += ", ...";
        return rettype + convension + name + "(" + paramlist + ")";
    }
    if (type.m_flags & TF_POINTER) {
        // if type was pointer
        auto sub_id = type.m_sub_id;
        auto& type2 = LogType(sub_id);
        if (type2.m_flags & TF_FUNCTION) {
            // function pointer
            if (type.m_flags & TF_CONST) {
                // const function pointer
                if (type.m_flags & TF_STDCALL)
                    return StringOfType(sub_id, "(__stdcall * const " + name + ")", false, true);
                else if (type.m_flags & TF_FASTCALL)
                    return StringOfType(sub_id, "(__fastcall * const " + name + ")", false, true);
                else
                    return StringOfType(sub_id, "(__cdecl * const " + name + ")", false, true);
            } else {
                // non-const function pointer
                if (type.m_flags & TF_STDCALL)
                    return StringOfType(sub_id, "(__stdcall *" + name + ")", false, true);
                else if (type.m_flags & TF_FASTCALL)
                    return StringOfType(sub_id, "(__fastcall *" + name + ")", false, true);
                else
                    return StringOfType(sub_id, "(__cdecl *" + name + ")", false, true);
            }
        } else if (type2.m_flags & TF_POINTER) {
            // pointer to pointer
            if (type.m_flags & TF_CONST) {
                return StringOfType(sub_id, "(* const " + name + ")", false);
            } else {
                return StringOfType(sub_id, "(*" + name + ")", false);
            }
        } else if (type2.m_flags & (TF_ARRAY | TF_VECTOR)) {
            // pointer to array
            if (type.m_flags & TF_CONST) {
                // pointer to const array
                if (type2.m_count) {
                    std::string s = "[";
                    s += std::to_string(type2.m_count);
                    s += ']';
                    return StringOfType(sub_id, "(* const " + name + s + ")", false);
                } else {
                    return StringOfType(sub_id, "(* const " + name + "[])", false);
                }
            } else {
                // pointer to non-const array
                if (type2.m_count) {
                    std::string s = "[";
                    s += std::to_string(type2.m_count);
                    s += ']';
                    return StringOfType(sub_id, "(*" + name + s + ")", false);
                } else {
                    return StringOfType(sub_id, "(*" + name + "[])", false);
                }
            }
        } else {
            // otherwise
            if (type.m_flags & TF_CONST) {
                return StringOfType(sub_id, "", false) + "* const " + name;
            } else {
                return StringOfType(sub_id, "", false) + "*" + name;
            }
        }
    }
    if (type.m_flags & TF_CONST) {
        // if type is const
        return "const " + StringOfType(type.m_sub_id, name, false);
    }
    if (type_name.size()) {
        // if there was type name
        return type_name + " " + name;
    }
    return "";  // no name
} // StringOfType

std::string CR_NameScope::StringOfParamList(
    const std::vector<CR_FuncParam>& params) const
{
    std::size_t i, size = params.size();
    std::string str;
    if (size > 0) {
        str += StringOfType(params[0].m_type_id, params[0].m_name, false);
        for (i = 1; i < size; i++) {
            str += ", ";
            str += StringOfType(params[i].m_type_id, params[i].m_name, false);
        }
    } else {
        str += "void";
    }
    return str;
} // StringOfParamList

bool CR_NameScope::IsFuncType(CR_TypeID tid) const {
    if (tid == cr_invalid_id)
        return false;
    tid = ResolveAlias(tid);
    auto& type = LogType(tid);
    if (type.m_flags & TF_FUNCTION)
        return true;
    return false;
} // IsFuncType

bool CR_NameScope::IsPredefinedType(CR_TypeID tid) const {
    while (tid != cr_invalid_id) {
        auto& type = LogType(tid);
        if (type.m_flags & (TF_POINTER | TF_ARRAY | TF_CONST)) {
            tid = type.m_sub_id;
            continue;
        }
        if (type.m_location.m_file == "(predefined)") {
            return true;
        }
        break;
    }
    return false;
} // IsPredefinedType

bool CR_NameScope::IsIntegralType(CR_TypeID tid) const {
    while (tid != cr_invalid_id) {
        tid = ResolveAlias(tid);
        if (tid == cr_invalid_id) {
            break;
        }
        auto& type = LogType(tid);
        const CR_TypeFlags not_flags =
            (TF_DOUBLE | TF_FLOAT | TF_POINTER | TF_ARRAY | TF_VECTOR |
             TF_FUNCTION | TF_STRUCT | TF_UNION | TF_ENUM);
        if (type.m_flags & not_flags)
            return false;
        const CR_TypeFlags flags =
            (TF_INT | TF_CHAR | TF_SHORT | TF_LONG | TF_LONGLONG);
        if (type.m_flags & flags)
            return true;
        if ((type.m_flags & (TF_CONST | TF_POINTER)) == TF_CONST) {
            tid = type.m_sub_id;
        } else {
            break;
        }
    }
    return false;
} // IsIntegralType

bool CR_NameScope::IsFloatingType(CR_TypeID tid) const {
    while (tid != cr_invalid_id) {
        tid = ResolveAlias(tid);
        if (tid == cr_invalid_id) {
            break;
        }
        auto& type = LogType(tid);
        if (type.m_flags & (TF_DOUBLE | TF_FLOAT))
            return true;
        if ((type.m_flags & (TF_CONST | TF_POINTER)) == TF_CONST) {
            tid = type.m_sub_id;
        } else {
            break;
        }
    }
    return false;
} // IsFloatingType

bool CR_NameScope::IsUnsignedType(CR_TypeID tid) const {
    while (tid != cr_invalid_id) {
        tid = ResolveAlias(tid);
        if (tid == cr_invalid_id) {
            break;
        }
        auto& type = LogType(tid);
        if (type.m_flags & TF_UNSIGNED)
            return true;
        if ((type.m_flags & (TF_CONST | TF_POINTER)) == TF_CONST) {
            tid = type.m_sub_id;
        } else {
            break;
        }
    }
    return false;
} // IsUnsignedType

bool CR_NameScope::IsPointerType(CR_TypeID tid) const {
    while (tid != cr_invalid_id) {
        tid = ResolveAlias(tid);
        if (tid == cr_invalid_id) {
            break;
        }
        auto& type = LogType(tid);
        if (type.m_flags & TF_POINTER)
            return true;
        if ((type.m_flags & (TF_CONST | TF_POINTER)) == TF_CONST) {
            tid = type.m_sub_id;
        } else {
            break;
        }
    }
    return false;
}

// is it a constant type?
bool CR_NameScope::IsConstantType(CR_TypeID tid) const {
    while (tid != cr_invalid_id) {
        tid = ResolveAlias(tid);
        if (tid == cr_invalid_id) {
            break;
        }
        auto& type = LogType(tid);
        if (type.m_flags & TF_CONST) {
            return true;
        }
        if (type.m_flags & (TF_ARRAY | TF_VECTOR)) {
            tid = type.m_sub_id;
        } else {
            break;
        }
    }
    return false;
} // CR_NameScope::IsConstantType

// is it an array type?
bool CR_NameScope::IsArrayType(CR_TypeID tid) const {
    while (tid != cr_invalid_id) {
        tid = ResolveAlias(tid);
        if (tid == cr_invalid_id) {
            break;
        }
        auto& type = LogType(tid);
        if (type.m_flags & TF_ARRAY)
            return true;
        if ((type.m_flags & (TF_CONST | TF_POINTER)) == TF_CONST) {
            tid = type.m_sub_id;
        } else {
            break;
        }
    }
    return false;
} // CR_NameScope::IsArrayType

// is it a struct type?
bool CR_NameScope::IsStructType(CR_TypeID tid) const {
    while (tid != cr_invalid_id) {
        tid = ResolveAlias(tid);
        if (tid == cr_invalid_id) {
            break;
        }
        auto& type = LogType(tid);
        if (type.m_flags & TF_STRUCT)
            return true;
        if ((type.m_flags & (TF_CONST | TF_POINTER)) == TF_CONST) {
            tid = type.m_sub_id;
        } else {
            break;
        }
    }
    return false;
} // CR_NameScope::IsStructType

// is it a union type?
bool CR_NameScope::IsUnionType(CR_TypeID tid) const {
    while (tid != cr_invalid_id) {
        tid = ResolveAlias(tid);
        if (tid == cr_invalid_id) {
            break;
        }
        auto& type = LogType(tid);
        if (type.m_flags & TF_UNION)
            return true;
        if ((type.m_flags & (TF_CONST | TF_POINTER)) == TF_CONST) {
            tid = type.m_sub_id;
        } else {
            break;
        }
    }
    return false;
} // CR_NameScope::IsUnionType

// is it a struct or union type?
bool CR_NameScope::IsStructOrUnionType(CR_TypeID tid) const {
    while (tid != cr_invalid_id) {
        tid = ResolveAlias(tid);
        if (tid == cr_invalid_id) {
            break;
        }
        auto& type = LogType(tid);
        if (type.m_flags & (TF_STRUCT | TF_UNION))
            return true;
        if ((type.m_flags & (TF_CONST | TF_POINTER)) == TF_CONST) {
            tid = type.m_sub_id;
        } else {
            break;
        }
    }
    return false;
}

CR_TypeID CR_NameScope::ResolveAlias(CR_TypeID tid) const {
    while (tid != cr_invalid_id) {
        auto& type = LogType(tid);
        if (type.m_flags & TF_ALIAS) {
            tid = type.m_sub_id;
        } else {
            break;
        }
    }
    return tid;
} // CR_NameScope::ResolveAlias

CR_TypeID CR_NameScope::ResolvePointer(CR_TypeID tid) const {
    tid = ResolveAlias(tid);
    if (tid == cr_invalid_id)
        return cr_invalid_id;

    auto& type = LogType(tid);
    if (type.m_flags & TF_POINTER) {
        return type.m_sub_id;
    } else {
        return cr_invalid_id;
    }
} // CR_NameScope::ResolvePointer

CR_TypeID CR_NameScope::ResolveAliasAndCV(CR_TypeID tid) const {
    while (tid != cr_invalid_id) {
        tid = ResolveAlias(tid);
        if (tid == cr_invalid_id) {
            break;
        }
        auto& type = LogType(tid);
        if ((type.m_flags & (TF_CONST | TF_POINTER)) == TF_CONST) {
            tid = type.m_sub_id;
        } else if (!(type.m_flags & TF_ALIAS)) {
            break;
        }
    }
    return tid;
} // CR_NameScope::ResolveAliasAndCV

CR_TypeID CR_NameScope::TypeIDFromFlags(CR_TypeFlags flags) const {
    const size_t siz = m_types.size();
    for (size_t i = 0; i < siz; ++i) {
        if (LogType(i).m_flags == flags)
            return i;
    }
    return cr_invalid_id;
}

CR_TypeID CR_NameScope::TypeIDFromName(const std::string& name) const {
    auto it = m_mNameToTypeID.find(name);
    if (it != m_mNameToTypeID.end())
        return it->second;
    else
        return cr_invalid_id;
}

std::string CR_NameScope::NameFromTypeID(CR_TypeID tid) const {
    auto it = m_mTypeIDToName.find(tid);
    if (it != m_mTypeIDToName.end())
        return it->second;
    else
        return "";
}

void CR_NameScope::AddStructMembers(
    std::vector<CR_StructMember>& members,
    CR_StructID sid, const std::string& name/* = ""*/, int bit_offset/* = 0*/) const
{
    // for all members of struct or union 
    auto& ls = LogStruct(sid);
    for (auto& mem : ls.m_members) {
        CR_StructMember member = mem;
        // shift bit offsets
        member.m_bit_offset += bit_offset;
        if (member.m_name.size()) {
            // member has a name
            if (name.size()) {
                member.m_name = name + "." + member.m_name;
            }
            members.emplace_back(member);
        } else {
            // member is nameless (anonymous)
            CR_TypeID rtid = ResolveAliasAndCV(mem.m_type_id);
            auto& rtype = LogType(rtid);
            // struct or union type?
            if (rtype.m_flags & (TF_STRUCT | TF_UNION)) {
                // add member
                AddStructMembers(
                    members, rtype.m_sub_id, name, member.m_bit_offset);
            }
        }
    }
} // CR_NameScope::AddStructMembers

void CR_NameScope::AddAccessMembers(
    std::vector<CR_AccessMember>& members,
    CR_TypeID tid, const std::string& name,
    int bit_offset/* = 0*/, int bits/* = -1*/) const
{
    auto rtid = ResolveAliasAndCV(tid);
    if (rtid == cr_invalid_id) {
        assert(bits != -1);
        if (name.size()) {
            CR_AccessMember member;
            member.m_type_id = rtid;
            member.m_name = name;
            member.m_bit_offset = bit_offset;
            member.m_bits = bits;
            members.emplace_back(member);
        }
        return;
    }
    auto& rtype = LogType(rtid);
    if (bits == -1) {
        bits = rtype.m_size * 8;
    }
    if (rtype.m_flags & TF_ARRAY) {
        // array type
        auto sub_id = rtype.m_sub_id;
        auto item_size = LogType(sub_id).m_size;
        for (size_t i = 0; i < rtype.m_count; ++i) {
            // add an access member
            CR_AccessMember member;
            member.m_type_id = sub_id;
            member.m_name = name + "[" + std::to_string(i) + "]";
            member.m_bit_offset = bit_offset + int((item_size * 8) * i);
            member.m_bits = int(item_size * 8);
            // add member's children
            AddAccessMembers(members,
                member.m_type_id,
                member.m_name, member.m_bit_offset, member.m_bits);
        }
    } else if (rtype.m_flags & (TF_STRUCT | TF_UNION)) {
        // struct or union type
        auto sid = rtype.m_sub_id;
        auto& ls = LogStruct(sid);
        std::vector<CR_AccessMember> new_members;
        for (auto& mem : ls.m_members) {
            CR_AccessMember member = mem;
            if (name.size() && member.m_name.size()) {
                // there is a name
                member.m_name = name + "." + member.m_name;
            }
            // shift bit offsets
            member.m_bit_offset += bit_offset;
            // set bits (not -1)
            auto item_size = LogType(mem.m_type_id).m_size;
            if (member.m_bits == -1) {
                member.m_bits = item_size * 8;
            }
            AddAccessMembers(new_members, member.m_type_id,
                member.m_name, member.m_bit_offset, member.m_bits);
        }
        // append new members
        members.insert(members.end(),
                       new_members.begin(), new_members.end());
    }
    // add self
    if (name.size()) {
        CR_AccessMember member;
        member.m_type_id = rtid;
        member.m_name = name;
        member.m_bit_offset = bit_offset;
        member.m_bits = bits;
        members.emplace_back(member);
    }
} // CR_NameScope::AddAccessMembers

CR_TypeID CR_NameScope::AddConstCharType() {
    return AddConstType(m_char_type);
}

CR_TypeID CR_NameScope::AddConstUCharType() {
    return AddConstType(m_uchar_type);
}

CR_TypeID CR_NameScope::AddConstShortType() {
    return AddConstType(m_short_type);
}

CR_TypeID CR_NameScope::AddConstUShortType() {
    return AddConstType(m_ushort_type);
}

CR_TypeID CR_NameScope::AddConstIntType() {
    return AddConstType(m_int_type);
}

CR_TypeID CR_NameScope::AddConstUIntType() {
    return AddConstType(m_uint_type);
}

CR_TypeID CR_NameScope::AddConstLongType() {
    return AddConstType(m_long_type);
}

CR_TypeID CR_NameScope::AddConstULongType() {
    return AddConstType(m_ulong_type);
}

CR_TypeID CR_NameScope::AddConstLongLongType() {
    return AddConstType(m_long_long_type);
}

CR_TypeID CR_NameScope::AddConstULongLongType() {
    return AddConstType(m_ulong_long_type);
}

CR_TypeID CR_NameScope::AddConstFloatType() {
    return AddConstType(m_float_type);
}

CR_TypeID CR_NameScope::AddConstDoubleType() {
    return AddConstType(m_double_type);
}

CR_TypeID CR_NameScope::AddConstLongDoubleType() {
    return AddConstType(m_long_double_type);
}

CR_TypeID CR_NameScope::AddConstStringType() {
    auto tid = m_char_type;
    auto& type = LogType(tid);
    tid = AddConstType(tid);
    tid = AddPointerType(tid, TF_CONST, type.m_location);
    return tid;
}

CR_TypeID CR_NameScope::AddConstWStringType() {
    CR_TypeID tid;
    auto it = MapNameToTypeID().find("wchar_t");
    if (it != MapNameToTypeID().end()) {
        tid = it->second;
    } else {
        tid = AddAliasType("wchar_t", m_ushort_type,
                           CR_Location("(predefined)", 0));
    }
    auto& type = LogType(tid);
    tid = AddConstType(tid);
    tid = AddPointerType(tid, TF_CONST, type.m_location);
    return tid;
}

CR_TypeID CR_NameScope::AddVoidPointerType() {
    CR_TypeID tid = 0;
    auto& type = LogType(tid);
    tid = AddPointerType(tid, 0, type.m_location);
    return tid;
}

CR_TypeID CR_NameScope::AddConstVoidPointerType() {
    CR_TypeID tid = 0;
    auto& type = LogType(tid);
    tid = AddConstType(tid);
    tid = AddPointerType(tid, 0, type.m_location);
    return tid;
}

CR_TypeID CR_NameScope::AddConstCharArray(size_t count) {
    auto tid = m_char_type;
    auto& type = LogType(tid);
    tid = AddConstType(tid);
    tid = AddArrayType(tid, static_cast<int>(count), type.m_location);
    return tid;
}

CR_TypeID CR_NameScope::AddConstWCharArray(size_t count) {
    CR_TypeID tid;
    auto it = MapNameToTypeID().find("wchar_t");
    if (it != MapNameToTypeID().end()) {
        tid = it->second;
    } else {
        tid = AddAliasType("wchar_t", m_ushort_type,
                           CR_Location("(predefined)", 0));
    }
    auto& type = LogType(tid);
    tid = AddConstType(tid);
    tid = AddArrayType(tid, static_cast<int>(count), type.m_location);
    return tid;
}

CR_TypeID CR_NameScope::IsStringType(CR_TypeID tid) const {
    tid = ResolveAliasAndCV(tid);
    if (tid == cr_invalid_id) {
        return tid;
    }
    auto& type1 = LogType(tid);
    if (type1.m_flags & (TF_POINTER | TF_ARRAY)) {
        auto tid2 = ResolveAliasAndCV(type1.m_sub_id);
        auto& type2 = LogType(tid2);
        if ((type2.m_flags & (TF_CONST | TF_POINTER)) == TF_CONST) {
            auto tid3 = ResolveAlias(type2.m_sub_id);
            auto& type3 = LogType(tid3);
            return (type3.m_flags & TF_CHAR) != 0;
        } else {
            return (type2.m_flags & TF_CHAR) != 0;
        }
    }
    return false;
}

CR_TypeID CR_NameScope::IsWStringType(CR_TypeID tid) const {
    tid = ResolveAliasAndCV(tid);
    if (tid == cr_invalid_id) {
        return tid;
    }
    auto& type1 = LogType(tid);
    if (type1.m_flags & (TF_POINTER | TF_ARRAY)) {
        auto tid2 = ResolveAliasAndCV(type1.m_sub_id);
        auto& type2 = LogType(tid2);
        if ((type2.m_flags & (TF_CONST | TF_POINTER)) == TF_CONST) {
            auto tid3 = ResolveAlias(type2.m_sub_id);
            auto& type3 = LogType(tid3);
            return (type3.m_flags & TF_SHORT) != 0;
        } else if (type2.m_flags & TF_SHORT) {
            return true;
        }
    }
    return false;
}

long long CR_NameScope::GetLongLongValue(const CR_TypedValue& value) const {
    long long ret = 0;
    if (value.m_type_id == cr_invalid_id) {
        return ret;
    }
    auto& type = LogType(value.m_type_id);
    if (type.m_size != static_cast<int>(value.m_size)) {
        return 0;
    }
    if (type.m_size == sizeof(int)) {
        ret = static_cast<long long>(value.get<int>());
    } else if (type.m_size == sizeof(char)) {
        ret = static_cast<long long>(value.get<char>());
    } else if (type.m_size == sizeof(short)) {
        ret = static_cast<long long>(value.get<short>());
    } else if (type.m_size == sizeof(long)) {
        ret = static_cast<long long>(value.get<long>());
    } else if (type.m_size == sizeof(long long)) {
        ret = value.get<long long>();
    }
    return ret;
}

unsigned long long CR_NameScope::GetULongLongValue(const CR_TypedValue& value) const {
    unsigned long long ret = 0;
    if (value.m_type_id == cr_invalid_id) {
        return ret;
    }
    auto& type = LogType(value.m_type_id);
    if (type.m_size != static_cast<int>(value.m_size)) {
        return 0;
    }
    if (type.m_size == sizeof(unsigned int)) {
        ret = static_cast<unsigned long long>(value.get<unsigned int>());
    } else if (type.m_size == sizeof(unsigned char)) {
        ret = static_cast<unsigned long long>(value.get<unsigned char>());
    } else if (type.m_size == sizeof(unsigned short)) {
        ret = static_cast<unsigned long long>(value.get<unsigned short>());
    } else if (type.m_size == sizeof(unsigned long)) {
        ret = static_cast<unsigned long long>(value.get<unsigned long>());
    } else if (type.m_size == sizeof(unsigned long long)) {
        ret = value.get<unsigned long long>();
    }
    return ret;
}

long double CR_NameScope::GetLongDoubleValue(const CR_TypedValue& value) const {
    long double ret = 0;
    if (value.m_type_id == cr_invalid_id) {
        return ret;
    }
    auto& type = LogType(value.m_type_id);
    if (type.m_size != static_cast<int>(value.m_size)) {
        return 0;
    }
    if (IsFloatingType(value.m_type_id)) {
        if (type.m_size == sizeof(float)) {
            ret = value.get<float>();
        } else if (type.m_size == sizeof(double)) {
            ret = value.get<double>();
        } else if (type.m_size == sizeof(long double)) {
            ret = value.get<long double>();
        } else {
            assert(0);
        }
    } else if (IsIntegralType(value.m_type_id)) {
        if (IsUnsignedType(value.m_type_id)) {
            if (type.m_size == sizeof(unsigned int)) {
                ret = value.get<unsigned int>();
            } else if (type.m_size == sizeof(unsigned char)) {
                ret = value.get<unsigned char>();
            } else if (type.m_size == sizeof(unsigned short)) {
                ret = value.get<unsigned short>();
            } else if (type.m_size == sizeof(unsigned long)) {
                ret = value.get<unsigned long>();
            } else if (type.m_size == sizeof(unsigned long long)) {
                ret = static_cast<long double>(
                    value.get<unsigned long long>()
                );
            }
        } else {
            if (type.m_size == sizeof(int)) {
                ret = value.get<int>();
            } else if (type.m_size == sizeof(char)) {
                ret = value.get<char>();
            } else if (type.m_size == sizeof(short)) {
                ret = value.get<short>();
            } else if (type.m_size == sizeof(long)) {
                ret = value.get<long>();
            } else if (type.m_size == sizeof(long long)) {
                ret = static_cast<long double>(
                    value.get<long long>()
                );
            }
        }
    } else if (IsPointerType(value.m_type_id)) {
        if (Is64Bit()) {
            ret = static_cast<long double>(
                value.get<unsigned long long>()
            );
        } else {
            ret = static_cast<long double>(
                value.get<unsigned int>()
            );
        }
    }
    return ret;
}

CR_TypedValue CR_NameScope::StaticCast(
    CR_TypeID tid, const CR_TypedValue& value) const
{
    if (tid == value.m_type_id) {
        return value;
    }

    CR_TypedValue ret;
    ret.m_type_id = tid;
    ret.m_size = SizeOfType(tid);
    auto& type = LogType(tid);
    if (HasValue(value)) {
        auto tid2 = value.m_type_id;
        auto& type2 = LogType(tid2);
        if (IsIntegralType(tid)) {
            if (IsUnsignedType(tid)) {
                // tid is unsigned
                if (IsUnsignedType(tid2)) {
                    auto u2 = GetULongLongValue(value);
                    SetULongLongValue(ret, u2);
                } else {
                    auto n2 = GetLongLongValue(value);
                    SetULongLongValue(ret, n2);
                }
            } else {
                // tid is signed
                if (IsUnsignedType(tid2)) {
                    auto u2 = GetULongLongValue(value);
                    SetLongLongValue(ret, u2);
                } else {
                    auto n2 = GetLongLongValue(value);
                    SetLongLongValue(ret, n2);
                }
            }
        } else if (IsFloatingType(tid)) {
            auto ld2 = GetLongDoubleValue(value);
            SetLongDoubleValue(ret, ld2);
        } else if (IsPointerType(tid) &&
                   !(value.m_text.size() && value.m_text[0] == '"'))
        {
            auto u2 = GetULongLongValue(value);
            SetULongLongValue(ret, u2);
        } else {
            ret = value;
            ret.m_type_id = tid;
        }
    }

    return ret;
}

CR_TypedValue CR_NameScope::ReinterpretCast(
    CR_TypeID tid, const CR_TypedValue& value) const
{
    if (value.m_type_id == tid) {
        return value;
    }
    CR_TypedValue ret(value);
    ret.m_type_id = tid;
    return ret;
}

CR_TypedValue CR_NameScope::Cast(
    CR_TypeID tid, const CR_TypedValue& value) const
{
    CR_TypedValue ret;
    if (tid == cr_invalid_id || value.m_type_id == cr_invalid_id) {
        ret.m_type_id = cr_invalid_id;
        return ret;
    }
    auto& type1 = LogType(tid);
    auto& type2 = LogType(value.m_type_id);
    if (IsPointerType(tid)) {
        int pointer_size = type1.m_size;
        if (IsPointerType(value.m_type_id)) {
            ret = ReinterpretCast(tid, value);
        } else {
            if (pointer_size == 8) {
                ret = StaticCast(m_ulong_long_type, value);
                ret.m_type_id = tid;
            } else if (pointer_size == 4) {
                ret = StaticCast(m_uint_type, value);
                ret.m_type_id = tid;
            }
        }
    } else if (IsPointerType(value.m_type_id)) {
        if (type1.m_size == sizeof(int)) {
            ret = StaticCast(m_int_type, value);
            ret.m_extra = "";
        } else if (type1.m_size == sizeof(char)) {
            ret = StaticCast(m_char_type, value);
            ret.m_extra = "i8";
        } else if (type1.m_size == sizeof(short)) {
            ret = StaticCast(m_short_type, value);
            ret.m_extra = "i16";
        } else if (type1.m_size == sizeof(long)) {
            ret = StaticCast(m_long_type, value);
            ret.m_extra = "L";
        } else if (type1.m_size == sizeof(long long)) {
            ret = StaticCast(m_long_long_type, value);
            ret.m_extra = "LL";
        }
        ret.m_type_id = tid;
    } else {
        return StaticCast(tid, value);
    }
    return ret;
}

void CR_NameScope::SetAlignas(CR_TypeID tid, int alignas_) {
    if (tid == cr_invalid_id) {
        return;
    }
    auto& type = LogType(tid);
    type.m_align = alignas_;
    type.m_alignas = alignas_;
    type.m_alignas_explicit = true;
    if (type.m_flags & (TF_STRUCT | TF_UNION)) {
        LogStruct(type.m_sub_id).m_align = alignas_;
        LogStruct(type.m_sub_id).m_alignas = alignas_;
        LogStruct(type.m_sub_id).m_alignas_explicit = true;
    }
}

CR_TypedValue
CR_NameScope::ArrayItem(const CR_TypedValue& array, size_t index) const {
    CR_TypedValue ret;
    if (array.m_type_id == cr_invalid_id) {
        ret.m_type_id = cr_invalid_id;
        return ret;
    }
    auto& array_type = LogType(array.m_type_id);
    auto item_tid = array_type.m_sub_id;
    auto& item_type = LogType(item_tid);
    if (0 <= index && index < array_type.m_count) {
    } else {
        return ret;
    }

    if (array.m_addr) {
        unsigned long long ull = *array.m_addr.get();
        ull += index * item_type.m_size;
        ret.m_addr = make_shared<unsigned long long>(ull);
    }
    const char *ptr = array.get_at<char>(
        index * item_type.m_size, item_type.m_size);
    if (ptr) {
        SetValue(ret, item_tid, ptr, item_type.m_size);
    }
    return ret;
}

CR_TypedValue CR_NameScope::Dot(
    const CR_TypedValue& struct_value, const std::string& name) const
{
    const int bits_of_one_byte = 8;
    CR_TypedValue ret;
    if (struct_value.m_type_id == cr_invalid_id) {
        ret.m_type_id = cr_invalid_id;
        return ret;
    }
    auto tid = ResolveAlias(struct_value.m_type_id);
    auto& struct_type = LogType(tid);
    std::vector<CR_AccessMember> children;
    AddStructMembers(children, struct_type.m_sub_id);
    bool is_set = false;
    for (auto& child : children) {
        if (child.m_name == name) {
            if (child.m_bit_offset % bits_of_one_byte) {
                // TODO: bitfield not supported yet
                break;
            }
            ret.m_type_id = child.m_type_id;
            if (struct_value.m_addr) {
                unsigned long long ull = *struct_value.m_addr.get();
                ull += child.m_bit_offset / bits_of_one_byte;
                ret.m_addr = make_shared<unsigned long long>(ull);
            }
            ret.m_size = SizeOfType(child.m_type_id);
            const char *ptr =
                struct_value.get_at<char>(
                child.m_bit_offset / bits_of_one_byte, ret.m_size);
            if (ptr) {
                SetValue(ret, child.m_type_id, ptr, ret.m_size);
            }
            is_set = true;
            break;
        }
    }
    if (!is_set) {
        ret.m_type_id = cr_invalid_id;
    }
    return ret;
}

CR_TypedValue
CR_NameScope::Asterisk(const CR_TypedValue& pointer_value) const {
    CR_TypedValue ret;
    auto tid = ResolveAlias(pointer_value.m_type_id);
    if (IsPointerType(tid)) {
        auto& type = LogType(tid);
        auto tid2 = type.m_sub_id;
        auto& type2 = LogType(tid2);
        if (HasValue(pointer_value)) {
            auto addr = GetULongLongValue(pointer_value);
            void *ptr = &addr;
            ret.m_addr = make_shared<unsigned long long>(addr);
            SetValue(ret, tid2, ptr, type2.m_size);
        } else {
            ret.m_type_id = tid2;
            ret.m_size = type2.m_size;
        }
    }
    return ret;
}

CR_TypedValue
CR_NameScope::Address(const CR_TypedValue& value) const {
    CR_TypedValue ret;
    if (value.m_type_id != cr_invalid_id) {
        ret.m_type_id = m_void_ptr_type;
        if (value.m_addr) {
            unsigned long long ull = *value.m_addr.get();
            if (Is64Bit()) {
                ret.assign<unsigned long long>(ull);
                ret.m_extra = "LL";
            } else {
                ret.assign<unsigned int>(
                    static_cast<unsigned int>(ull));
            }
        }
    }
    return ret;
}

CR_TypedValue CR_NameScope::Arrow(
    const CR_TypedValue& pointer_value, const std::string& name) const
{
    CR_TypedValue ret = Asterisk(pointer_value);
    ret = Dot(ret, name);
    return ret;
}

void CR_NameScope::SetLongLongValue(CR_TypedValue& value, long long n) const {
    if (value.m_type_id == cr_invalid_id || value.m_size == 0) {
        return;
    }
    if (IsIntegralType(value.m_type_id) || IsPointerType(value.m_type_id)) {
        if (value.m_size == sizeof(int)) {
            value.assign<int>(static_cast<int>(n));
            value.m_extra = "";
        } else if (value.m_size == sizeof(char)) {
            value.assign<char>(static_cast<char>(n));
            value.m_extra = "i8";
        } else if (value.m_size == sizeof(short)) {
            value.assign<short>(static_cast<short>(n));
            value.m_extra = "i16";
        } else if (value.m_size == sizeof(long)) {
            value.assign<long>(static_cast<long>(n));
            value.m_extra = "L";
        } else if (value.m_size == sizeof(long long)) {
            value.assign<long long>(n);
            value.m_extra = "LL";
        }
    } else if (IsFloatingType(value.m_type_id)) {
        long double ld;
        if (value.m_size == sizeof(float)) {
            ld = static_cast<float>(n);
            value.assign<float>(static_cast<float>(n));
            value.m_extra = "F";
        } else if (value.m_size == sizeof(double)) {
            ld = static_cast<double>(n);
            value.assign<double>(static_cast<double>(n));
            value.m_extra = "";
        } else if (value.m_size == sizeof(long double)) {
            ld = static_cast<long double>(n);
            value.assign<long double>(static_cast<long double>(n));
            value.m_extra = "L";
        } else {
            return;
        }
        if (std::isinf(ld)) {
            if (ld >= 0) {
                value.m_text = "+INF";
            } else {
                value.m_text = "-INF";
            }
        } else if (std::isnan(ld)) {
            value.m_text = "NAN";
        } else {
            char buf[512];
            std::sprintf(buf, "%Lg", ld);
            value.m_text = buf;
        }
    }
}

void CR_NameScope::SetULongLongValue(CR_TypedValue& value, unsigned long long u) const {
    if (value.m_type_id == cr_invalid_id || value.m_size == 0) {
        return;
    }
    if (IsIntegralType(value.m_type_id) || IsPointerType(value.m_type_id)) {
        if (value.m_size == sizeof(int)) {
            value.assign<unsigned int>(static_cast<unsigned int>(u));
            value.m_extra = "u";
        } else if (value.m_size == sizeof(char)) {
            value.assign<unsigned char>(static_cast<unsigned char>(u));
            value.m_extra = "ui8";
        } else if (value.m_size == sizeof(short)) {
            value.assign<unsigned short>(static_cast<unsigned short>(u));
            value.m_extra = "ui16";
        } else if (value.m_size == sizeof(long)) {
            value.assign<unsigned long>(static_cast<unsigned long>(u));
            value.m_extra = "UL";
        } else if (value.m_size == sizeof(long long)) {
            value.assign<unsigned long long>(u);
            value.m_extra = "ULL";
        }
    } else if (IsFloatingType(value.m_type_id)) {
        long double ld;
        if (value.m_size == sizeof(float)) {
            ld = static_cast<float>(u);
            value.assign<float>(static_cast<float>(u));
            value.m_extra = "F";
        } else if (value.m_size == sizeof(double)) {
            ld = static_cast<double>(u);
            value.assign<double>(static_cast<double>(u));
            value.m_extra = "";
        } else if (value.m_size == sizeof(long double)) {
            ld = static_cast<long double>(u);
            value.assign<long double>(static_cast<long double>(u));
            value.m_extra = "L";
        } else {
            return;
        }
        if (std::isinf(ld)) {
            if (ld >= 0) {
                value.m_text = "+INF";
            } else {
                value.m_text = "-INF";
            }
        } else if (std::isnan(ld)) {
            value.m_text = "NAN";
        } else {
            char buf[512];
            std::sprintf(buf, "%Lg", ld);
            value.m_text = buf;
        }
    }
}

void CR_NameScope::SetLongDoubleValue(CR_TypedValue& value, long double ld) const {
    if (value.m_type_id == cr_invalid_id || value.m_size == 0) {
        return;
    }
    if (IsIntegralType(value.m_type_id)) {
        if (value.m_size == sizeof(int)) {
            value.assign<int>(static_cast<int>(ld));
            value.m_extra = "";
        } else if (value.m_size == sizeof(char)) {
            value.assign<char>(static_cast<char>(ld));
            value.m_extra = "i8";
        } else if (value.m_size == sizeof(short)) {
            value.assign<short>(static_cast<short>(ld));
            value.m_extra = "i16";
        } else if (value.m_size == sizeof(long)) {
            value.assign<long>(static_cast<long>(ld));
            value.m_extra = "i32";
        } else if (value.m_size == sizeof(long long)) {
            value.assign<long long>(static_cast<long long>(ld));
            value.m_extra = "i64";
        }
    } else if (IsFloatingType(value.m_type_id)) {
        if (value.m_size == sizeof(float)) {
            value.assign<float>(static_cast<float>(ld));
            value.m_extra = "F";
        } else if (value.m_size == sizeof(double)) {
            value.assign<double>(static_cast<double>(ld));
            value.m_extra = "";
        } else if (value.m_size == sizeof(long double)) {
            value.assign<long double>(ld);
            value.m_extra = "L";
        }
        if (std::isinf(ld)) {
            if (ld >= 0) {
                value.m_text = "+INF";
            } else {
                value.m_text = "-INF";
            }
        } else if (std::isnan(ld)) {
            value.m_text = "NAN";
        } else {
            char buf[512];
            std::sprintf(buf, "%Lg", ld);
            value.m_text = buf;
        }
    }
}

////////////////////////////////////////////////////////////////////////////
// calculations

void CR_NameScope::IntZero(CR_TypedValue& value1) const {
    value1.m_type_id = m_int_type;
    int n = 0;
    value1.assign<int>(n);
    value1.m_extra = "";
}

void CR_NameScope::IntOne(CR_TypedValue& value1) const {
    value1.m_type_id = m_int_type;
    int n = 1;
    value1.assign<int>(n);
    value1.m_extra = "";
}

bool CR_NameScope::IsZero(const CR_TypedValue& value1) const {
    if (!HasValue(value1)) {
        return false;
    }
    if (IsIntegralType(value1.m_type_id)) {
        auto ull = GetULongLongValue(value1);
        return ull == 0;
    } else if (IsFloatingType(value1.m_type_id)) {
        auto ld = GetLongDoubleValue(value1);
        return ld == 0;
    } else if (IsPointerType(value1.m_type_id)) {
        auto ull = GetULongLongValue(value1);
        return ull == 0;
    }
    return false;
}

bool CR_NameScope::IsNonZero(const CR_TypedValue& value1) const {
    return !IsZero(value1);
}

CR_TypedValue CR_NameScope::BiOp(CR_TypedValue& v1, CR_TypedValue& v2) const {
    CR_TypedValue ret;
    if (v1.m_type_id == cr_invalid_id || v2.m_type_id == cr_invalid_id) {
        ret.m_type_id = cr_invalid_id;
    } else if (IsIntegralType(v1.m_type_id)) {
        if (IsIntegralType(v2.m_type_id)) {
            int size1 = SizeOfType(v1.m_type_id);
            if (size1 < 4) {
                v1 = StaticCast(m_int_type, v1);
                size1 = 4;
            }
            int size2 = SizeOfType(v2.m_type_id);
            if (size2 < 4) {
                v2 = StaticCast(m_int_type, v2);
                size2 = 4;
            }
            if (size1 >= size2) {
                ret.m_type_id = v1.m_type_id;
                ret.m_size = size1;
            } else {
                ret.m_type_id = v2.m_type_id;
                ret.m_size = size2;
            }
        } else if (IsFloatingType(v2.m_type_id)) {
            v1 = StaticCast(v2.m_type_id, v1);
            ret.m_type_id = v2.m_type_id;
            ret.m_size = SizeOfType(v2.m_type_id);
        }
    } else if (IsFloatingType(v1.m_type_id)) {
        if (IsIntegralType(v2.m_type_id)) {
            v2 = StaticCast(v1.m_type_id, v2);
            ret.m_type_id = v1.m_type_id;
            ret.m_size = SizeOfType(v1.m_type_id);
        } else if (IsFloatingType(v2.m_type_id)) {
            int size1 = SizeOfType(v1.m_type_id);
            int size2 = SizeOfType(v2.m_type_id);
            if (size1 >= size2) {
                ret.m_type_id = v1.m_type_id;
                ret.m_size = size1;
            } else {
                ret.m_type_id = v2.m_type_id;
                ret.m_size = size2;
            }
        }
    }
    return ret;
}

int CR_NameScope::CompareValue(
    const CR_TypedValue& v1, const CR_TypedValue& v2) const
{
    if (v1.m_type_id == cr_invalid_id || v2.m_type_id == cr_invalid_id) {
        return -2;
    }
    if (!HasValue(v1) || !HasValue(v2)) {
        return -2;
    }
    bool is_floating = false;
    if (IsIntegralType(v1.m_type_id)) {
        if (IsIntegralType(v2.m_type_id)) {
            bool un1 = IsUnsignedType(v1.m_type_id);
            bool un2 = IsUnsignedType(v2.m_type_id);
            if (un1) {
                if (un2) {
                    auto u1 = GetULongLongValue(v1);
                    auto u2 = GetULongLongValue(v2);
                    if (u1 < u2) return -1;
                    if (u1 > u2) return 1;
                } else {
                    auto u1 = GetULongLongValue(v1);
                    auto n2 = GetLongLongValue(v2);
                    auto n1 = static_cast<long long>(u1);
                    if (n1 < n2) return -1;
                    if (n1 > n2) return 1;
                }
            } else {
                if (un2) {
                    auto n1 = GetLongLongValue(v1);
                    auto u2 = GetULongLongValue(v2);
                    auto n2 = static_cast<long long>(u2);
                    if (n1 < n2) return -1;
                    if (n1 > n2) return 1;
                } else {
                    auto n1 = GetLongLongValue(v1);
                    auto n2 = GetLongLongValue(v2);
                    if (n1 < n2) return -1;
                    if (n1 > n2) return 1;
                }
            }
        } else if (IsFloatingType(v2.m_type_id)) {
            is_floating = true;
        }
    } else if (IsFloatingType(v1.m_type_id)) {
        is_floating = true;
    } else if (IsPointerType(v1.m_type_id)) {
        if (IsPointerType(v2.m_type_id)) {
            auto u1 = GetULongLongValue(v1);
            auto u2 = GetULongLongValue(v2);
            if (u1 < u2) return -1;
            if (u1 > u2) return 1;
        }
    }
    if (is_floating) {
        auto ld1 = GetLongDoubleValue(v1);
        auto ld2 = GetLongDoubleValue(v2);
        if (ld1 < ld2) return -1;
        if (ld1 > ld2) return 1;
    }
    return 0;
}

CR_TypedValue
CR_NameScope::BiOpInt(CR_TypedValue& v1, CR_TypedValue& v2) const {
    CR_TypedValue ret;
    if (v1.m_type_id == cr_invalid_id || v2.m_type_id == cr_invalid_id) {
        ret.m_type_id = cr_invalid_id;
    } else if (IsIntegralType(v1.m_type_id) && IsIntegralType(v2.m_type_id)) {
        int size1 = SizeOfType(v1.m_type_id);
        if (size1 < 4) {
            v1 = StaticCast(m_int_type, v1);
            size1 = 4;
        }
        int size2 = SizeOfType(v2.m_type_id);
        if (size2 < 4) {
            v2 = StaticCast(m_int_type, v2);
            size2 = 4;
        }
        if (size1 >= size2) {
            ret.m_type_id = v1.m_type_id;
            ret.m_size = size1;
        } else {
            ret.m_type_id = v2.m_type_id;
            ret.m_size = size2;
        }
    }
    return ret;
}

CR_TypedValue
CR_NameScope::Add(const CR_TypedValue& value1,
                  const CR_TypedValue& value2) const
{
    CR_TypedValue v1 = value1, v2 = value2;
    auto ret = BiOp(v1, v2);
    if (ret.m_type_id != cr_invalid_id) {
        if (HasValue(v1) && HasValue(v2)) {
            if (IsFloatingType(ret.m_type_id)) {
                auto ld1 = GetLongDoubleValue(v1);
                auto ld2 = GetLongDoubleValue(v2);
                SetLongDoubleValue(ret, ld1 + ld2);
            } else if (IsIntegralType(ret.m_type_id)) {
                auto n1 = GetLongLongValue(v1);
                auto n2 = GetLongLongValue(v2);
                if (IsUnsignedType(ret.m_type_id)) {
                    SetULongLongValue(ret, n1 + n2);
                } else {
                    SetLongLongValue(ret, n1 + n2);
                }
            }
        }
    } else {
        if (IsIntegralType(v1.m_type_id) && IsPointerType(v2.m_type_id)) {
            std::swap(v1, v2);
        }
        if (IsPointerType(v1.m_type_id) && IsIntegralType(v2.m_type_id)) {
            v1.m_type_id = ResolveAliasAndCV(v1.m_type_id);
            auto& type1 = LogType(v1.m_type_id);
            auto& type2 = LogType(type1.m_sub_id);
            ret.m_type_id = v1.m_type_id;
            ret.m_size = v1.m_size;
            if (HasValue(v1) && HasValue(v2)) {
                if (IsUnsignedType(v2.m_type_id)) {
                    auto u1 = GetULongLongValue(v1);
                    auto u2 = GetULongLongValue(v2);
                    SetULongLongValue(ret, u1 + u2 * type2.m_size);
                } else {
                    auto u1 = GetULongLongValue(v1);
                    auto n2 = GetLongLongValue(v2);
                    SetULongLongValue(ret, u1 + n2 * type2.m_size);
                }
            }
        }
    }
    return ret;
}

CR_TypedValue
CR_NameScope::Sub(const CR_TypedValue& value1,
                  const CR_TypedValue& value2) const
{
    CR_TypedValue v1 = value1, v2 = value2;
    auto ret = BiOp(v1, v2);
    if (ret.m_type_id != cr_invalid_id) {
        if (HasValue(v1) && HasValue(v2)) {
            if (IsFloatingType(ret.m_type_id)) {
                auto ld1 = GetLongDoubleValue(v1);
                auto ld2 = GetLongDoubleValue(v2);
                SetLongDoubleValue(ret, ld1 - ld2);
            } else if (IsIntegralType(ret.m_type_id)) {
                auto n1 = GetLongLongValue(v1);
                auto n2 = GetLongLongValue(v2);
                if (IsUnsignedType(ret.m_type_id)) {
                    SetULongLongValue(ret, n1 - n2);
                } else {
                    SetLongLongValue(ret, n1 - n2);
                }
            }
        }
    } else {
        if (IsPointerType(v1.m_type_id) && IsPointerType(v2.m_type_id)) {
            v1.m_type_id = ResolveAliasAndCV(v1.m_type_id);
            auto& type1_1 = LogType(v1.m_type_id);
            auto& type1_2 = LogType(type1_1.m_sub_id);
            v2.m_type_id = ResolveAliasAndCV(v2.m_type_id);
            auto& type2_1 = LogType(v2.m_type_id);
            auto& type2_2 = LogType(type2_1.m_sub_id);
            if (type1_2.m_size == type2_2.m_size) {
                ret.m_type_id = v1.m_type_id;
                ret.m_size = v1.m_size;
                if (HasValue(v1) && HasValue(v2)) {
                    auto u1 = GetULongLongValue(v1);
                    auto u2 = GetULongLongValue(v2);
                    SetULongLongValue(ret, (u1 - u2) / type1_2.m_size);
                }
            }
        }
    }
    return ret;
}

CR_TypedValue
CR_NameScope::Mul(const CR_TypedValue& value1,
                  const CR_TypedValue& value2) const
{
    CR_TypedValue v1 = value1, v2 = value2;
    auto ret = BiOp(v1, v2);
    if (ret.m_type_id != cr_invalid_id) {
        if (HasValue(v1) && HasValue(v2)) {
            if (IsFloatingType(ret.m_type_id)) {
                auto ld1 = GetLongDoubleValue(v1);
                auto ld2 = GetLongDoubleValue(v2);
                SetLongDoubleValue(ret, ld1 * ld2);
            } else if (IsIntegralType(ret.m_type_id)) {
                auto n1 = GetLongLongValue(v1);
                auto n2 = GetLongLongValue(v2);
                if (IsUnsignedType(ret.m_type_id)) {
                    SetULongLongValue(ret, n1 * n2);
                } else {
                    SetLongLongValue(ret, n1 * n2);
                }
            }
        }
    }
    return ret;
}

CR_TypedValue
CR_NameScope::Div(const CR_TypedValue& value1,
                  const CR_TypedValue& value2) const
{
    CR_TypedValue v1 = value1, v2 = value2;
    auto ret = BiOp(v1, v2);
    if (ret.m_type_id != cr_invalid_id) {
        if (HasValue(v1) && HasValue(v2)) {
            if (IsFloatingType(ret.m_type_id)) {
                auto ld1 = GetLongDoubleValue(v1);
                auto ld2 = GetLongDoubleValue(v2);
                SetLongDoubleValue(ret, ld1 / ld2);
            } else if (IsIntegralType(ret.m_type_id)) {
                auto n1 = GetLongLongValue(v1);
                auto n2 = GetLongLongValue(v2);
                if (IsUnsignedType(ret.m_type_id)) {
                    SetULongLongValue(ret, n1 / n2);
                } else {
                    SetLongLongValue(ret, n1 / n2);
                }
            }
        }
    }
    return ret;
}

CR_TypedValue
CR_NameScope::Mod(const CR_TypedValue& value1,
                  const CR_TypedValue& value2) const
{
    CR_TypedValue v1 = value1, v2 = value2;
    auto ret = BiOpInt(v1, v2);
    if (ret.m_type_id != cr_invalid_id) {
        if (HasValue(v1) && HasValue(v2)) {
            if (IsIntegralType(ret.m_type_id)) {
                auto n1 = GetLongLongValue(v1);
                auto n2 = GetLongLongValue(v2);
                if (IsUnsignedType(ret.m_type_id)) {
                    SetULongLongValue(ret, n1 / n2);
                } else {
                    SetLongLongValue(ret, n1 / n2);
                }
            }
        }
    }
    return ret;
}

CR_TypedValue
CR_NameScope::Not(const CR_TypedValue& value1) const {
    auto ret = value1;
    if (IsIntegralType(ret.m_type_id) && HasValue(ret)) {
        if (ret.m_size < sizeof(int)) {
            ret = StaticCast(m_int_type, ret);
        }
        if (IsUnsignedType(ret.m_type_id)) {
            auto ull = GetULongLongValue(ret);
            SetULongLongValue(ret, ~ull);
        } else {
            auto ll = GetLongLongValue(ret);
            SetLongLongValue(ret, ~ll);
        }
    }
    return ret;
}

CR_TypedValue
CR_NameScope::Minus(const CR_TypedValue& value1) const {
    auto ret = value1;
    if (HasValue(ret)) {
        if (IsIntegralType(ret.m_type_id)) {
            if (ret.m_size < sizeof(int)) {
                ret = StaticCast(m_int_type, ret);
            }
            if (IsUnsignedType(ret.m_type_id)) {
                auto u = GetULongLongValue(ret);
                SetULongLongValue(ret, -static_cast<long long>(u));
            } else {
                auto n = GetLongLongValue(ret);
                SetLongLongValue(ret, -n);
            }
        } else if (IsFloatingType(ret.m_type_id)) {
            auto ld = GetLongDoubleValue(ret);
            SetLongDoubleValue(ret, -ld);
        }
    }
    return ret;
}

CR_TypedValue
CR_NameScope::And(const CR_TypedValue& value1,
                  const CR_TypedValue& value2) const
{
    CR_TypedValue v1 = value1, v2 = value2;
    auto ret = BiOpInt(v1, v2);
    if (ret.m_type_id != cr_invalid_id && HasValue(v1) && HasValue(v2)) {
        if (IsIntegralType(ret.m_type_id)) {
            auto n1 = GetLongLongValue(v1);
            auto n2 = GetLongLongValue(v2);
            if (IsUnsignedType(ret.m_type_id)) {
                SetULongLongValue(ret, n1 & n2);
            } else {
                SetLongLongValue(ret, n1 & n2);
            }
        }
    }
    return ret;
}

CR_TypedValue
CR_NameScope::Or(const CR_TypedValue& value1,
                 const CR_TypedValue& value2) const
{
    CR_TypedValue v1 = value1, v2 = value2;
    auto ret = BiOpInt(v1, v2);
    if (ret.m_type_id != cr_invalid_id && HasValue(v1) && HasValue(v2)) {
        if (IsIntegralType(ret.m_type_id)) {
            auto n1 = GetLongLongValue(v1);
            auto n2 = GetLongLongValue(v2);
            if (IsUnsignedType(ret.m_type_id)) {
                SetULongLongValue(ret, n1 | n2);
            } else {
                SetLongLongValue(ret, n1 | n2);
            }
        }
    }
    return ret;
}

CR_TypedValue
CR_NameScope::Xor(const CR_TypedValue& value1,
                  const CR_TypedValue& value2) const
{
    CR_TypedValue v1 = value1, v2 = value2;
    auto ret = BiOpInt(v1, v2);
    if (ret.m_type_id != cr_invalid_id && HasValue(v1) && HasValue(v2)) {
        if (IsIntegralType(ret.m_type_id)) {
            auto n1 = GetLongLongValue(v1);
            auto n2 = GetLongLongValue(v2);
            if (IsUnsignedType(ret.m_type_id)) {
                SetULongLongValue(ret, n1 ^ n2);
            } else {
                SetLongLongValue(ret, n1 ^ n2);
            }
        }
    }
    return ret;
}

CR_TypedValue
CR_NameScope::Eq(const CR_TypedValue& value1,
                 const CR_TypedValue& value2) const
{
    CR_TypedValue ret;
    if (!HasValue(value1) || !HasValue(value2)) {
        return ret;
    }
    int compare = CompareValue(value1, value2);
    if (compare == 0) {
        IntOne(ret);
    } else {
        IntZero(ret);
    }
    return ret;
}

CR_TypedValue
CR_NameScope::Ne(const CR_TypedValue& value1, const CR_TypedValue& value2) const
{
    CR_TypedValue ret;
    if (!HasValue(value1) || !HasValue(value2)) {
        return ret;
    }
    int compare = CompareValue(value1, value2);
    if (compare != 0) {
        IntOne(ret);
    } else {
        IntZero(ret);
    }
    return ret;
}

CR_TypedValue
CR_NameScope::Gt(const CR_TypedValue& value1, const CR_TypedValue& value2) const
{
    CR_TypedValue ret;
    if (!HasValue(value1) || !HasValue(value2)) {
        return ret;
    }
    int compare = CompareValue(value1, value2);
    if (compare > 0) {
        IntOne(ret);
    } else {
        IntZero(ret);
    }
    return ret;
}

CR_TypedValue
CR_NameScope::Lt(const CR_TypedValue& value1, const CR_TypedValue& value2) const
{
    CR_TypedValue ret;
    if (!HasValue(value1) || !HasValue(value2)) {
        return ret;
    }
    int compare = CompareValue(value1, value2);
    if (compare < 0) {
        IntOne(ret);
    } else {
        IntZero(ret);
    }
    return ret;
}

CR_TypedValue
CR_NameScope::Ge(const CR_TypedValue& value1, const CR_TypedValue& value2) const
{
    CR_TypedValue ret;
    if (!HasValue(value1) || !HasValue(value2)) {
        return ret;
    }
    int compare = CompareValue(value1, value2);
    if (compare >= 0) {
        IntOne(ret);
    } else {
        IntZero(ret);
    }
    return ret;
}

CR_TypedValue
CR_NameScope::Le(const CR_TypedValue& value1, const CR_TypedValue& value2) const
{
    CR_TypedValue ret;
    if (!HasValue(value1) || !HasValue(value2)) {
        return ret;
    }
    int compare = CompareValue(value1, value2);
    if (compare <= 0) {
        IntOne(ret);
    } else {
        IntZero(ret);
    }
    return ret;
}

CR_TypedValue
CR_NameScope::Shl(const CR_TypedValue& value1, const CR_TypedValue& value2) const
{
    CR_TypedValue v1 = value1, v2 = value2;
    auto ret = BiOpInt(v1, v2);
    if (ret.m_type_id != cr_invalid_id) {
        if (IsIntegralType(ret.m_type_id)) {
            if (IsUnsignedType(v1.m_type_id)) {
                auto u1 = GetULongLongValue(v1);
                auto n2 = GetLongLongValue(v2);
                SetULongLongValue(ret, u1 << static_cast<int>(n2));
                ret.m_type_id = MakeUnsigned(ret.m_type_id);
            } else {
                auto n1 = GetLongLongValue(v1);
                auto n2 = GetLongLongValue(v2);
                SetLongLongValue(ret, n1 << static_cast<int>(n2));
                ret.m_type_id = MakeSigned(ret.m_type_id);
            }
        }
    }
    return ret;
}

CR_TypedValue
CR_NameScope::Shr(const CR_TypedValue& value1, const CR_TypedValue& value2) const
{
    CR_TypedValue v1 = value1, v2 = value2;
    auto ret = BiOpInt(v1, v2);
    if (ret.m_type_id != cr_invalid_id) {
        if (IsIntegralType(ret.m_type_id)) {
            if (IsUnsignedType(v1.m_type_id)) {
                auto u1 = GetULongLongValue(v1);
                auto n2 = GetLongLongValue(v2);
                SetULongLongValue(ret, u1 >> static_cast<int>(n2));
                ret.m_type_id = MakeUnsigned(ret.m_type_id);
            } else {
                auto n1 = GetLongLongValue(v1);
                auto n2 = GetLongLongValue(v2);
                SetLongLongValue(ret, n1 >> static_cast<int>(n2));
                ret.m_type_id = MakeSigned(ret.m_type_id);
            }
        }
    }
    return ret;
}

CR_TypedValue
CR_NameScope::LNot(const CR_TypedValue& value1) const {
    CR_TypedValue ret;
    if (IsZero(value1)) {
        IntOne(ret);
    } else {
        IntZero(ret);
    }
    return ret;
}

CR_TypedValue
CR_NameScope::LAnd(const CR_TypedValue& value1, const CR_TypedValue& value2) const
{
    CR_TypedValue ret;
    if (IsZero(value1) || IsZero(value2)) {
        IntZero(ret);
    } else {
        IntOne(ret);
    }
    return ret;
}

CR_TypedValue
CR_NameScope::LOr(const CR_TypedValue& value1, const CR_TypedValue& value2) const
{
    CR_TypedValue ret;
    if (IsNonZero(value1) || IsNonZero(value2)) {
        IntOne(ret);
    } else {
        IntZero(ret);
    }
    return ret;
}

int CR_NameScope::GetIntValue(const CR_TypedValue& value) const {
    if (HasValue(value)) {
        auto int_value = StaticCast(m_int_type, value);
        if (HasValue(int_value)) {
            return int_value.get<int>();
        }
    }
    return 0;
}

void CR_NameScope::SetIntValue(CR_TypedValue& value, int n) const {
    value.m_type_id = m_int_type;
    value.assign<int>(n);
}

bool CR_NameScope::HasValue(const CR_TypedValue& value) const {
    if (value.m_text.size() && value.m_text[0] == '\"') {
        return true;
    }
    if (value.m_ptr == NULL || value.m_type_id == cr_invalid_id) {
        return false;
    }
    auto& type = LogType(value.m_type_id);
    return static_cast<int>(value.m_size) >= type.m_size;
}

void CR_NameScope::SetValue(
    CR_TypedValue& value, CR_TypeID tid, const void *ptr, size_t size) const
{
    value.m_type_id = tid;
    value.m_size = size;
    if (ptr == NULL) {
        value.clear();
        return;
    }
    if (IsIntegralType(tid)) {
        if (IsUnsignedType(tid)) {
            if (size == sizeof(int)) {
                value.assign<unsigned int>(
                    *reinterpret_cast<const unsigned int *>(ptr));
                value.m_extra = "U";
            } else if (size == sizeof(char)) {
                value.assign<unsigned char>(
                    *reinterpret_cast<const unsigned char *>(ptr));
                value.m_extra = "ui8";
            } else if (size == sizeof(short)) {
                value.assign<unsigned short>(
                    *reinterpret_cast<const unsigned short *>(ptr));
                value.m_extra = "ui16";
            } else if (size == sizeof(long)) {
                value.assign<unsigned long>(
                    *reinterpret_cast<const unsigned long *>(ptr));
                value.m_extra = "UL";
            } else if (size == sizeof(long long)) {
                value.assign<unsigned long long>(
                    *reinterpret_cast<const unsigned long long *>(ptr));
                value.m_extra = "ULL";
            }
        } else {
            if (size == sizeof(int)) {
                value.assign<int>(
                    *reinterpret_cast<const int *>(ptr));
                value.m_extra = "";
            } else if (size == sizeof(char)) {
                value.assign<char>(
                    *reinterpret_cast<const char *>(ptr));
                value.m_extra = "i8";
            } else if (size == sizeof(short)) {
                value.assign<short>(
                    *reinterpret_cast<const short *>(ptr));
                value.m_extra = "i16";
            } else if (size == sizeof(long)) {
                value.assign<long>(
                    *reinterpret_cast<const long *>(ptr));
                value.m_extra = "L";
            } else if (size == sizeof(long long)) {
                value.assign<long long>(
                    *reinterpret_cast<const long long *>(ptr));
                value.m_extra = "LL";
            }
        }
    } else if (IsFloatingType(tid)){
        long double ld;
        if (size == sizeof(float)) {
            ld = *reinterpret_cast<const float *>(ptr);
            value.assign<float>(*reinterpret_cast<const float *>(ptr));
            value.m_extra = "f";
        } else if (size == sizeof(double)) {
            ld = *reinterpret_cast<const double *>(ptr);
            value.assign<double>(*reinterpret_cast<const double *>(ptr));
            value.m_extra = "";
        } else if (size == sizeof(long double)) {
            ld = *reinterpret_cast<const long double *>(ptr);
            long double ld = *reinterpret_cast<const long double *>(ptr);
            value.assign<long double>(ld);
            value.m_extra = "L";
        } else {
            return;
        }
        if (std::isinf(ld)) {
            if (ld >= 0) {
                value.m_text = "+INF";
            } else {
                value.m_text = "-INF";
            }
        } else if (std::isnan(ld)) {
            value.m_text = "NAN";
        } else {
            char buf[512];
            std::sprintf(buf, "%Lg", ld);
            value.m_text = buf;
        }
    } else if (IsPointerType(tid)) {
        auto& type = LogType(tid);
        if (type.m_size == 8) {
            auto n = reinterpret_cast<unsigned long long>(ptr);
            value.assign<unsigned long long>(n);
            value.m_extra = "LL";
        } else if (type.m_size == 4) {
            auto n = static_cast<unsigned int>(reinterpret_cast<size_t>(ptr));
            value.assign<unsigned int>(n);
            value.m_extra = "";
        }
    } else {
        value.assign(ptr, size);
        value.m_text.clear();
    }
}

CR_TypeID CR_NameScope::MakeSigned(CR_TypeID tid) const {
    tid = ResolveAlias(tid);
    if (IsIntegralType(tid)) {
        auto& type = LogType(tid);
        if (type.m_flags & TF_CONST) {
            if (type.m_size == sizeof(int)) {
                return m_const_int_type;
            } else if (type.m_size == sizeof(char)) {
                return m_const_char_type;
            } else if (type.m_size == sizeof(short)) {
                return m_const_short_type;
            } else if (type.m_size == sizeof(long)) {
                return m_const_long_type;
            } else if (type.m_size == sizeof(long long)) {
                return m_const_long_long_type;
            }
        } else {
            if (type.m_size == sizeof(int)) {
                return m_int_type;
            } else if (type.m_size == sizeof(char)) {
                return m_char_type;
            } else if (type.m_size == sizeof(short)) {
                return m_short_type;
            } else if (type.m_size == sizeof(long)) {
                return m_long_type;
            } else if (type.m_size == sizeof(long long)) {
                return m_long_long_type;
            }
        }
    }
    return cr_invalid_id;
}

CR_TypeID CR_NameScope::MakeUnsigned(CR_TypeID tid) const {
    tid = ResolveAlias(tid);
    if (IsIntegralType(tid)) {
        auto& type = LogType(tid);
        if (type.m_flags & TF_CONST) {
            if (type.m_size == sizeof(int)) {
                return m_const_uint_type;
            } else if (type.m_size == sizeof(char)) {
                return m_const_uchar_type;
            } else if (type.m_size == sizeof(short)) {
                return m_const_ushort_type;
            } else if (type.m_size == sizeof(long)) {
                return m_const_ulong_type;
            } else if (type.m_size == sizeof(long long)) {
                return m_const_ulong_long_type;
            }
        } else {
            if (type.m_size == sizeof(int)) {
                return m_uint_type;
            } else if (type.m_size == sizeof(char)) {
                return m_uchar_type;
            } else if (type.m_size == sizeof(short)) {
                return m_ushort_type;
            } else if (type.m_size == sizeof(long)) {
                return m_ulong_type;
            } else if (type.m_size == sizeof(long long)) {
                return m_ulong_long_type;
            }
        }
    }
    return cr_invalid_id;
}

CR_TypeID CR_NameScope::MakeConst(CR_TypeID tid) const {
    if (tid == cr_invalid_id) {
        return tid;
    }
    CR_LogType type1;
    auto& type2 = LogType(tid);
    if (type2.m_flags & TF_CONST) {
        // it is already a constant
        return tid;
    }
    if (type2.m_flags & TF_INCOMPLETE) {
        type1.m_flags = TF_CONST | TF_INCOMPLETE;
    } else {
        type1.m_flags = TF_CONST;
        type1.m_size = type2.m_size;
        type1.m_align = type2.m_align;
    }
    if (type2.m_flags & TF_BITFIELD) {
        type1.m_flags |= TF_BITFIELD;
    }
    type1.m_sub_id = tid;
    auto tid2 = m_types.Find(type1);
    return tid2;
}

CR_TypedValue
CR_NameScope::PConstant(CR_TypeID tid, const std::string& text, const std::string& extra) {
    CR_TypedValue ret;

    ret.m_type_id = tid;
    ret.m_extra = extra;

    unsigned long long ull;
    if (text[0] == '-') {
        ull = std::strtoull(text.data() + 1, NULL, 0);
        auto ll = -static_cast<long long>(ull);
        ull = ll;
    } else {
        ull = std::stoull(text, NULL, 0);
    }

    auto& type = LogType(tid);
    if (type.m_size == 8) {
        ret.assign<unsigned long long>(ull);
    } else if (type.m_size == 4) {
        ret.assign<unsigned int>(static_cast<unsigned int>(ull));
    }
    return ret;
}

CR_TypedValue
CR_NameScope::FConstant(const std::string& text, const std::string& extra) {
    CR_TypedValue ret;

    long double ld;

    if (text == "INF" || text == "+INF") {
        ld = +std::numeric_limits<long double>::infinity();
    } else if (text == "-INF") {
        ld = -std::numeric_limits<long double>::infinity();
    } else if (text == "NAN" || text == "+NAN") {
        static_assert(std::numeric_limits<long double>::has_quiet_NaN, "no quiet NAN");
        ld = +std::numeric_limits<long double>::quiet_NaN();
        // ld = +NAN;
    } else if (text == "-NAN") {
        static_assert(std::numeric_limits<long double>::has_quiet_NaN, "no quiet NAN");
        ld = -std::numeric_limits<long double>::quiet_NaN();
        // ld = -NAN;
    } else {
        try {
            ld = std::stold(text, NULL);
        } catch (std::out_of_range&) {
            return ret;
        }
    }

    ret.m_text = text;

    if (extra.find('f') != std::string::npos ||
        extra.find('F') != std::string::npos)
    {
        ret.m_type_id = m_float_type;
        ret.assign<float>(static_cast<float>(ld));
        ret.m_extra = "F";
    } else if (extra.find('l') != std::string::npos ||
               extra.find('L') != std::string::npos)
    {
        ret.m_type_id = m_long_double_type;
        ret.assign<long double>(ld);
        ret.m_extra = "L";
    } else {
        ret.m_type_id = m_double_type;
        ret.assign<double>(static_cast<double>(ld));
        ret.m_extra = "";
    }
    if (std::isinf(ld)) {
        if (ld >= 0) {
            ret.m_text = "+INF";
        } else {
            ret.m_text = "-INF";
        }
        ret.m_extra = "";
    } else if (std::isnan(ld)) {
        ret.m_text = "NAN";
        ret.m_extra = "";
    } else {
        char buf[512];
        std::sprintf(buf, "%Lg", ld);
        ret.m_text = buf;
        ret.m_extra = extra;
    }
    return ret;
}

CR_TypedValue
CR_NameScope::FConstant(CR_TypeID tid, const std::string& text, const std::string& extra) {
    CR_TypedValue ret = FConstant(text, extra);
    ret = StaticCast(tid, ret);
    if (ret.m_extra.empty()) {
        if (ret.m_size == sizeof(float)) {
            ret.m_extra += "f";
        } else if (ret.m_size == sizeof(long double)) {
            ret.m_extra += "L";
        }
    }
    return ret;
}

CR_TypedValue
CR_NameScope::SConstant(const std::string& text, const std::string& extra) {
    CR_TypedValue ret;
    ret.m_text = text;
    ret.m_extra = extra;
    if (extra.find("L") != std::string::npos ||
        extra.find("l") != std::string::npos)
    {
        std::wstring wstr = CrUnescapeStringA2W(text);
        ret.m_type_id = AddConstWCharArray(wstr.size() + 1);
        ret.assign(wstr.data(), (wstr.size() + 1) * sizeof(WCHAR));
    } else {
        std::string str = CrUnescapeStringA2A(text);
        ret.m_type_id = AddConstCharArray(str.size() + 1);
        ret.assign(str.data(), str.size() + 1);
    }
    return ret;
}

CR_TypedValue
CR_NameScope::SConstant(CR_TypeID tid, const std::string& text, const std::string& extra) {
    CR_TypedValue ret;
    ret.m_text = text;
    ret.m_extra = extra;
    ret.m_type_id = tid;

    if (extra.find("L") != std::string::npos ||
        extra.find("l") != std::string::npos)
    {
        std::wstring wstr = CrUnescapeStringA2W(text);
        ret.assign(wstr.data(), (wstr.size() + 1) * sizeof(WCHAR));
        ret.m_size = SizeOfType(tid);
    } else {
        std::string str = CrUnescapeStringA2A(text);
        ret.assign(str.data(), str.size() + 1);
        ret.m_size = SizeOfType(tid);
    }
    return ret;
}

CR_TypedValue
CR_NameScope::IConstant(const std::string& text, const std::string& extra) {
    CR_TypedValue ret;
    if (text.empty()) {
        return ret;
    }

    ret.m_extra = extra;

    bool minus = false;
    unsigned long long ull;
    long long ll = 0;
    if (text[0] == '-') {
        ull = std::strtoull(text.data() + 1, NULL, 0);
        ll = static_cast<long long>(ull);
        ull = -ll;
        minus = true;
    } else {
        ull = std::stoull(text, NULL, 0);
    }

    bool is_unsigned = false;
    if (extra.find("U") != std::string::npos ||
        extra.find("u") != std::string::npos)
    {
        is_unsigned = true;
    }

    if (extra.find("LL") != std::string::npos ||
        extra.find("ll") != std::string::npos ||
        extra.find("i64") != std::string::npos)
    {
        if (is_unsigned) {
            ret.m_type_id = m_ulong_long_type;
            ret.assign<unsigned long long>(ull);
        } else {
            ret.m_type_id = m_long_long_type;
            ret.assign<long long>(ull);
        }
    } else if (extra.find('L') != std::string::npos ||
               extra.find('l') != std::string::npos ||
               extra.find("i32") != std::string::npos)
    {
        if (is_unsigned) {
            ret.m_type_id = m_ulong_type;
            ret.assign<unsigned long>(static_cast<unsigned long>(ull));
        } else {
            ret.m_type_id = m_long_type;
            ret.assign<long>(static_cast<long>(ull));
        }
    } else if (extra.find("i16") != std::string::npos) {
        if (is_unsigned) {
            ret.m_type_id = m_ushort_type;
            ret.assign<unsigned short>(static_cast<unsigned short>(ull));
        } else {
            ret.m_type_id = m_short_type;
            ret.assign<short>(static_cast<short>(ull));
        }
    } else if (extra.find("i8") != std::string::npos) {
        if (is_unsigned) {
            ret.m_type_id = m_uchar_type;
            ret.assign<unsigned char>(static_cast<unsigned char>(ull));
        } else {
            ret.m_type_id = m_char_type;
            ret.assign<char>(static_cast<char>(ull));
        }
    } else {
        if (is_unsigned) {
            if (ull & 0xFFFFFFFF00000000) {
                ret.m_type_id = m_ulong_long_type;
                ret.assign<unsigned long long>(ull);
                ret.m_extra = "ULL";
            } else {
                ret.m_type_id = m_uint_type;
                ret.assign<unsigned int>(static_cast<unsigned int>(ull));
                ret.m_extra = "U";
            }
        } else {
            if (minus) {
                if (ll & 0xFFFFFFFF00000000) {
                    ret.m_type_id = m_long_long_type;
                    ret.assign<long long>(ull);
                    ret.m_extra = "LL";
                } else {
                    ret.m_type_id = m_uint_type;
                    ret.assign<int>(static_cast<unsigned int>(ull));
                    ret.m_extra = "U";
                }
            } else {
                if (ull & 0x8000000000000000) {
                    ret.m_type_id = m_ulong_long_type;
                    ret.assign<unsigned long long>(ull);
                    ret.m_extra = "ULL";
                } else if (ull & 0x7FFFFFFF00000000) {
                    ret.m_type_id = m_long_long_type;
                    ret.assign<long long>(ull);
                    ret.m_extra = "LL";
                } else if (ull & 0x80000000) {
                    ret.m_type_id = m_uint_type;
                    ret.assign<unsigned int>(
                        static_cast<unsigned int>(ull));
                    ret.m_extra = "U";
                } else {
                    ret.m_type_id = m_int_type;
                    ret.assign<int>(static_cast<int>(ull));
                    ret.m_extra = "";
                }
            }
        }
    }
    return ret;
}

CR_TypedValue
CR_NameScope::IConstant(CR_TypeID tid, const std::string& text, const std::string& extra) {
    CR_TypedValue ret = IConstant(text, extra);
    ret = StaticCast(tid, ret);
    if (ret.m_extra.empty()) {
        if (IsUnsignedType(ret.m_type_id)) {
            ret.m_extra += "u";
        }
        if (ret.m_size == 1) {
            ret.m_extra += "i8";
        } else if (ret.m_size == 2) {
            ret.m_extra += "i16";
        } else if (ret.m_size == 4) {
            ret.m_extra += "i32";
        } else if (ret.m_size == 8) {
            ret.m_extra += "i64";
        }
    }
    return ret;
}

void CR_NameScope::clear() {
    m_types.clear();
    m_structs.clear();
    m_enums.clear();
    m_funcs.clear();
    m_mNameToTypeID.clear();
    m_mTypeIDToName.clear();
    m_mNameToVarID.clear();
    m_mVarIDToName.clear();
    m_mNameToName.clear();
}

////////////////////////////////////////////////////////////////////////////

// Wonders API data format version
int cr_data_version = 4;

bool CR_NameScope::LoadFromFiles(
    const std::string& prefix/* = ""*/,
    const std::string& suffix/* = ".dat"*/)
{
    clear();

    std::string fname, line;

    fname = prefix + "types" + suffix;
    std::ifstream in1(fname);
    if (in1) {
        // version check
        std::getline(in1, line);
        int version = std::stoi(line);
        if (version != cr_data_version) {
            ErrorInfo()->add_error("File '" + fname +
                "' has different format version. Failed.");
            return false;
        }
        // skip header
        std::getline(in1, line);
        // load body
        for (; std::getline(in1, line); ) {
            katahiromz::chomp(line);
            std::vector<std::string> fields;
            katahiromz::split_by_char(fields, line, '\t');

            if (fields.size() != 12) {
                ErrorInfo()->add_error("File '" + fname + "' is invalid.");
                return false;
            }

            CR_TypeID type_id = std::stol(fields[0], NULL, 0);
            std::string name = fields[1];
            CR_TypeFlags flags = std::stoul(fields[2], NULL, 0);
            CR_TypeID sub_id = std::stol(fields[3], NULL, 0);
            int count = std::stol(fields[4], NULL, 0);
            int size = std::stol(fields[5], NULL, 0);
            int align = std::stol(fields[6], NULL, 0);
            int alignas_ = std::stol(fields[7], NULL, 0);
            bool alignas_explicit = !!std::stol(fields[8], NULL, 0);
            bool is_macro = !!std::stol(fields[9], NULL, 0);
            std::string file = fields[10];
            int lineno = std::stol(fields[11], NULL, 0);

            if (name.size()) {
                MapNameToTypeID()[name] = type_id;
            }
            MapTypeIDToName()[type_id] = name;

            CR_LogType type;
            type.m_flags = flags;
            type.m_sub_id = sub_id;
            type.m_count = count;
            type.m_size = size;
            type.m_align = align;
            type.m_alignas = alignas_;
            type.m_alignas_explicit = alignas_explicit;
            type.m_location.set(file, lineno);
            type.m_is_macro = is_macro;
            m_types.emplace_back(type);
        }
    } else {
        ErrorInfo()->add_error("Cannot load file '" + fname + "'");
        return false;
    }

    fname = prefix + "structures" + suffix;
    std::ifstream in2(fname);
    if (in2) {
        // version check
        std::getline(in2, line);
        int version = std::stoi(line);
        if (version != cr_data_version) {
            ErrorInfo()->add_error("File '" + fname +
                "' has different format version. Failed.");
            return false;
        }
        // skip header
        std::getline(in2, line);
        // load body
        for (; std::getline(in2, line);) {
            katahiromz::chomp(line);
            std::vector<std::string> fields;
            katahiromz::split_by_char(fields, line, '\t');

            if (fields.size() < 13) {
                ErrorInfo()->add_error("File '" + fname + "' is invalid.");
                return false;
            }

            //CR_StructID struct_id = std::stol(fields[0], NULL, 0);
            std::string name = fields[1];
            CR_TypeID type_id = std::stol(fields[2], NULL, 0);
            CR_TypeFlags flags = std::stoul(fields[3], NULL, 0);
            bool is_struct = !!std::stol(fields[4], NULL, 0);
            //int size = std::stol(fields[5], NULL, 0);
            int count = std::stol(fields[6], NULL, 0);
            int pack = std::stol(fields[7], NULL, 0);
            int align = std::stol(fields[8], NULL, 0);
            int alignas_ = std::stol(fields[9], NULL, 0);
            bool alignas_explicit = !!std::stol(fields[10], NULL, 0);
            //std::string file = fields[11];
            //int lineno = std::stol(fields[12], NULL, 0);

            if (int(fields.size()) != 13 + 4 * count) {
                ErrorInfo()->add_error("File '" + fname + "' is invalid.");
                return false;
            }

            CR_LogStruct ls;
            ls.m_tid = type_id;
            ls.m_is_struct = is_struct;
            ls.m_pack = pack;
            ls.m_align = align;
            ls.m_alignas = alignas_;
            ls.m_alignas_explicit = alignas_explicit;
            ls.m_is_complete = !(flags & TF_INCOMPLETE);

            for (int i = 0; i < count; ++i) {
                int j = 13 + 4 * i;
                auto& name = fields[j + 0];
                auto tid = std::stol(fields[j + 1], NULL, 0);
                auto bit_offset = std::stol(fields[j + 2], NULL, 0);
                auto bits = std::stol(fields[j + 3], NULL, 0);
                ls.m_members.emplace_back(tid, name, bit_offset, bits);
            }
            m_structs.emplace_back(ls);
        }
    } else {
        ErrorInfo()->add_error("Cannot load file '" + fname + "'");
        return false;
    }

    fname = prefix + "enums" + suffix;
    std::ifstream in3(fname);
    if (in3) {
        // version check
        std::getline(in3, line);
        int version = std::stoi(line);
        if (version != cr_data_version) {
            ErrorInfo()->add_error("File '" + fname +
                "' has different format version. Failed.");
            return false;
        }
        // skip header
        std::getline(in3, line);
        // load body
        for (; std::getline(in3, line);) {
            katahiromz::chomp(line);
            std::vector<std::string> fields;
            katahiromz::split_by_char(fields, line, '\t');

            if (fields.size() < 2) {
                ErrorInfo()->add_error("File '" + fname + "' is invalid.");
                return false;
            }

            //CR_EnumID eid = std::stol(fields[0], NULL, 0);
            int num_items = std::stol(fields[1], NULL, 0);

            if (int(fields.size()) != 2 + num_items * 2) {
                ErrorInfo()->add_error("File '" + fname + "' is invalid.");
                return false;
            }

            CR_LogEnum le;
            for (int i = 0; i < num_items; ++i) {
                int j = 2 + i * 2;
                std::string name = fields[j + 0];
                int value = std::stol(fields[j + 1], NULL, 0);
                le.m_mNameToValue[name] = value;
                le.m_mValueToName[value] = name;
            }
            m_enums.emplace_back(le);
        }
    } else {
        ErrorInfo()->add_error("Cannot load file '" + fname + "'");
        return false;
    }

    fname = prefix + "func_types" + suffix;
    std::ifstream in4(fname);
    if (in4) {
        // version check
        std::getline(in4, line);
        int version = std::stoi(line);
        if (version != cr_data_version) {
            ErrorInfo()->add_error("File '" + fname +
                "' has different format version. Failed.");
            return false;
        }
        // skip header
        std::getline(in4, line);
        // load body
        for (; std::getline(in4, line);) {
            katahiromz::chomp(line);
            std::vector<std::string> fields;
            katahiromz::split_by_char(fields, line, '\t');

            if (fields.size() < 5) {
                ErrorInfo()->add_error("File '" + fname + "' is invalid.");
                return false;
            }

            //CR_FuncID fid = std::stol(fields[0], NULL, 0);
            int return_type = std::stol(fields[1], NULL, 0);
            bool ellipsis = !!std::stol(fields[2], NULL, 0);
            int param_count = std::stol(fields[3], NULL, 0);
            int convention = std::stol(fields[4], NULL, 0);

            CR_LogFunc func;
            func.m_ellipsis = ellipsis;
            func.m_return_type = return_type;
            func.m_convention = static_cast<CR_LogFunc::Convention>(convention);

            if (int(fields.size()) != 5 + param_count * 2) {
                ErrorInfo()->add_error("File '" + fname + "' is invalid.");
                return false;
            }

            for (int i = 0; i < param_count; ++i) {
                int j = 5 + i * 2;
                CR_TypeID tid = std::stol(fields[j + 0], NULL, 0);
                std::string name = fields[j + 1];
                func.m_params.emplace_back(tid, name);
            }
            m_funcs.emplace_back(func);
        }
    } else {
        ErrorInfo()->add_error("Cannot load file '" + fname + "'");
        return false;
    }

    fname = prefix + "vars" + suffix;
    std::ifstream in5(fname);
    if (in5) {
        // version check
        std::getline(in5, line);
        int version = std::stoi(line);
        if (version != cr_data_version) {
            ErrorInfo()->add_error("File '" + fname +
                "' has different format version. Failed.");
            return false;
        }
        // skip header
        std::getline(in5, line);
        // load body
        for (; std::getline(in5, line);) {
            katahiromz::chomp(line);
            std::vector<std::string> fields;
            katahiromz::split_by_char(fields, line, '\t');

            if (fields.size() != 10) {
                ErrorInfo()->add_error("File '" + fname + "' is invalid.");
                return false;
            }

            //CR_VarID vid = std::stol(fields[0], NULL, 0);
            std::string name = fields[1];
            int type_id = std::stol(fields[2], NULL, 0);
            std::string text = fields[3];
            std::string extra = fields[4];
            std::string value_type = fields[5];
            bool is_macro = !!std::stol(fields[6], NULL, 0);
            std::string binary = fields[7];
            std::string file = fields[8];
            int lineno = std::stol(fields[9], NULL, 0);

            CR_LogVar var;
            if (text.size() && value_type == "i" && IsIntegralType(type_id)) {
                var.m_typed_value = IConstant(text, extra);
            } else if (text.size() && value_type == "f" && IsFloatingType(type_id)) {
                var.m_typed_value = FConstant(text, extra);
            } else if (text.size() && value_type == "s" && IsStringType(type_id)) {
                var.m_typed_value = SConstant(type_id, text, extra);
            } else if (text.size() && value_type == "S" && IsWStringType(type_id)) {
                var.m_typed_value = SConstant(type_id, text, extra);
            } else if (text.size() && value_type == "p" && IsPointerType(type_id)) {
                var.m_typed_value = PConstant(type_id, text, extra);
            } else if (text.size() && value_type == "c") {
                var.m_typed_value.m_text = text;
            }
            if (binary.size()) {
                binary = CrParseBinary(binary);
                var.m_typed_value.assign(binary.data(), binary.size());
            }
            var.m_typed_value.m_type_id = type_id;
            var.m_typed_value.m_extra = extra;
            var.m_location.set(file, lineno);
            var.m_is_macro = is_macro;

            auto vid = m_vars.insert(var);
            m_mVarIDToName.emplace(vid, name);
            if (name.size()) {
                m_mNameToVarID.emplace(name, vid);
            }
        }
    } else {
        ErrorInfo()->add_error("Cannot load file '" + fname + "'");
        return false;
    }

    fname = prefix + "name2name" + suffix;
    std::ifstream in6(fname);
    if (in6) {
        // version check
        std::getline(in6, line);
        int version = std::stoi(line);
        if (version != cr_data_version) {
            ErrorInfo()->add_error("File '" + fname +
                "' has different format version. Failed.");
            return false;
        }
        // skip header
        std::getline(in6, line);
        // load body
        for (; std::getline(in6, line);) {
            katahiromz::chomp(line);
            std::vector<std::string> fields;
            katahiromz::split_by_char(fields, line, '\t');

            if (fields.size() != 4) {
                ErrorInfo()->add_error("File '" + fname + "' is invalid.");
                return false;
            }

            std::string name1 = fields[0];
            std::string name2 = fields[1];
            std::string file = fields[2];
            int line = std::stol(fields[3], NULL, 0);

            CR_Name2Name name2name;
            name2name.m_from = name1;
            name2name.m_to = name2;
            name2name.m_location = CR_Location(file, line);

            m_mNameToName.emplace(name1, name2name);
        }
    } else {
        ErrorInfo()->add_error("Cannot load file '" + fname + "'");
        return false;
    }

    return true;
}

bool CR_NameScope::LoadMacros(
    const std::string& prefix/* = ""*/,
    const std::string& suffix/* = ".dat"*/)
{
    m_macros.clear();

    std::string fname = prefix + "macros" + suffix;
    std::ifstream in1(fname);
    if (in1) {
        std::string line;
        for (; std::getline(in1, line); ) {
            katahiromz::chomp(line);

            if (memcmp(line.c_str(), "#define ", 8) != 0)
                continue;

            line = line.substr(8);
            size_t n;
            std::string name;
            for (n = 0; n < line.size(); ++n) {
                char ch = line[n];
                if (name.empty()) {
                    if (!std::isalpha(ch) && ch != '_')
                        break;
                } else {
                    if (!std::isalnum(ch) && ch != '_')
                        break;
                }
                name += ch;
            }
            line = line.substr(n);

            std::string paralist;
            bool is_func = line.c_str()[0] == '(';
            if (is_func) {
                n = line.find(')');
                paralist = line.substr(1, n - 1);

                line = line.substr(n + 1);
                katahiromz::trim(line);
            }

            std::vector<std::string> params;
            katahiromz::split_by_char(params, paralist, '\t');

            bool ellipsis = false;
            for (auto& para : params) {
                katahiromz::trim(para);
                if (para == "...") {
                    ellipsis = true;
                    params.resize(params.size() - 1);
                    break;
                }
            }

            CR_Macro macro;
            if (is_func)
                macro.m_num_params = int(params.size());
            else
                macro.m_num_params = -1;

            macro.m_contents = line;
            macro.m_ellipsis = ellipsis;

            m_macros.emplace(name, macro);
        }
        return true;
    } else {
        ErrorInfo()->add_error("Cannot load file '" + fname + "'");
        return false;
    }

    return false;
}

void CR_NameScope::FixupLogFuncs(void) {
    for (auto& type : LogTypes()) {
        if (type.m_flags & TF_FUNCTION) {
            auto fid = type.m_sub_id;
            auto& func = LogFuncs()[fid];
            if ((type.m_flags & TF_STDCALL) == TF_STDCALL) {
                func.m_convention = CR_LogFunc::LFC_STDCALL;
            } else if ((type.m_flags & TF_FASTCALL) == TF_FASTCALL) {
                func.m_convention = CR_LogFunc::LFC_FASTCALL;
            } else {
                func.m_convention = CR_LogFunc::LFC_CDECL;
            }
        }
    }
}

bool CR_NameScope::SaveToFiles(
    const std::string& prefix/* = ""*/,
    const std::string& suffix/* = ".dat"*/) const
{
    std::string fname;

    fname = prefix + "types" + suffix;
    std::ofstream out1(fname);
    if (out1) {
        out1 << cr_data_version << std::endl;
        out1 << "(type_id)\t(name)\t(flags)\t(sub_id)\t(count)\t(size)\t(align)\t(alignas)\t(alignas_explicit)\t(is_macro)\t(file)\t(line)" <<
            std::endl;
        for (CR_TypeID tid = 0; tid < LogTypes().size(); ++tid) {
            auto& type = LogType(tid);
            auto& location = type.m_location;
            std::string name;
            auto it = MapTypeIDToName().find(tid);
            if (it != MapTypeIDToName().end()) {
                name = it->second;
            }

            std::string file = location.m_file;
            int lineno = location.m_line;
            if (IsPredefinedType(tid)) {
                file = "(predefined)";
                lineno = 0;
            }
            for (auto& ch : file) {
                if (ch == '\\') {
                    ch = '/';
                }
            }

            out1 <<
                tid << "\t" <<
                name << "\t0x" <<
                std::hex << type.m_flags << std::dec << "\t" <<
                type.m_sub_id << "\t" <<
                type.m_count << "\t" <<
                type.m_size << "\t" <<
                type.m_align << "\t" <<
                type.m_alignas << "\t" <<
                type.m_alignas_explicit << "\t" <<
                type.m_is_macro << "\t" <<
                file << "\t" <<
                lineno << std::endl;
        }
    } else {
        ErrorInfo()->add_error("Cannot write file '" + fname + "'");
        return false;
    }

    fname = prefix + "structures" + suffix;
    std::ofstream out2(fname);
    if (out2) {
        out2 << cr_data_version << std::endl;
        out2 << "(struct_id)\t(name)\t(type_id)\t(flags)\t(is_struct)\t(size)\t(count)\t(pack)\t(align)\t(alignas)\t(alignas_explicit)\t(file)\t(line)\t(item_1_name)\t(item_1_type_id)\t(item_1_bit_offset)\t(item_1_bits)\t(item_2_type_id)\t..." <<
            std::endl;
        for (CR_TypeID sid = 0; sid < LogStructs().size(); ++sid) {
            auto& ls = LogStruct(sid);
            auto tid = ls.m_tid;
            auto& type = LogType(tid);
            auto& location = type.m_location;
            std::string name;
            auto it = MapTypeIDToName().find(tid);
            if (it != MapTypeIDToName().end()) {
                name = it->second;
            }

            std::string file = location.m_file;
            for (auto& ch : file) {
                if (ch == '\\') {
                    ch = '/';
                }
            }

            out2 <<
                sid << "\t" <<
                name << "\t" <<
                tid << "\t0x" <<
                std::hex << type.m_flags << std::dec << "\t" <<
                ls.m_is_struct << "\t" <<
                type.m_size << "\t" <<
                ls.m_members.size() << "\t" <<
                ls.m_pack << "\t" <<
                ls.m_align << "\t" <<
                ls.m_alignas << "\t" <<
                ls.m_alignas_explicit << "\t" <<
                file << "\t" <<
                location.m_line;

            const size_t siz = ls.m_members.size();
            for (size_t i = 0; i < siz; ++i) {
                out2 << "\t" <<
                    ls.m_members[i].m_name << "\t" <<
                    ls.m_members[i].m_type_id << "\t" <<
                    ls.m_members[i].m_bit_offset << "\t" <<
                    ls.m_members[i].m_bits;
            }
            out2 << std::endl;
        }
    } else {
        ErrorInfo()->add_error("Cannot write file '" + fname + "'");
        return false;
    }

    fname = prefix + "enums" + suffix;
    std::ofstream out3(fname);
    if (out3) {
        out3 << cr_data_version << std::endl;
        out3 << "(enum_id)\t(num_items)\t(item_name_1)\t(item_value_1)\t(item_name_2)\t..." <<
            std::endl;
        for (CR_EnumID eid = 0; eid < LogEnums().size(); ++eid) {
            auto& le = LogEnum(eid);
            size_t num_items = le.m_mNameToValue.size();

            out3 <<
                eid << "\t" <<
                num_items;
            for (auto& item : le.m_mNameToValue) {
                out3 << "\t" <<
                    item.first << "\t" <<
                    item.second;
            }
            out3 << std::endl;
        }
    } else {
        ErrorInfo()->add_error("Cannot write file '" + fname + "'");
        return false;
    }

    fname = prefix + "func_types" + suffix;
    std::ofstream out4(fname);
    if (out4) {
        out4 << cr_data_version << std::endl;
        out4 << "(func_id)\t(return_type)\t(ellipsis)\t(param_count)\t(convention)\t(param_1_typeid)\t(param_1_name)\t(param_2_typeid)\t..." <<
            std::endl;
        for (size_t fid = 0; fid < LogFuncs().size(); ++fid) {
            auto& func = LogFunc(fid);

            out4 <<
                fid << "\t" <<
                func.m_return_type << "\t" <<
                func.m_ellipsis << "\t" <<
                func.m_params.size() << "\t" <<
                func.m_convention;
            const size_t siz = func.m_params.size();
            for (size_t j = 0; j < siz; ++j) {
                out4 << "\t" <<
                    func.m_params[j].m_type_id << "\t" <<
                    func.m_params[j].m_name;
            }
            out4 << std::endl;
        }
    } else {
        ErrorInfo()->add_error("Cannot write file '" + fname + "'");
        return false;
    }

    fname = prefix + "vars" + suffix;
    std::ofstream out5(fname);
    if (out5) {
        out5 << cr_data_version << std::endl;
        out5 << "(var_id)\t(name)\t(type_id)\t(text)\t(extra)\t(value_type)\t(is_macro)\t(binary)\t(file)\t(line)" <<
            std::endl;
        for (size_t vid = 0; vid < LogVars().size(); ++vid) {
            auto& var = LogVar(vid);
            std::string name;
            auto it = MapVarIDToName().find(vid);
            if (it != MapVarIDToName().end()) {
                name = it->second;
            } else {
                continue;
            }
            auto& location = var.m_location;

            std::string text = var.m_typed_value.m_text;

            std::string value_type, binary;
            auto& typed_value = var.m_typed_value;
            auto tid = typed_value.m_type_id;
            auto& type = LogType(tid);
            const char *ptr = reinterpret_cast<const char *>(typed_value.m_ptr);
            if (text.size()) {
                if (IsIntegralType(tid)) {
                    value_type = "i";
                    binary.assign(ptr, typed_value.m_size);
                    binary = CrFormatBinary(binary);
                } else if (IsFloatingType(tid)) {
                    value_type = "f";
                    binary.assign(ptr, typed_value.m_size);
                    binary = CrFormatBinary(binary);
                } else if (IsStringType(tid) && text[0] == '\"') {
                    value_type = "s";
                    binary.assign(ptr, typed_value.m_size);
                    binary = CrFormatBinary(binary);
                } else if (IsWStringType(tid) && text[0] == '\"') {
                    value_type = "S";
                    binary.assign(ptr, typed_value.m_size);
                    binary = CrFormatBinary(binary);
                } else if (text[0] == '{') {
                    value_type = "c";
                    binary.assign(ptr, typed_value.m_size);
                    binary = CrFormatBinary(binary);
                } else if (IsPointerType(tid)) {
                    unsigned long long n = std::stoull(text, NULL, 0);
                    value_type = "p";
                    ptr = reinterpret_cast<const char *>(&n);
                    if (type.m_size == 8) {
                        binary.assign(ptr, 8);
                    } else {
                        binary.assign(ptr, 4);
                    }
                    binary = CrFormatBinary(binary);
                }
            }

            std::string file = location.m_file;
            for (auto& ch : file) {
                if (ch == '\\') {
                    ch = '/';
                }
            }

            out5 <<
                vid << "\t" <<
                name << "\t" <<
                tid << "\t" <<
                text << "\t" <<
                var.m_typed_value.m_extra << "\t" <<
                value_type << "\t" <<
                var.m_is_macro << "\t" <<
                binary << "\t" <<
                file << "\t" <<
                location.m_line << std::endl;
        }
    } else {
        ErrorInfo()->add_error("Cannot write file '" + fname + "'");
        return false;
    }

    fname = prefix + "name2name" + suffix;
    std::ofstream out6(fname);
    if (out6) {
        out6 << cr_data_version << std::endl;
        out6 << "(name1)\t(name2)\t(file)\t(line)" << std::endl;
        for (auto& it : m_mNameToName) {
            out6 <<
                it.second.m_from << "\t" <<
                it.second.m_to << "\t" <<
                it.second.m_location.m_file << "\t" <<
                it.second.m_location.m_line << std::endl;
        }
    } else {
        ErrorInfo()->add_error("Cannot write file '" + fname + "'");
        return false;
    }

    fname = prefix + "functions" + suffix;
    std::ofstream out7(fname);
    if (out7) {
        out7 << "(func_name)\t(convention)\t(return)\t(param_1)\t(param_2)\t...\n";

        CR_VarID vid = 0;
        for (auto& var : m_vars) {
            auto it = m_mVarIDToName.find(vid++);
            if (it == m_mVarIDToName.end())
                continue;
            auto name = it->second;

            if (var.m_is_macro)
                continue;

            auto type_id = var.m_typed_value.m_type_id;
            if (!IsFuncType(type_id))
                continue;

            auto& type = LogType(type_id);

            CR_FuncID fid = type.m_sub_id;
            auto& func = LogFunc(fid);

            auto ret_name = DecoratedTypeName(func.m_return_type);

            std::string convention;
            switch (func.m_convention)
            {
            case CR_LogFunc::LFC_UNKNOWN: convention = "unknown"; break;
            case CR_LogFunc::LFC_CDECL: convention = "__cdecl"; break;
            case CR_LogFunc::LFC_STDCALL: convention = "__stdcall"; break;
            case CR_LogFunc::LFC_FASTCALL: convention = "__fastcall"; break;
            }

            out7 << name << "\t" << convention << "\t" << ret_name;

            for (auto& param : func.m_params) {
                auto param_type = DecoratedTypeName(param.m_type_id);
                out7 << "\t" << param_type << ":" << param.m_name;
            }

            if (func.m_ellipsis)
                out7 << "\t" << "...";

            out7 << std::endl;
        }
    } else {
        ErrorInfo()->add_error("Cannot write file '" + fname + "'");
        return false;
    }

    return true;
}

std::string CR_NameScope::DecoratedTypeName(CR_TypeID tid) const
{
    std::string ret;
    char buf[32];
    auto name = NameFromTypeID(tid);
    if (IsVoidType(tid)) {
        ret += "v:";
    } else if (IsPointerType(tid)) {
        if (IsHandleType(tid)) {
            ret += "h:";
        } else {
            auto base = ResolvePointer(tid);
            auto size = SizeOfType(base) * 8;
            std::sprintf(buf, "p%u:", (int)size);
            ret += buf;
        }
    } else if (IsIntegralType(tid) || IsEnumType(tid)) {
        if (IsUnsignedType(tid)) {
            auto size = SizeOfType(tid) * 8;
            std::sprintf(buf, "u%u:", (int)size);
            ret += buf;
        } else {
            auto size = SizeOfType(tid) * 8;
            std::sprintf(buf, "i%u:", (int)size);
            ret += buf;
        }
    } else if (IsFloatingType(tid)) {
        auto size = SizeOfType(tid) * 8;
        std::sprintf(buf, "f%u:", (int)size);
        ret += buf;
    } else {
        auto size = SizeOfType(tid) * 8;
        std::sprintf(buf, "r%u:", (int)size);
        ret += buf;
    }
    ret += name;
    katahiromz::trim(ret);
    return ret;
}

bool CR_NameScope::IsEnumType(CR_TypeID tid) const
{
    while (tid != cr_invalid_id) {
        tid = ResolveAlias(tid);
        if (tid == cr_invalid_id) {
            break;
        }
        auto& type = LogType(tid);
        if (type.m_flags & TF_ENUM)
            return true;
        if ((type.m_flags & (TF_CONST | TF_POINTER)) == TF_CONST) {
            tid = type.m_sub_id;
        } else {
            break;
        }
    }
    return false;
}

bool CR_NameScope::IsVoidType(CR_TypeID tid) const
{
    while (tid != cr_invalid_id) {
        tid = ResolveAlias(tid);
        if (tid == cr_invalid_id) {
            break;
        }
        auto& type = LogType(tid);
        if (type.m_flags & TF_VOID)
            return true;
        if ((type.m_flags & (TF_CONST | TF_POINTER)) == TF_CONST) {
            tid = type.m_sub_id;
        } else {
            break;
        }
    }
    return false;
}

bool CR_NameScope::IsHandleType(CR_TypeID tid) const
{
    if (!IsPointerType(tid))
        return false;
    tid = ResolvePointer(tid);
    tid = ResolveAlias(tid);
    if (tid == cr_invalid_id)
        return false;
    auto& type = LogType(tid);
    if (!(type.m_flags & TF_STRUCT))
        return false;
    auto& st = LogStruct(type.m_sub_id);
    if (st.m_members.size() != 1)
        return false;

    return st.m_members[0].m_name == "unused";
}

////////////////////////////////////////////////////////////////////////////
