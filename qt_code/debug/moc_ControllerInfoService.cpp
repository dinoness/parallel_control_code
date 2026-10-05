/****************************************************************************
** Meta object code from reading C++ file 'ControllerInfoService.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../service/ControllerInfoService.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ControllerInfoService.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ControllerInfoService_t {
    QByteArrayData data[13];
    char stringdata0[176];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_ControllerInfoService_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_ControllerInfoService_t qt_meta_stringdata_ControllerInfoService = {
    {
QT_MOC_LITERAL(0, 0, 21), // "ControllerInfoService"
QT_MOC_LITERAL(1, 22, 12), // "stateUpdated"
QT_MOC_LITERAL(2, 35, 0), // ""
QT_MOC_LITERAL(3, 36, 23), // "ControllerStateSnapshot"
QT_MOC_LITERAL(4, 60, 8), // "snapshot"
QT_MOC_LITERAL(5, 69, 19), // "sensorBatchReceived"
QT_MOC_LITERAL(6, 89, 16), // "SensorTableBatch"
QT_MOC_LITERAL(7, 106, 5), // "batch"
QT_MOC_LITERAL(8, 112, 19), // "statusBatchReceived"
QT_MOC_LITERAL(9, 132, 16), // "StatusTableBatch"
QT_MOC_LITERAL(10, 149, 12), // "monitorError"
QT_MOC_LITERAL(11, 162, 6), // "Result"
QT_MOC_LITERAL(12, 169, 6) // "result"

    },
    "ControllerInfoService\0stateUpdated\0\0"
    "ControllerStateSnapshot\0snapshot\0"
    "sensorBatchReceived\0SensorTableBatch\0"
    "batch\0statusBatchReceived\0StatusTableBatch\0"
    "monitorError\0Result\0result"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ControllerInfoService[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   34,    2, 0x06 /* Public */,
       5,    1,   37,    2, 0x06 /* Public */,
       8,    1,   40,    2, 0x06 /* Public */,
      10,    1,   43,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 6,    7,
    QMetaType::Void, 0x80000000 | 9,    7,
    QMetaType::Void, 0x80000000 | 11,   12,

       0        // eod
};

void ControllerInfoService::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ControllerInfoService *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->stateUpdated((*reinterpret_cast< const ControllerStateSnapshot(*)>(_a[1]))); break;
        case 1: _t->sensorBatchReceived((*reinterpret_cast< const SensorTableBatch(*)>(_a[1]))); break;
        case 2: _t->statusBatchReceived((*reinterpret_cast< const StatusTableBatch(*)>(_a[1]))); break;
        case 3: _t->monitorError((*reinterpret_cast< Result(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 0:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< ControllerStateSnapshot >(); break;
            }
            break;
        case 1:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< SensorTableBatch >(); break;
            }
            break;
        case 2:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< StatusTableBatch >(); break;
            }
            break;
        case 3:
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
            using _t = void (ControllerInfoService::*)(const ControllerStateSnapshot & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ControllerInfoService::stateUpdated)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (ControllerInfoService::*)(const SensorTableBatch & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ControllerInfoService::sensorBatchReceived)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (ControllerInfoService::*)(const StatusTableBatch & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ControllerInfoService::statusBatchReceived)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (ControllerInfoService::*)(Result );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ControllerInfoService::monitorError)) {
                *result = 3;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject ControllerInfoService::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ControllerInfoService.data,
    qt_meta_data_ControllerInfoService,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *ControllerInfoService::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ControllerInfoService::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ControllerInfoService.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int ControllerInfoService::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void ControllerInfoService::stateUpdated(const ControllerStateSnapshot & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void ControllerInfoService::sensorBatchReceived(const SensorTableBatch & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void ControllerInfoService::statusBatchReceived(const StatusTableBatch & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void ControllerInfoService::monitorError(Result _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
