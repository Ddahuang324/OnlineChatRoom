/****************************************************************************
** Meta object code from reading C++ file 'ChatViewModel.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.9.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/viewmodel/ChatViewModel.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ChatViewModel.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.9.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN13ChatViewModelE_t {};
} // unnamed namespace

template <> constexpr inline auto ChatViewModel::qt_create_metaobjectdata<qt_meta_tag_ZN13ChatViewModelE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "ChatViewModel",
        "messageReceived",
        "",
        "MessageRecord",
        "msg",
        "messageReceivedText",
        "text",
        "messageSend",
        "messageSendText",
        "messageSendStatusUpdated",
        "roomsViewModelChanged",
        "connectionStatusChanged",
        "connected",
        "loggedInChanged",
        "onIncomingMessage",
        "onSendMessageAck",
        "serverMessageID",
        "code",
        "onCurrentRoomChanged",
        "roomId",
        "handleConnectionEvent",
        "connectionEvents",
        "event",
        "sendText",
        "sendFile",
        "filePath",
        "isConnected",
        "isLoggedIn"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'messageReceived'
        QtMocHelpers::SignalData<void(const MessageRecord &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Signal 'messageReceivedText'
        QtMocHelpers::SignalData<void(const QString &)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 6 },
        }}),
        // Signal 'messageSend'
        QtMocHelpers::SignalData<void(const MessageRecord &)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Signal 'messageSendText'
        QtMocHelpers::SignalData<void(const QString &)>(8, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 6 },
        }}),
        // Signal 'messageSendStatusUpdated'
        QtMocHelpers::SignalData<void(const MessageRecord &)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Signal 'roomsViewModelChanged'
        QtMocHelpers::SignalData<void()>(10, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'connectionStatusChanged'
        QtMocHelpers::SignalData<void(bool)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 12 },
        }}),
        // Signal 'loggedInChanged'
        QtMocHelpers::SignalData<void()>(13, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onIncomingMessage'
        QtMocHelpers::SlotData<void(const MessageRecord &)>(14, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Slot 'onSendMessageAck'
        QtMocHelpers::SlotData<void(const MessageRecord &, qint64, int)>(15, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 3, 4 }, { QMetaType::LongLong, 16 }, { QMetaType::Int, 17 },
        }}),
        // Slot 'onCurrentRoomChanged'
        QtMocHelpers::SlotData<void(const QString &)>(18, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 19 },
        }}),
        // Slot 'handleConnectionEvent'
        QtMocHelpers::SlotData<void(connectionEvents)>(20, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 21, 22 },
        }}),
        // Method 'sendText'
        QtMocHelpers::MethodData<void(const QString &)>(23, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 6 },
        }}),
        // Method 'sendFile'
        QtMocHelpers::MethodData<void(const QString &)>(24, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 25 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'isConnected'
        QtMocHelpers::PropertyData<bool>(26, QMetaType::Bool, QMC::DefaultPropertyFlags, 6),
        // property 'isLoggedIn'
        QtMocHelpers::PropertyData<bool>(27, QMetaType::Bool, QMC::DefaultPropertyFlags, 7),
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<ChatViewModel, qt_meta_tag_ZN13ChatViewModelE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject ChatViewModel::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13ChatViewModelE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13ChatViewModelE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN13ChatViewModelE_t>.metaTypes,
    nullptr
} };

void ChatViewModel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<ChatViewModel *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->messageReceived((*reinterpret_cast< std::add_pointer_t<MessageRecord>>(_a[1]))); break;
        case 1: _t->messageReceivedText((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 2: _t->messageSend((*reinterpret_cast< std::add_pointer_t<MessageRecord>>(_a[1]))); break;
        case 3: _t->messageSendText((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 4: _t->messageSendStatusUpdated((*reinterpret_cast< std::add_pointer_t<MessageRecord>>(_a[1]))); break;
        case 5: _t->roomsViewModelChanged(); break;
        case 6: _t->connectionStatusChanged((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 7: _t->loggedInChanged(); break;
        case 8: _t->onIncomingMessage((*reinterpret_cast< std::add_pointer_t<MessageRecord>>(_a[1]))); break;
        case 9: _t->onSendMessageAck((*reinterpret_cast< std::add_pointer_t<MessageRecord>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<qint64>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[3]))); break;
        case 10: _t->onCurrentRoomChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 11: _t->handleConnectionEvent((*reinterpret_cast< std::add_pointer_t<connectionEvents>>(_a[1]))); break;
        case 12: _t->sendText((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 13: _t->sendFile((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (ChatViewModel::*)(const MessageRecord & )>(_a, &ChatViewModel::messageReceived, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (ChatViewModel::*)(const QString & )>(_a, &ChatViewModel::messageReceivedText, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (ChatViewModel::*)(const MessageRecord & )>(_a, &ChatViewModel::messageSend, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (ChatViewModel::*)(const QString & )>(_a, &ChatViewModel::messageSendText, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (ChatViewModel::*)(const MessageRecord & )>(_a, &ChatViewModel::messageSendStatusUpdated, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (ChatViewModel::*)()>(_a, &ChatViewModel::roomsViewModelChanged, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (ChatViewModel::*)(bool )>(_a, &ChatViewModel::connectionStatusChanged, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (ChatViewModel::*)()>(_a, &ChatViewModel::loggedInChanged, 7))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<bool*>(_v) = _t->isConnected(); break;
        case 1: *reinterpret_cast<bool*>(_v) = _t->isLoggedIn(); break;
        default: break;
        }
    }
}

const QMetaObject *ChatViewModel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ChatViewModel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13ChatViewModelE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int ChatViewModel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 14)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 14;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 14)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 14;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    }
    return _id;
}

// SIGNAL 0
void ChatViewModel::messageReceived(const MessageRecord & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void ChatViewModel::messageReceivedText(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void ChatViewModel::messageSend(const MessageRecord & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void ChatViewModel::messageSendText(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}

// SIGNAL 4
void ChatViewModel::messageSendStatusUpdated(const MessageRecord & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1);
}

// SIGNAL 5
void ChatViewModel::roomsViewModelChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void ChatViewModel::connectionStatusChanged(bool _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 6, nullptr, _t1);
}

// SIGNAL 7
void ChatViewModel::loggedInChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}
QT_WARNING_POP
