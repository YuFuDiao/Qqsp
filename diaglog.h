#ifndef QQSP_DIAGLOG_H
#define QQSP_DIAGLOG_H

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QString>
#include <QTextStream>

// Diagnostic logging for the blank-render investigation.  Appends one line per
// call to TEMP/qqsp_diag.log; opens and closes the file every time so calls from
// different threads cannot corrupt each other.
inline void qqspDiag(const QString& msg)
{
        QFile f(QDir::tempPath() + QStringLiteral("/qqsp_diag.log"));
        if (!f.open(QIODevice::Append | QIODevice::Text)) {
                return;
        }
        QTextStream s(&f);
        s << QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss.zzz"))
          << QStringLiteral("  ") << msg << QStringLiteral("\n");
}

#endif
