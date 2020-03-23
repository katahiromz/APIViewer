////////////////////////////////////////////////////////////////////////////
// Location.h
// Copyright (C) 2014-2015 Katayama Hirofumi MZ.  All rights reserved.
////////////////////////////////////////////////////////////////////////////
// This file is part of CodeReverse. See file ReadMe.txt and License.txt.
////////////////////////////////////////////////////////////////////////////

#ifndef LOCATION_H_
#define LOCATION_H_

#include <string>

//
// CR_Location
//
struct CR_Location
{
    std::string m_file;
    int m_line;

    CR_Location() : m_file(), m_line(1) { }

    CR_Location(const std::string& file, int line) :
        m_file(file), m_line(line) { }

    void set(const std::string& file, int line) {
        m_file = file;
        m_line = line;
    }

    CR_Location& operator++() {
        m_line++;
        return *this;
    }

    CR_Location operator++(int) {
        CR_Location loc(*this);
        m_line++;
        return loc;
    }

    std::string str() const {
        std::string s = m_file;
        s += " (";
        s += std::to_string(m_line);
        s += ')';
        return s;
    }
};

#endif  // def LOCATION_H_
