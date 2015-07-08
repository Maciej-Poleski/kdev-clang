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
#include <QAbstractTableModel>

// KF5
#include <KF5/KI18n/klocalizedstring.h>

#include "changesignaturedialog.h"
#include "changesignaturerefactoringinfopack.h"
#include "changesignaturerefactoringchangepack.h"

using InfoPack = ChangeSignatureRefactoring::InfoPack;
using ChangePack = ChangeSignatureRefactoring::ChangePack;

// TODO: make it modifiable
class ChangeSignatureDialog::Model : public QAbstractTableModel
{
    Q_OBJECT;
    Q_DISABLE_COPY(Model);
public:
    Model(const InfoPack *infoPack, QObject *parent);

    virtual int rowCount(const QModelIndex &parent) const override;

    virtual int columnCount(const QModelIndex &parent) const override;

    virtual QVariant data(const QModelIndex &index, int role) const override;

    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    virtual Qt::ItemFlags flags(const QModelIndex &index) const override;

    virtual bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    const InfoPack *infoPack() const
    {
        return m_infoPack;
    }

    void resetChanges();

private:
    const InfoPack *const m_infoPack;
    ChangePack *m_changePack;
};

ChangeSignatureDialog::ChangeSignatureDialog(const InfoPack *infoPack, QWidget *parent)
    : QDialog(parent), m_model(new Model(infoPack, this))
{
    setupUi(this);
    reinitializeDialogData();

    connect(dialogButtonBox->button(QDialogButtonBox::Reset), &QPushButton::clicked, this,
            &ChangeSignatureDialog::reinitializeDialogData);

    parametersTableView->setModel(m_model);
    parametersTableView->verticalHeader()->hide();
    parametersTableView->horizontalHeader()->setStretchLastSection(true);
}

void ChangeSignatureDialog::reinitializeDialogData()
{
    returnTypeLineEdit->setText(QString::fromStdString(m_model->infoPack()->returnType()));
    functionNameLineEdit->setText(QString::fromStdString(m_model->infoPack()->functionName()));
    if (m_model->infoPack()->isRestricted()) {
        functionNameLineEdit->setDisabled(true);
        returnTypeLineEdit->setDisabled(true);
    }
    m_model->resetChanges();
}


//////////////////// CHANGE MODEL

ChangeSignatureDialog::Model::Model(const InfoPack *infoPack, QObject *parent)
    : QAbstractTableModel(parent), m_infoPack(infoPack), m_changePack(new ChangePack(infoPack)) { }

int ChangeSignatureDialog::Model::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return static_cast<int>(m_infoPack->parameters().size());
}

int ChangeSignatureDialog::Model::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 2;
}

QVariant ChangeSignatureDialog::Model::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        Q_ASSERT(static_cast<int>(m_changePack->m_paramRefs.size()) > index.row());
        int row = m_changePack->m_paramRefs[index.row()];
        const auto &params = row >= 0 ? m_infoPack->parameters() : m_changePack->m_newParam;
        if (row < 0) {
            row = -row - 1;
        }
        auto &t = params[row];
        switch (index.column()) {
        case 0:
            return QString::fromStdString(std::get<0>(t));
        case 1:
            return QString::fromStdString(std::get<1>(t));
        }
    }
    return QVariant();
}

QVariant ChangeSignatureDialog::Model::headerData(int section, Qt::Orientation orientation,
                                                  int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case 0:
            return i18n("Type");
        case 1:
            return i18n("Name");
        }
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}

Qt::ItemFlags ChangeSignatureDialog::Model::flags(const QModelIndex &index) const
{
    Qt::ItemFlags result = QAbstractTableModel::flags(index);
    if (index.column() == 0 || m_changePack->m_paramRefs[index.row()] < 0) {
        result |= Qt::ItemIsEditable;
    }
    return result;
}

bool ChangeSignatureDialog::Model::setData(const QModelIndex &index, const QVariant &value,
                                           int role)
{
    if (role == Qt::EditRole) {
        int row = m_changePack->m_paramRefs[index.row()];
        if (row >= 0) {
            m_changePack->m_newParam.push_back(m_infoPack->parameters()[row]);
            row = -static_cast<int>(m_changePack->m_newParam.size());
            m_changePack->m_paramRefs[index.row()] = row;
        }
        Q_ASSERT(row < 0);
        auto &t = m_changePack->m_newParam[-row - 1];
        std::string newValue = value.toString().toStdString();
        switch (index.column()) {
        case 0:
            std::get<0>(t) = newValue;
            break;
        case 1:
            std::get<1>(t) = newValue;
            break;
        default:
            return false;
        }
        dataChanged(index, index, {role});
        return true;
    }
    return false;
}

void ChangeSignatureDialog::Model::resetChanges()
{
    beginResetModel();
    m_changePack = new ChangePack(m_infoPack);
    endResetModel();
}

#include "changesignaturedialog.moc"
