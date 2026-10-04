#pragma once

#include <QString>
#include <QMetaType>

struct Result
{
    bool ok;
    int code;
    QString message;

    // 默认构造：表示成功
    Result()
        : ok(true), code(0), message(QString())
    {
    }

    // 带参数构造：用于 success / fail
    Result(bool isOk, int errCode, const QString& msg)
        : ok(isOk), code(errCode), message(msg)
    {
    }

    // 成功结果
    static Result success()
    {
        return Result(true, 0, QString());
    }

    // 失败结果
    static Result fail(int errCode, const QString& msg)
    {
        return Result(false, errCode, msg);
    }
};

Q_DECLARE_METATYPE(Result)
