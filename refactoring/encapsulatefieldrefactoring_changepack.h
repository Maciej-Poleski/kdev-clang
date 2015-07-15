/*
    This file is part of KDevelop

    Copyright 2015 Maciej Poleski <d82ks8djf82msd83hf8sc02lqb5gh5@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef KDEV_CLANG_ENCAPSULATEFIELDREFACTORING_CHANGEPACK_H
#define KDEV_CLANG_ENCAPSULATEFIELDREFACTORING_CHANGEPACK_H

// C++
#include <string>
#include <memory>

// Clang
#include <clang/AST/DeclBase.h>
#include <clang/Basic/Specifiers.h>

#include "encapsulatefieldrefactoring.h"

class EncapsulateFieldRefactoring::ChangePack
{
public:
    enum class AccessorStyle : unsigned char
    {
        ConstReference,
        Value,
    };

    ChangePack(const std::string &fieldDescription, const std::string &fieldType,
               const std::string &getterName, const std::string &setterName,
               clang::AccessSpecifier getterAccess, clang::AccessSpecifier setterAccess,
               AccessorStyle accessorStyle, bool createSetter);

    static std::unique_ptr<ChangePack> fromDeclaratorDecl(const clang::DeclaratorDecl *decl);


    const std::string &fieldDescription() const
    {
        return m_fieldDescription;
    }

    const std::string &fieldType() const
    {
        return m_fieldType;
    }

    const std::string &getterName() const
    {
        return m_getterName;
    }

    void setGetterName(const std::string &getterName)
    {
        m_getterName = getterName;
    }

    const std::string &setterName() const
    {
        return m_setterName;
    }

    void setSetterName(const std::string &setterName)
    {
        m_setterName = setterName;
    }

    const clang::AccessSpecifier &getterAccess() const
    {
        return m_getterAccess;
    }

    void setGetterAccess(const clang::AccessSpecifier &getterAccess)
    {
        m_getterAccess = getterAccess;
    }

    const clang::AccessSpecifier &setterAccess() const
    {
        return m_setterAccess;
    }

    void setSetterAccess(const clang::AccessSpecifier &setterAccess)
    {
        m_setterAccess = setterAccess;
    }

    const AccessorStyle &accessorStyle() const
    {
        return m_accessorStyle;
    }

    void setAccessorStyle(const AccessorStyle &accessorStyle)
    {
        m_accessorStyle = accessorStyle;
    }

    bool createSetter() const
    {
        return m_createSetter;
    }

    void setCreateSetter(bool createSetter)
    {
        m_createSetter = createSetter;
    }

private:
    const std::string m_fieldDescription;
    const std::string m_fieldType;
    std::string m_getterName;
    std::string m_setterName;
    clang::AccessSpecifier m_getterAccess;
    clang::AccessSpecifier m_setterAccess;
    AccessorStyle m_accessorStyle;
    bool m_createSetter;
};


#endif //KDEV_CLANG_ENCAPSULATEFIELDREFACTORING_CHANGEPACK_H
