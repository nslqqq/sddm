/***************************************************************************
* Copyright (c) 2013 Nikita Mikhaylov <nslqqq@gmail.com>
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


#include "KeyboardModel.h"

#define explicit explicit_is_keyword_in_cpp
#include <xcb/xkb.h>
#undef explicit
#include <cstdint>
#include <QDebug>
#include <QRegExp>
#include <QSet>
#include <QList>

namespace SDDM {
    class Layout : public QObject {
        Q_OBJECT
        Q_PROPERTY(QString shortName READ shortName CONSTANT)
        Q_PROPERTY(QString longName READ longName CONSTANT)
    public:
        Layout(QString shortName, QString longName) : m_short(shortName), m_long(longName) {
        }

        virtual ~Layout() = default;

        QString shortName() const {
            return m_short;
        }

        QString longName() const {
            return m_long;
        }

    private:
        QString m_short, m_long;
    };

    struct Indicator {
        bool enabled { false };
        uint8_t mask { 0 };
    };

    class KeyboardModelPrivate {
    public:
        KeyboardModelPrivate();
        ~KeyboardModelPrivate();

        void updateState();
        QString atomName(xcb_atom_t atom) const;
        void setLayout(int id);

        bool enabled { true };

        Indicator numlock, capslock;

        int layout_id { 0 };
        QList<QObject*> layouts;

        xcb_connection_t *conn { nullptr };

    private:
        void connect();
        void initLedMap();
        void initLayouts();
        void initState();

        uint8_t getIndicatorMask(uint8_t id) const;
        static QList<QString> parseShortNames(QString text);
    };

    KeyboardModel::KeyboardModel() : d(new KeyboardModelPrivate) {
    }

    KeyboardModel::~KeyboardModel() {
        delete d;
    }

    bool KeyboardModel::numLockState() const {
        return d->numlock.enabled;
    }

    void KeyboardModel::setNumLockState(bool state) {
        if (d->numlock.enabled != state) {
            d->numlock.enabled = state;
            d->updateState();

            emit numLockStateChanged();
        }
    }

    bool KeyboardModel::capsLockState() const {
        return d->capslock.enabled;
    }

    void KeyboardModel::setCapsLockState(bool state) {
        if (d->capslock.enabled != state) {
            d->capslock.enabled = state;
            d->updateState();

            emit capsLockStateChanged();
        }
    }

    QList<QObject*> KeyboardModel::layouts() const {
        return d->layouts;
    }

    int KeyboardModel::currentLayout() const {
        return d->layout_id;
    }

    void KeyboardModel::setCurrentLayout(int id) {
        if (d->layout_id != id) {
            d->layout_id = id;
            d->updateState();

            emit currentLayoutChanged();
        }
    }

    bool KeyboardModel::enabled() const {
        return d->enabled;
    }

    KeyboardModelPrivate::KeyboardModelPrivate() {
        connect();
        if (enabled)
            initLedMap();
        if (enabled)
            initLayouts();
        if (enabled)
            initState();
    }


    void KeyboardModelPrivate::connect() {
        // Connect and initialize xkb extention
        xcb_xkb_use_extension_cookie_t cookie;
        xcb_generic_error_t *error = nullptr;

        conn = xcb_connect(nullptr, nullptr);
        if (conn == nullptr) {
            qCritical() << "xcb_connect failed, keyboard extention disabled";
            enabled = false;
            return;
        }

        // Initialize xkb extension
        cookie = xcb_xkb_use_extension(conn, XCB_XKB_MAJOR_VERSION, XCB_XKB_MINOR_VERSION);
        xcb_xkb_use_extension_reply(conn, cookie, &error);

        if (error != nullptr) {
            qCritical() << "xcb_xkb_use_extension failed, keyboard extention disabled, error code"
                        << error->error_code;
            enabled = false;
            return;
        }
    }

    KeyboardModelPrivate::~KeyboardModelPrivate() {
        // Delete layout objects and disconnect
        for (QObject *layout: layouts) {
            delete layout;
        }

        xcb_disconnect(conn);
    }

    void KeyboardModelPrivate::initLedMap() {
        // Get indicator names atoms
        xcb_xkb_get_names_cookie_t cookie;
        xcb_xkb_get_names_reply_t *reply = nullptr;
        xcb_generic_error_t *error = nullptr;

        cookie = xcb_xkb_get_names(conn,
                XCB_XKB_ID_USE_CORE_KBD,
                XCB_XKB_NAME_DETAIL_INDICATOR_NAMES);
        reply = xcb_xkb_get_names_reply(conn, cookie, &error);

        if (error) {
            qCritical() << "Can't init led map: " << error->error_code;
            enabled = false;
            return;
        }

        // Get indicators count
        xcb_xkb_get_names_value_list_t *list = (xcb_xkb_get_names_value_list_t *)xcb_xkb_get_names_value_list(reply);
        int ind_cnt = xcb_xkb_get_names_value_list_indicator_names_length(reply, list);
        xcb_atom_t *indicators = (xcb_atom_t *) list;

        // Loop through indicators and get their properties
        for (int i = 0; i < ind_cnt; i++) {
            QString name = atomName(indicators[i]);

            if (name == "Num Lock") {
                numlock.mask = getIndicatorMask(i);
            } else if (name == "Caps Lock") {
                capslock.mask = getIndicatorMask(i);
            }
        }

        // Free memory
        free(reply);
    }

    void KeyboardModelPrivate::initLayouts() {
        xcb_xkb_get_names_cookie_t cookie;
        xcb_xkb_get_names_reply_t *reply = nullptr;
        xcb_generic_error_t *error = nullptr;

        // Get atoms for short and long names
        cookie = xcb_xkb_get_names(conn,
                XCB_XKB_ID_USE_CORE_KBD,
                XCB_XKB_NAME_DETAIL_GROUP_NAMES | XCB_XKB_NAME_DETAIL_SYMBOLS);
        reply = xcb_xkb_get_names_reply(conn, cookie, nullptr);

        if (error) {
            // Log and disable
            qCritical() << "Can't init layouts: " << error->error_code;
            return;
        }
        xcb_xkb_get_names_value_list_t * val = (xcb_xkb_get_names_value_list_t *)xcb_xkb_get_names_value_list(reply);

        // Actual count is cnt + 1, since NAME_DETAIL_SYMBOLS is requested
        int cnt = xcb_xkb_get_names_value_list_groups_length(reply, val);


        xcb_atom_t *ptr = (xcb_atom_t *)val;

        // Get short names
        QList<QString> short_names = parseShortNames(atomName(ptr[0]));

        // Loop through group names
        layouts.clear();
        for (int i = 0; i < cnt; i++) {
            QString nshort, nlong = atomName(ptr[i + 1]);
            if (i < short_names.length())
                nshort = short_names[i];

            layouts << new Layout(nshort, nlong);
        }

        // Free
        free(reply);
    }

    void KeyboardModelPrivate::initState() {
        xcb_xkb_get_state_cookie_t cookie;
        xcb_xkb_get_state_reply_t *reply = nullptr;
        xcb_generic_error_t *error = nullptr;

        // Get xkb state
        cookie = xcb_xkb_get_state(conn, XCB_XKB_ID_USE_CORE_KBD);
        reply = xcb_xkb_get_state_reply(conn, cookie, &error);

        if (reply) {
            // Set locks state
            capslock.enabled = reply->lockedMods & capslock.mask;
            numlock.enabled = reply->lockedMods & numlock.mask;

            // Set current layout
            layout_id = reply->group;

            // Free
            free(reply);
        } else {
            // Log error and disable extension
            qCritical() << "Can't load leds state - " << error->error_code;
            enabled = false;
        }
    }

    void KeyboardModelPrivate::updateState() {
        xcb_void_cookie_t cookie;
        xcb_generic_error_t *error = nullptr;

        // Compute masks
        uint8_t mask_full = numlock.mask | capslock.mask,
                mask_cur  = (numlock.enabled ? numlock.mask : 0) |
                            (capslock.enabled ? capslock.mask : 0);

        // Change state
        cookie = xcb_xkb_latch_lock_state(conn,
                        XCB_XKB_ID_USE_CORE_KBD,
                        mask_full,
                        mask_cur,
                        1,
                        layout_id,
                        0, 0, 0);
        error = xcb_request_check(conn, cookie);

        if (error) {
            qWarning() << "Can't update state: " << error->error_code;
        }
    }

    uint8_t KeyboardModelPrivate::getIndicatorMask(uint8_t i) const {
        xcb_xkb_get_indicator_map_cookie_t cookie;
        xcb_xkb_get_indicator_map_reply_t *reply = nullptr;
        xcb_generic_error_t *error = nullptr;
        uint8_t mask = 0;

        cookie = xcb_xkb_get_indicator_map(conn, XCB_XKB_ID_USE_CORE_KBD, 1 << i);
        reply = xcb_xkb_get_indicator_map_reply(conn, cookie, &error);


        if (reply) {
            xcb_xkb_indicator_map_t *map = xcb_xkb_get_indicator_map_maps(reply);

            mask = map->mods;

            free(reply);
        } else {
            // Log error
            qWarning() << "Can't get indicator mask " << error->error_code;
        }
        return mask;
    }

    QList<QString> KeyboardModelPrivate::parseShortNames(QString text) {
        QRegExp re(R"(\+([a-z]+))");
        re.setCaseSensitivity(Qt::CaseInsensitive);

        QList<QString> res;
        QSet<QString> blackList;
        blackList << "inet" << "group";

        int pos = 0;
        while ((pos = re.indexIn(text, pos)) != -1) {
            if (!blackList.contains(re.cap(1)))
                res << re.cap(1);
            pos += re.matchedLength();
        }
        return res;
    }

    QString KeyboardModelPrivate::atomName(xcb_atom_t atom) const {
        xcb_get_atom_name_cookie_t cookie;
        xcb_get_atom_name_reply_t *reply = nullptr;
        xcb_generic_error_t *error = nullptr;

        // Get atom name
        cookie = xcb_get_atom_name(conn, atom);
        reply = xcb_get_atom_name_reply(conn, cookie, &error);

        QString res = "";

        if (reply) {
            res = QByteArray(xcb_get_atom_name_name(reply),
                             xcb_get_atom_name_name_length(reply));
            free(reply);
        } else {
            // Log error
            qWarning() << "Failed to get atom name: " << error->error_code;
        }
        return res;
    }

    void KeyboardModelPrivate::setLayout(int id) {
    }




}


// FIXME
#include "KeyboardModel.moc"
