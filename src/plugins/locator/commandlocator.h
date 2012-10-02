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

#ifndef COMMANDLOCATOR_H
#define COMMANDLOCATOR_H

#include "locator_global.h"
#include "ilocatorfilter.h"
#include <QList>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace Core {
    class Command;
}

namespace Locator {
/* Command locators: Provides completion for a set of
 * Core::Command's by sub-string of their action's text. */

struct CommandLocatorPrivate;

class LOCATOR_EXPORT CommandLocator : public Locator::ILocatorFilter
{
    Q_OBJECT

public:
    explicit CommandLocator(const QString &prefix,
                            const QString &displayName,
                            const QString &shortCutString,
                            QObject *parent = 0);
    virtual ~CommandLocator();

    void appendCommand(Core::Command *cmd);

    virtual QString displayName() const;
    virtual QString id() const;
    virtual Priority priority() const;
    virtual QList<FilterEntry> matchesFor(QFutureInterface<Locator::FilterEntry> &future, const QString &entry);
    virtual void accept(FilterEntry selection) const;
    virtual void refresh(QFutureInterface<void> &future);

private:
    CommandLocatorPrivate *d;
};

inline CommandLocator &operator<<(CommandLocator &locator, Core::Command *cmd)
{
    locator.appendCommand(cmd);
    return locator;
}

} // namespace Locator

#endif // COMMANDLOCATOR_H
