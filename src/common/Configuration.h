/***************************************************************************
* Copyright (c) 2013 Abdurrahman AVCI <abdurrahmanavci@gmail.com>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the
* Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
***************************************************************************/

#ifndef SDDM_CONFIGURATION_H
#define SDDM_CONFIGURATION_H

#include <QObject>
#include <QString>
#include <QStringList>

namespace SDDM {
    class ConfigurationPrivate;

    class Configuration : public QObject {
        Q_OBJECT
        Q_DISABLE_COPY(Configuration)
    public:
        Configuration(QObject *parent = 0);
        ~Configuration();

        void load();
        void saveState();

        static Configuration *instance();

        const QString &cursorTheme() const;

        const QString &defaultPath() const;

        const QString &serverPath() const;

        const QString &xauthPath() const;

        const QString &authDir() const;

        const QString &haltCommand() const;
        const QString &rebootCommand() const;

        const QString &sessionsDir() const;

        const QString &sessionCommand() const;

        const QString &facesDir() const;

        const QString &themesDir() const;
        const QString &currentTheme() const;
        QString currentThemePath() const;

        const int minimumUid() const;
        const int maximumUid() const;
        const QStringList &hideUsers() const;
        const QStringList &hideShells() const;

        const QString &autoUser() const;
        bool autoRelogin() const;

        enum NumState { NUM_NONE, NUM_SET_ON, NUM_SET_OFF };
        const NumState numlock() const;

        bool first { true };

        bool testing { false };

        uint minimumVT { 7 };

        // State information
        const QString &lastUser() const;
        void setLastUser(const QString &lastUser);

        const QVariantMap &lastSessions() const; // Maps user name -> last session
        void setLastSessions(const QVariantMap &lastSessions);

    private:
        ConfigurationPrivate *d { nullptr };
    };
}
#endif // SDDM_CONFIGURATION_H
