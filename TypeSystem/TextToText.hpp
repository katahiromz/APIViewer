////////////////////////////////////////////////////////////////////////////
// TextToText.hpp -- a text converting utility for Win32
// This file is part of MZC3.  See file "ReadMe.txt" and "License.txt".
////////////////////////////////////////////////////////////////////////////

#ifndef __MZC3_TEXTTOTEXT__
#define __MZC3_TEXTTOTEXT__

#ifndef _INC_WINDOWS
	#include <Windows.h>
#endif

///////////////////////////////////////////////////////////////////////////////
// MAnsiToWide

class MAnsiToWide
{
public:
    MAnsiToWide();
    MAnsiToWide(const char *ansi);
    MAnsiToWide(const char *ansi, size_t count);
    MAnsiToWide(const MAnsiToWide& a2w);
    MAnsiToWide& operator=(const MAnsiToWide& a2w);
    #ifdef MODERN_CXX
        MAnsiToWide(MAnsiToWide&& a2w);
        MAnsiToWide& operator=(MAnsiToWide&& a2w);
    #endif
    virtual ~MAnsiToWide();

    bool empty() const;
    size_t size() const;
    const wchar_t *data() const;
    const wchar_t *c_str() const;
    operator const wchar_t *() const;

protected:
    wchar_t *m_wide;
    size_t m_size;
};

///////////////////////////////////////////////////////////////////////////////
// MWideToAnsi

class MWideToAnsi
{
public:
    MWideToAnsi();
    MWideToAnsi(const wchar_t *wide);
    MWideToAnsi(const wchar_t *wide, size_t count);
    MWideToAnsi(const MWideToAnsi& w2a);
    MWideToAnsi& operator=(const MWideToAnsi& w2a);
    #ifdef MODERN_CXX
        MWideToAnsi(MWideToAnsi&& w2a);
        MWideToAnsi& operator=(MWideToAnsi&& w2a);
    #endif
    virtual ~MWideToAnsi();

    bool empty() const;
    size_t size() const;
    const char *data() const;
    const char *c_str() const;
    operator const char *() const;

protected:
    char *m_ansi;
    size_t m_size;
};

///////////////////////////////////////////////////////////////////////////////
// MUtf8ToWide

class MUtf8ToWide
{
public:
    MUtf8ToWide();
    MUtf8ToWide(const char *utf8);
    MUtf8ToWide(const MUtf8ToWide& u2w);
    MUtf8ToWide& operator=(const MUtf8ToWide& u2w);
    #ifdef MODERN_CXX
        MUtf8ToWide(MUtf8ToWide&& u2w);
        MUtf8ToWide& operator=(MUtf8ToWide&& u2w);
    #endif
    virtual ~MUtf8ToWide();

    bool empty() const;
    size_t size() const;
    const wchar_t *data() const;
    const wchar_t *c_str() const;
    operator const wchar_t *() const;

protected:
    wchar_t *m_wide;
};

///////////////////////////////////////////////////////////////////////////////
// MWideToUtf8

class MWideToUtf8
{
public:
    MWideToUtf8();
    MWideToUtf8(const wchar_t *wide);
    MWideToUtf8(const MWideToUtf8& w2u);
    MWideToUtf8& operator=(const MWideToUtf8& w2u);
    #ifdef MODERN_CXX
        MWideToUtf8(MWideToUtf8&& w2u);
        MWideToUtf8& operator=(MWideToUtf8&& w2u);
    #endif
    virtual ~MWideToUtf8();

    bool empty() const;
    size_t size() const;
    const char *data() const;
    const char *c_str() const;
    operator const char *() const;

protected:
    char *m_utf8;
};

///////////////////////////////////////////////////////////////////////////////

#define MAnsiToAnsi(ansi) (ansi)
#define MWideToWide(wide) (wide)
#define MUtf8ToUtf8(utf8) (utf8)

#define MAnsiToUtf8(ansi) MWideToUtf8(MAnsiToWide(ansi))
#define MUtf8ToAnsi(utf8) MWideToAnsi(MUtf8ToWide(utf8))

///////////////////////////////////////////////////////////////////////////////

#ifdef UNICODE
    #define MAnsiToText MAnsiToWide
    #define MTextToAnsi MWideToAnsi
    #define MWideToText MWideToWide
    #define MTextToWide MWideToWide
    #define MUtf8ToText MUtf8ToWide
    #define MTextToUtf8 MWideToUtf8
    #define MTextToText MWideToWide
#else
    #define MAnsiToText MAnsiToAnsi
    #define MTextToAnsi MAnsiToAnsi
    #define MWideToText MWideToAnsi
    #define MTextToWide MAnsiToWide
    #define MUtf8ToText MUtf8ToAnsi
    #define MTextToUtf8 MAnsiToUtf8
    #define MTextToText MAnsiToAnsi
#endif

///////////////////////////////////////////////////////////////////////////////

#ifndef MZC_NO_INLINES
    #undef MZC_INLINE
    #define MZC_INLINE inline
    #include "TextToText_inl.hpp"
#endif

///////////////////////////////////////////////////////////////////////////////

#endif  // ndef __MZC3_TEXTTOTEXT__
