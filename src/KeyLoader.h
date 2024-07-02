/*
    Copyright (c) 2023 Vladimir Vorobyev <b800xy@gmail.com>
    Copyright 2011-2012 Heikki Holstila <heikki.holstila@gmail.com>

    This work is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This work is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this work.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef KEYLOADER_H
#define KEYLOADER_H

#include <QObject>
#include <QVariant>
#include <QList>

class QIODevice;

struct KeyData
{
    QString label;
    int code;
    QString label_alt;
    int code_alt;
    int width;
    bool isModifier;
};

class KeyLoader : public QObject
{
    Q_OBJECT

public:
    static constexpr char const *keyLayoutFilter  = "key-layout.*";
    static constexpr char const *keyLayoutDefault = ":/key-layout.en_US";

    explicit KeyLoader(const QString &layoutDir, QObject* parent = 0);
    ~KeyLoader() = default;

    Q_INVOKABLE bool loadLayout(const QString &locale = QString()); // null for keyLayoutDefault

    Q_INVOKABLE int vkbRows() const { return iVkbRows; }
    Q_INVOKABLE int vkbColumns() const { return iVkbColumns; }
    Q_INVOKABLE QVariantList keyAt(int row, int col) const;
    Q_INVOKABLE QStringList availableLayouts() const;

signals:

public slots:

private:
    Q_DISABLE_COPY(KeyLoader)
    bool loadLayoutInternal(QIODevice& from);
    void cleanUpKey(KeyData& key);

    int iVkbRows;
    int iVkbColumns;

    QList<QList<KeyData>> iKeyData;

    QString layout_dir;
};

#endif // KEYLOADER_H
