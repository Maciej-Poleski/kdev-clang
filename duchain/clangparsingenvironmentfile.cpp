/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright 2014  Milian Wolff <mail@milianw.de>
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
 *
 */

#include "clangparsingenvironmentfile.h"

#include <language/duchain/duchainregister.h>

#include "parsesession.h"
#include "clangparsingenvironment.h"

using namespace KDevelop;

REGISTER_DUCHAIN_ITEM(ClangParsingEnvironmentFile);

struct ClangParsingEnvironmentFileData : public ParsingEnvironmentFileData
{
    ClangParsingEnvironmentFileData()
        : ParsingEnvironmentFileData()
        , environmentHash(0)
        , projectWasKnown(false)
    {
    }

    ClangParsingEnvironmentFileData(const ClangParsingEnvironmentFileData& rhs)
        : ParsingEnvironmentFileData(rhs)
        , environmentHash(rhs.environmentHash)
        , projectWasKnown(rhs.projectWasKnown)
    {
    }

    ~ClangParsingEnvironmentFileData() = default;

    uint environmentHash;
    bool projectWasKnown;
};

ClangParsingEnvironmentFile::ClangParsingEnvironmentFile(const IndexedString& url, const ClangParsingEnvironment& environment)
    : ParsingEnvironmentFile(*(new ClangParsingEnvironmentFileData), url)
{
  d_func_dynamic()->setClassId(this);
  setEnvironment(environment);
  setLanguage(ParseSession::languageString());
}

ClangParsingEnvironmentFile::ClangParsingEnvironmentFile(ClangParsingEnvironmentFileData& data)
    : ParsingEnvironmentFile(data)
{
}

ClangParsingEnvironmentFile::~ClangParsingEnvironmentFile() = default;

int ClangParsingEnvironmentFile::type() const
{
    return CppParsingEnvironment;
}

bool ClangParsingEnvironmentFile::needsUpdate(const ParsingEnvironment* environment) const
{
    if (environment) {
        Q_ASSERT(dynamic_cast<const ClangParsingEnvironment*>(environment));
        auto env = static_cast<const ClangParsingEnvironment*>(environment);
        if ((env->projectKnown() || env->projectKnown() == d_func()->projectWasKnown) && env->hash() != d_func()->environmentHash) {
            if (url().str().contains("vector3d.h")) {
                qDebug() << "environment differs, require update:" << url() << env->projectKnown() << d_func()->projectWasKnown << env->hash() << d_func()->environmentHash;
            }
            return true;
        } else {
            if (url().str().contains("vector3d.h")) {
                qDebug() << "environment matches:" << url() << env->projectKnown() << d_func()->projectWasKnown << env->hash() << d_func()->environmentHash;
            }
        }
    }
    bool ret = KDevelop::ParsingEnvironmentFile::needsUpdate(environment);
    if (url().str().contains("vector3d.h")) {
        qDebug() << "FALLBACK:" << url() << ret;
    }
    return ret;
}

void ClangParsingEnvironmentFile::setEnvironment(const ClangParsingEnvironment& environment)
{
    d_func_dynamic()->environmentHash = environment.hash();
    d_func_dynamic()->projectWasKnown = environment.projectKnown();
}