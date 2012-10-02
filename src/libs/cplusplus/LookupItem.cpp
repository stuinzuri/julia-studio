/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "LookupItem.h"
#include <FullySpecifiedType.h>
#include <Symbol.h>
#include <Control.h>

#include <QDebug>

using namespace CPlusPlus;

uint CPlusPlus::qHash(const CPlusPlus::LookupItem &key)
{
    const uint h1 = QT_PREPEND_NAMESPACE(qHash)(key.type().type());
    const uint h2 = QT_PREPEND_NAMESPACE(qHash)(key.scope());
    return ((h1 << 16) | (h1 >> 16)) ^ h2;
}

LookupItem::LookupItem()
    : _scope(0), _declaration(0), _binding(0)
{ }

FullySpecifiedType LookupItem::type() const
{
    if (! _type && _declaration)
        return _declaration->type();

    return _type;
}

void LookupItem::setType(const FullySpecifiedType &type)
{ _type = type; }

Symbol *LookupItem::declaration() const
{ return _declaration; }

void LookupItem::setDeclaration(Symbol *declaration)
{ _declaration = declaration; }

Scope *LookupItem::scope() const
{
    if (! _scope && _declaration)
        return _declaration->enclosingScope();

    return _scope;
}

void LookupItem::setScope(Scope *scope)
{ _scope = scope; }

ClassOrNamespace *LookupItem::binding() const
{ return _binding; }

void LookupItem::setBinding(ClassOrNamespace *binding)
{ _binding = binding; }

bool LookupItem::operator == (const LookupItem &other) const
{
    if (_type == other._type && _declaration == other._declaration && _scope == other._scope
            && _binding == other._binding)
        return true;

    return false;
}

bool LookupItem::operator != (const LookupItem &result) const
{ return ! operator == (result); }
