////////////////////////////////////////////////////////////////////////////
// Main.h
// Copyright (C) 2014-2015 Katayama Hirofumi MZ.  All rights reserved.
////////////////////////////////////////////////////////////////////////////
// This file is part of CodeReverse. See file ReadMe.txt and License.txt.
////////////////////////////////////////////////////////////////////////////

#ifndef MAIN_H_
#define MAIN_H_

#include <vector>
#include <iostream>
#include <cassert>
#include "Location.h"

////////////////////////////////////////////////////////////////////////////
// CR_VecSet<ITEM_T> -- vector and set

template <typename ITEM_T>
class CR_VecSet : public std::vector<ITEM_T>
{
public:
    CR_VecSet() { }

    CR_VecSet(const CR_VecSet<ITEM_T>& vs) : std::vector<ITEM_T>(vs)
    { }

    CR_VecSet<ITEM_T>& operator=(const CR_VecSet<ITEM_T>& vs) {
        this->assign(vs.begin(), vs.end());
        return *this;
    }

    virtual ~CR_VecSet() { }

    std::size_t insert(const ITEM_T& item) {
        this->push_back(item);
        return this->size() - 1;
    }

    bool Contains(const ITEM_T& item) const {
        const std::size_t siz = this->size();
        for (std::size_t i = 0; i < siz; i++) {
            if (this->at(i) == item)
                return true;
        }
        return false;
    }

    std::size_t Find(const ITEM_T& item) const {
        const std::size_t siz = this->size();
        for (std::size_t i = 0; i < siz; i++) {
            if (this->at(i) == item)
                return i;
        }
        return static_cast<std::size_t>(-1);
    }

    std::size_t AddUnique(const ITEM_T& item) {
        const std::size_t siz = this->size();
        for (std::size_t i = 0; i < siz; i++) {
            if (this->at(i) == item)
                return i;
        }
        this->push_back(item);
        return this->size() - 1;
    }

    void AddHead(const CR_VecSet<ITEM_T>& items) {
        std::vector<ITEM_T>::insert(
            std::vector<ITEM_T>::begin(), items.begin(), items.end());
    }

    void AddTail(const CR_VecSet<ITEM_T>& items) {
        std::vector<ITEM_T>::insert(
            std::vector<ITEM_T>::end(), items.begin(), items.end());
    }

    std::size_t count(const ITEM_T& item) const {
        std::size_t count = 0;
        for (std::size_t i : *this) {
            if (this->at(i) == item)
                count++;
        }
        return count;
    }

    void sort() {
        std::sort(this->begin(), this->end());
    }

    void unique() {
        std::unique(this->begin(), this->end());
    }

    void erase(const ITEM_T& item) {
        std::size_t i, j;
        const std::size_t count = this->size();
        for (i = j = 0; i < count; i++) {
            if (this->at(i) != item) {
                this->at(j++) = this->at(i);
            }
        }
        if (i != j)
            this->resize(j);
    }
}; // class CR_VecSet<ITEM_T>

namespace std
{
    template <typename ITEM_T>
    inline void swap(CR_VecSet<ITEM_T>& vs1, CR_VecSet<ITEM_T>& vs2) {
        vs1.swap(vs2);
    }
}

////////////////////////////////////////////////////////////////////////////
// CR_StringSet

typedef CR_VecSet<std::string> CR_StringSet;

////////////////////////////////////////////////////////////////////////////
// CR_ErrorInfo

class CR_ErrorInfo {
public:
    typedef CR_StringSet error_container;
    enum Type {
        NOTHING = 0, NOTICE, WARN, ERR
    };

public:
    CR_ErrorInfo() { }

    void add_message(Type type, const CR_Location& location,
                     const std::string str)
    {
        switch (type) {
        case NOTICE:    add_notice(location, str); break;
        case WARN:      add_warning(location, str); break;
        case ERR:       add_error(location, str); break;
        default: break;
        }
    }

    void add_notice(const CR_Location& location, const std::string str) {
        m_notices.emplace_back(location.str() + ": " + str);
    }

    void add_warning(const CR_Location& location, const std::string str) {
        m_warnings.emplace_back(location.str() + ": WARNING: " + str);
    }

    void add_error(const CR_Location& location, const std::string str) {
        m_errors.emplace_back(location.str() + ": ERROR: " + str);
    }

    void add_notice(const std::string str) {
        m_notices.emplace_back(str);
    }

    void add_warning(const std::string str) {
        m_warnings.emplace_back("WARNING: " + str);
    }

    void add_error(const std::string str) {
        m_errors.emplace_back("ERROR: " + str);
    }

          error_container& notices()        { return m_notices; }
    const error_container& notices() const  { return m_notices; }
          error_container& warnings()       { return m_warnings; }
    const error_container& warnings() const { return m_warnings; }
          error_container& errors()         { return m_errors; }
    const error_container& errors() const   { return m_errors; }

    void emit_all(std::ostream& out = std::cerr) {
        for (auto& e : errors()) {
            out << e << std::endl;
        }
        for (auto& w : warnings()) {
            out << w << std::endl;
        }
        for (auto& n : notices()) {
            out << n << std::endl;
        }
    }

    void clear_notices()    { m_notices.clear(); }
    void clear_warnings()   { m_warnings.clear(); }
    void clear_errors()     { m_errors.clear(); }

    void clear() {
        m_notices.clear();
        m_warnings.clear();
        m_errors.clear();
    }

protected:
    error_container m_notices;
    error_container m_warnings;
    error_container m_errors;
};

////////////////////////////////////////////////////////////////////////////

#endif  // ndef MAIN_H_
