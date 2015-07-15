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

// Qt
#include <QPushButton>

#include "encapsulatefielddialog.h"
#include "encapsulatefieldrefactoring_changepack.h"

using namespace clang;

EncapsulateFieldDialog::EncapsulateFieldDialog(ChangePack *changePack)
    : QDialog(nullptr)
      , m_changePack(changePack)
{
    setupUi(this);
    fieldLineEdit->setText(QString::fromStdString(m_changePack->fieldDescription()));
    constRefRadioButton->setText(
        QStringLiteral("const ") + QString::fromStdString(m_changePack->fieldType()) +
        QStringLiteral("&&"));
    valueRadioButton->setText(QString::fromStdString(m_changePack->fieldType()));
    switch (m_changePack->accessorStyle()) {
    case ChangePack::AccessorStyle::ConstReference:
        constRefRadioButton->setChecked(true);
        break;
    case ChangePack::AccessorStyle::Value:
        constRefRadioButton->setChecked(true);
    }
    setterGroupBox->setEnabled(m_changePack->createSetter());
    getterNameLineEdit->setText(QString::fromStdString(m_changePack->getterName()));
    setterNameLineEdit->setText(QString::fromStdString(m_changePack->setterName()));

    Q_ASSERT(m_changePack->getterAccess() != AS_none);
    Q_ASSERT(m_changePack->getterAccess() == m_changePack->setterAccess());
    // It's property of initialization code, used below
    switch (m_changePack->getterAccess()) {
    case AccessSpecifier::AS_public:
        getterProtectedRadioButton->setDisabled(true);
        setterProtectedRadioButton->setDisabled(true);
        // fallthrough
    case AccessSpecifier::AS_protected:
        getterPrivateRadioButton->setDisabled(true);
        setterPrivateRadioButton->setDisabled(true);
        // fallthrough
    case AccessSpecifier::AS_private:
        break;
    default:
        Q_ASSERT(false);
    }
    switch (m_changePack->getterAccess()) {
    case AccessSpecifier::AS_public:
        getterPublicRadioButton->setChecked(true);
        setterPublicRadioButton->setChecked(true);
        break;
    case AccessSpecifier::AS_protected:
        getterProtectedRadioButton->setChecked(true);
        setterProtectedRadioButton->setChecked(true);
        break;
    case AccessSpecifier::AS_private:
        getterPrivateRadioButton->setChecked(true);
        setterPrivateRadioButton->setChecked(true);
        break;
    default:
        Q_ASSERT(false);
    } // or set to public instead...

    connect(buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, [this]()
    {
        ChangePack::AccessorStyle style;
        if (constRefRadioButton->isChecked()) {
            style = ChangePack::AccessorStyle::ConstReference;
        } else {
            Q_ASSERT(valueRadioButton->isChecked());
            style = ChangePack::AccessorStyle::Value;
        }
        m_changePack->setAccessorStyle(style);
        m_changePack->setGetterName(getterNameLineEdit->text().toStdString());
        AccessSpecifier access;
        if (getterPublicRadioButton->isChecked()) {
            access = AccessSpecifier::AS_public;
        } else if (getterProtectedRadioButton->isChecked()) {
            access = AccessSpecifier::AS_protected;
        } else {
            Q_ASSERT(getterPrivateRadioButton->isChecked());
            access = AccessSpecifier::AS_private;
        }
        m_changePack->setGetterAccess(access);
        m_changePack->setCreateSetter(setterGroupBox->isChecked());
        if (m_changePack->createSetter()) {
            m_changePack->setSetterName(setterNameLineEdit->text().toStdString());
            if (setterPublicRadioButton->isChecked()) {
                access = AccessSpecifier::AS_public;
            } else if (setterProtectedRadioButton->isChecked()) {
                access = AccessSpecifier::AS_protected;
            } else {
                Q_ASSERT(setterPrivateRadioButton->isChecked());
                access = AccessSpecifier::AS_private;
            }
            m_changePack->setSetterAccess(access);
        }
    });
}

