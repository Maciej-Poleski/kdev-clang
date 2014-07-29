/*
 * Copyright 2014  Kevin Funk <kfunk@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CLANGDUCONTEXT_H
#define CLANGDUCONTEXT_H

#include <language/duchain/ducontext.h>
#include <language/duchain/topducontext.h>

template<class BaseContext, int IdentityT>
class ClangDUContext : public BaseContext
{
public:
    template<class Data>
    ClangDUContext(Data& data) : BaseContext(data) {
    }

    ///Parameters will be reached to the base-class
    template<class Param1, class Param2>
    ClangDUContext(const Param1& p1, const Param2& p2, bool isInstantiationContext) : BaseContext(p1, p2, isInstantiationContext) {
        static_cast<KDevelop::DUChainBase*>(this)->d_func_dynamic()->setClassId(this);
    }

    ///Both parameters will be reached to the base-class. This fits TopDUContext.
    template<class Param1, class Param2, class Param3>
    ClangDUContext(const Param1& p1, const Param2& p2, const Param3& p3) : BaseContext(p1, p2, p3) {
        static_cast<KDevelop::DUChainBase*>(this)->d_func_dynamic()->setClassId(this);
    }
    template<class Param1, class Param2>
    ClangDUContext(const Param1& p1, const Param2& p2) : BaseContext(p1, p2) {
        static_cast<KDevelop::DUChainBase*>(this)->d_func_dynamic()->setClassId(this);
    }

    virtual QWidget* createNavigationWidget(KDevelop::Declaration* decl = 0, KDevelop::TopDUContext* topContext = 0,
                                            const QString& htmlPrefix = QString(), const QString& htmlSuffix = QString()) const override;

    enum {
        Identity = IdentityT
    };
};

typedef ClangDUContext<KDevelop::TopDUContext, 140> ClangTopDUContext;
typedef ClangDUContext<KDevelop::DUContext, 141> ClangNormalDUContext;

#endif // CLANGDUCONTEXT_H