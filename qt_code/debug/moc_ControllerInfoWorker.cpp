/****************************************************************************
** Meta object code from reading C++ file 'ControllerInfoWorker.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../worker/ControllerInfoWorker.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ControllerInfoWorker.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ControllerInfoWorker_t {
    QByteArrayData data[25];
    char stringdata0[362];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_ControllerInfoWorker_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_ControllerInfoWorker_t qt_meta_stringdata_ControllerInfoWorker = {
    {
QT_MOC_LITERAL(0, 0, 20), // "ControllerInfoWorker"
QT_MOC_LITERAL(1, 21, 12), // "stateUpdated"
QT_MOC_LITERAL(2, 34, 0), // ""
QT_MOC_LITERAL(3, 35, 23), // "ControllerStateSnapshot"
QT_MOC_LITERAL(4, 59, 8), // "snapshot"
QT_MOC_LITERAL(5, 68, 19), // "sensorBatchReceived"
QT_MOC_LITERAL(6, 88, 16), // "SensorTableBatch"
QT_MOC_LITERAL(7, 105, 5), // "batch"
QT_MOC_LITERAL(8, 111, 19), // "statusBatchReceived"
QT_MOC_LITERAL(9, 131, 16), // "StatusTableBatch"
QT_MOC_LITERAL(10, 148, 12), // "monitorError"
QT_MOC_LITERAL(11, 161, 6), // "Result"
QT_MOC_LITERAL(12, 168, 6), // "result"
QT_MOC_LITERAL(13, 175, 17), // "startStateMonitor"
QT_MOC_LITERAL(14, 193, 10), // "intervalMs"
QT_MOC_LITERAL(15, 204, 16), // "stopStateMonitor"
QT_MOC_LITERAL(16, 221, 17), // "startSensorUpload"
QT_MOC_LITERAL(17, 239, 17), // "SensorTableConfig"
QT_MOC_LITERAL(18, 257, 6), // "config"
QT_MOC_LITERAL(19, 264, 16), // "stopSensorUpload"
QT_MOC_LITERAL(20, 281, 18), // "startStatusMonitor"
QT_MOC_LITERAL(21, 300, 17), // "stopStatusMonitor"
QT_MOC_LITERAL(22, 318, 13), // "pollStateOnce"
QT_MOC_LITERAL(23, 332, 14), // "pollSensorOnce"
QT_MOC_LITERAL(24, 347, 14) // "pollStatusOnce"

    },
    "ControllerInfoWorker\0stateUpdated\0\0"
    "ControllerStateSnapshot\0snapshot\0"
    "sensorBatchReceived\0SensorTableBatch\0"
    "batch\0statusBatchReceived\0StatusTableBatch\0"
    "monitorError\0Result\0result\0startStateMonitor\0"
    "intervalMs\0stopStateMonitor\0"
    "startSensorUpload\0SensorTableConfig\0"
    "config\0stopSensorUpload\0startStatusMonitor\0"
    "stopStatusMonitor\0pollStateOnce\0"
    "pollSensorOnce\0pollStatusOnce"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ControllerInfoWorker[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      13,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   79,    2, 0x06 /* Public */,
       5,    1,   82,    2, 0x06 /* Public */,
       8,    1,   85,    2, 0x06 /* Public */,
      10,    1,   88,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      13,    1,   91,    2, 0x0a /* Public */,
      15,    0,   94,    2, 0x0a /* Public */,
      16,    1,   95,    2, 0x0a /* Public */,
      19,    0,   98,    2, 0x0a /* Public */,
      20,    1,   99,    2, 0x0a /* Public */,
      21,    0,  102,    2, 0x0a /* Public */,
      22,    0,  103,    2, 0x08 /* Private */,
      23,    0,  104,    2, 0x08 /* Private */,
      24,    0,  105,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 6,    7,
    QMetaType::Void, 0x80000000 | 9,    7,
    QMetaType::Void, 0x80000000 | 11,   12,

 // slots: parameters
    QMetaType::Void, QMetaType::Int,   14,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 17,   18,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   14,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void ControllerInfoWorker::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ControllerInfoWorker *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->stateUpdated((*reinterpret_cast< const ControllerStateSnapshot(*)>(_a[1]))); break;
        case 1: _t->sensorBatchReceived((*reinterpret_cast< const SensorTableBatch(*)>(_a[1]))); break;
        case 2: _t->statusBatchReceived((*reinterpret_cast< const StatusTableBatch(*)>(_a[1]))); break;
        case 3: _t->monitorError((*reinterpret_cast< Result(*)>(_a[1]))); break;
        case 4: _t->startStateMonitor((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 5: _t->stopStateMonitor(); break;
        case 6: _t->startSensorUpload((*reinterpret_cast< const SensorTableConfig(*)>(_a[1]))); break;
        case 7: _t->stopSensorUpload(); break;
        case 8: _t->startStatusMonitor((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 9: _t->stopStatusMonitor(); break;
        case 10: _t->pollStateOnce(); break;
        case 11: _t->pollSensorOnce(); break;
        case 12: _t->pollStatusOnce(); break;
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
        case 6:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< SensorTableConfig >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ControllerInfoWorker::*)(const ControllerStateSnapshot & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ControllerInfoWorker::stateUpdated)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (ControllerInfoWorker::*)(const SensorTableBatch & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ControllerInfoWorker::sensorBatchReceived)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (ControllerInfoWorker::*)(const StatusTableBatch & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ControllerInfoWorker::statusBatchReceived)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (ControllerInfoWorker::*)(Result );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ControllerInfoWorker::monitorError)) {
                *result = 3;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject ControllerInfoWorker::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ControllerInfoWorker.data,
    qt_meta_data_ControllerInfoWorker,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *ControllerInfoWorker::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ControllerInfoWorker::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ControllerInfoWorker.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int ControllerInfoWorker::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 13)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 13;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 13)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 13;
    }
    return _id;
}

// SIGNAL 0
void ControllerInfoWorker::stateUpdated(const ControllerStateSnapshot & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void ControllerInfoWorker::sensorBatchReceived(const SensorTableBatch & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void ControllerInfoWorker::statusBatchReceived(const StatusTableBatch & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void ControllerInfoWorker::monitorError(Result _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
