/****************************************************************************
** Meta object code from reading C++ file 'TrajectorySendWorker.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../worker/TrajectorySendWorker.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'TrajectorySendWorker.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_TrajectorySendWorker_t {
    QByteArrayData data[16];
    char stringdata0[154];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_TrajectorySendWorker_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_TrajectorySendWorker_t qt_meta_stringdata_TrajectorySendWorker = {
    {
QT_MOC_LITERAL(0, 0, 20), // "TrajectorySendWorker"
QT_MOC_LITERAL(1, 21, 15), // "progressChanged"
QT_MOC_LITERAL(2, 37, 0), // ""
QT_MOC_LITERAL(3, 38, 10), // "sentGroups"
QT_MOC_LITERAL(4, 49, 11), // "totalGroups"
QT_MOC_LITERAL(5, 61, 7), // "percent"
QT_MOC_LITERAL(6, 69, 8), // "finished"
QT_MOC_LITERAL(7, 78, 6), // "Result"
QT_MOC_LITERAL(8, 85, 6), // "result"
QT_MOC_LITERAL(9, 92, 9), // "startSend"
QT_MOC_LITERAL(10, 102, 11), // "datFilePath"
QT_MOC_LITERAL(11, 114, 11), // "totalPoints"
QT_MOC_LITERAL(12, 126, 7), // "eventId"
QT_MOC_LITERAL(13, 134, 6), // "cancel"
QT_MOC_LITERAL(14, 141, 5), // "pause"
QT_MOC_LITERAL(15, 147, 6) // "resume"

    },
    "TrajectorySendWorker\0progressChanged\0"
    "\0sentGroups\0totalGroups\0percent\0"
    "finished\0Result\0result\0startSend\0"
    "datFilePath\0totalPoints\0eventId\0cancel\0"
    "pause\0resume"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_TrajectorySendWorker[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    3,   44,    2, 0x06 /* Public */,
       6,    1,   51,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       9,    3,   54,    2, 0x0a /* Public */,
      13,    0,   61,    2, 0x0a /* Public */,
      14,    0,   62,    2, 0x0a /* Public */,
      15,    0,   63,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::Int, QMetaType::Int, QMetaType::Int,    3,    4,    5,
    QMetaType::Void, 0x80000000 | 7,    8,

 // slots: parameters
    QMetaType::Void, QMetaType::QString, QMetaType::Int, QMetaType::Int,   10,   11,   12,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void TrajectorySendWorker::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<TrajectorySendWorker *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->progressChanged((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3]))); break;
        case 1: _t->finished((*reinterpret_cast< Result(*)>(_a[1]))); break;
        case 2: _t->startSend((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3]))); break;
        case 3: _t->cancel(); break;
        case 4: _t->pause(); break;
        case 5: _t->resume(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 1:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< Result >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (TrajectorySendWorker::*)(int , int , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TrajectorySendWorker::progressChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (TrajectorySendWorker::*)(Result );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TrajectorySendWorker::finished)) {
                *result = 1;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject TrajectorySendWorker::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_TrajectorySendWorker.data,
    qt_meta_data_TrajectorySendWorker,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *TrajectorySendWorker::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TrajectorySendWorker::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_TrajectorySendWorker.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int TrajectorySendWorker::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void TrajectorySendWorker::progressChanged(int _t1, int _t2, int _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void TrajectorySendWorker::finished(Result _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
