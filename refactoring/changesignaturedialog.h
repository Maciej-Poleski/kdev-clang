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

#ifndef KDEV_CLANG_CHANGESIGNATUREDIALOG_H
#define KDEV_CLANG_CHANGESIGNATUREDIALOG_H

// Qt
#include <QDialog>

#include "ui_changesignaturedialog.h"

#include "changesignaturerefactoring.h"

class ChangeSignatureDialog : public QDialog, private Ui::ChangeSignatureDialog
{
    Q_OBJECT;
    Q_DISABLE_COPY(ChangeSignatureDialog);

    class Model;

    using InfoPack = ChangeSignatureRefactoring::InfoPack;
    using ChangePack = ChangeSignatureRefactoring::ChangePack;

public:
    ChangeSignatureDialog(const InfoPack *infoPack, QWidget *parent = nullptr);

    const InfoPack *infoPack() const;
    const ChangePack *changePack() const;

private:
    void reinitializeDialogData();

private:
    Model *m_model;
};


#endif //KDEV_CLANG_CHANGESIGNATUREDIALOG_H