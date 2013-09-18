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

#include "Configuration.h"

#include <QSettings>

#include "Constants.h"

namespace SDDM {
    static Configuration *_instance = nullptr;

    class ConfigurationPrivate {
    public:
        QString configPath { "" };

        QString stateFilePath { "" };

        QString cursorTheme { "" };

        QString defaultPath { "" };

        QString serverPath { "" };

        QString xauthPath { "" };

        QString authDir { "" };

        QString haltCommand { "" };
        QString rebootCommand { "" };

        QString sessionsDir { "" };
        bool rememberLastSession { true };
        QString sessionCommand { "" };

        QString facesDir { "" };

        QString themesDir { "" };
        QString currentTheme { "" };

        int minimumUid { 0 };
        int maximumUid { 65000 };
        QStringList hideUsers;
        QStringList hideShells;

        bool rememberLastUser { true };

        QString autoUser { "" };
        bool autoRelogin { false };

        Configuration::NumState numlock { Configuration::NUM_NONE };

        // State information
        QString lastUser { "" };
        QVariantMap lastSessions;
    };

    Configuration::Configuration(QObject *parent) : QObject(parent), d(new ConfigurationPrivate()) {
        _instance = this;
        // set config path
        d->configPath = CONFIG_FILE;

        // set state file
        d->stateFilePath = STATE_FILE;

        // load settings
        load();
    }

    Configuration::~Configuration() {
        // clean up
        delete d;
    }

    QString appendSlash(const QString path) {
        if (path.isEmpty() || path.endsWith('/')) {
            return path;
        } else {
            return path + '/';
        }
    }

    void Configuration::load() {
        // create settings object
        QSettings settings(d->configPath, QSettings::IniFormat);
        // read settings
        d->cursorTheme = settings.value("CursorTheme", "").toString();
        d->defaultPath = settings.value("DefaultPath", "").toString();
        d->serverPath = settings.value("ServerPath", "").toString();
        d->xauthPath = settings.value("XauthPath", "").toString();
        d->authDir = appendSlash(settings.value("AuthDir", "").toString());
        d->haltCommand = settings.value("HaltCommand", "").toString();
        d->rebootCommand = settings.value("RebootCommand", "").toString();
        d->sessionsDir = appendSlash(settings.value("SessionsDir", "").toString());
        d->rememberLastSession = settings.value("RememberLastSession", d->rememberLastSession).toBool();
        d->sessionCommand = settings.value("SessionCommand", "").toString();
        d->facesDir = appendSlash(settings.value("FacesDir", "").toString());
        d->themesDir = appendSlash(settings.value("ThemesDir", "").toString());
        d->currentTheme = settings.value("CurrentTheme", "").toString();
        d->minimumUid = settings.value("MinimumUid", "0").toInt();
        d->maximumUid = settings.value("MaximumUid", "65000").toInt();
        d->hideUsers = settings.value("HideUsers", "").toString().split(' ', QString::SkipEmptyParts);
        d->hideShells = settings.value("HideShells", "").toString().split(' ', QString::SkipEmptyParts);
        d->rememberLastUser = settings.value("RememberLastUser", d->rememberLastUser).toBool();
        d->autoUser = settings.value("AutoUser", "").toString();
        d->autoRelogin = settings.value("AutoRelogin", d->autoRelogin).toBool();
        minimumVT = settings.value("MinimumVT", minimumVT).toUInt();

        QString num_val = settings.value("Numlock", "none").toString().toLower();
        if (num_val == "on") {
            d->numlock = Configuration::NUM_SET_ON;
        } else if (num_val == "off") {
            d->numlock = Configuration::NUM_SET_OFF;
        } else {
            d->numlock = Configuration::NUM_NONE;
        }

        // State information
        QSettings stateInfo(d->stateFilePath, QSettings::IniFormat);
        d->lastUser = stateInfo.value("LastUser", "").toString();
        d->lastSessions = stateInfo.value("LastSessions", QVariantMap()).toMap();
    }

    void Configuration::save() {
        // Save state
        QSettings stateInfo(d->configPath, QSettings::IniFormat);

        stateInfo.setValue("LastUser", d->lastUser);
        stateInfo.setValue("LastSessions", d->lastSessions);
    }

    Configuration *Configuration::instance() {
        return _instance;
    }

    const QString &Configuration::cursorTheme() const {
        return d->cursorTheme;
    }

    const QString &Configuration::defaultPath() const {
        return d->defaultPath;
    }

    const QString &Configuration::serverPath() const {
        return d->serverPath;
    }

    const QString &Configuration::xauthPath() const {
        return d->xauthPath;
    }

    const QString &Configuration::authDir() const {
        return d->authDir;
    }

    const QString &Configuration::haltCommand() const {
        return d->haltCommand;
    }

    const QString &Configuration::rebootCommand() const {
        return d->rebootCommand;
    }

    const QString &Configuration::sessionsDir() const {
        return d->sessionsDir;
    }

    const QVariantMap &Configuration::lastSessions() const {
        return d->lastSessions;
    }

    const QString &Configuration::sessionCommand() const {
        return d->sessionCommand;
    }

    void Configuration::setLastSessions(const QVariantMap &lastSessions) {
        if (d->rememberLastSession)
            d->lastSessions = lastSessions;
    }

    const QString &Configuration::facesDir() const {
        return d->facesDir;
    }

    const QString &Configuration::themesDir() const {
        return d->themesDir;
    }

    const QString &Configuration::currentTheme() const {
        return d->currentTheme;
    }

    QString Configuration::currentThemePath() const {
        return d->themesDir + d->currentTheme;
    }

    const int Configuration::minimumUid() const {
        return d->minimumUid;
    }

    const int Configuration::maximumUid() const {
        return d->maximumUid;
    }

    const QStringList &Configuration::hideUsers() const {
        return d->hideUsers;
    }

    const QStringList &Configuration::hideShells() const {
        return d->hideShells;
    }

    const QString &Configuration::lastUser() const {
        return d->lastUser;
    }

    void Configuration::setLastUser(const QString &lastUser) {
        if (d->rememberLastUser)
            d->lastUser = lastUser;
    }

    const QString &Configuration::autoUser() const {
        return d->autoUser;
    }

    bool Configuration::autoRelogin() const {
        return d->autoRelogin;
    }

    const Configuration::NumState Configuration::numlock() const {
        return d->numlock;
    }
}
