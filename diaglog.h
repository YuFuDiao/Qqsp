#ifndef QQSP_DIAGLOG_H
#define QQSP_DIAGLOG_H

#include <QCoreApplication>#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QString>
#include <QTextStream>

// Diagnostic logging for the blank-render investigation.
// Writes to several fixed locations so the log is found regardless of how
// TEMP happens to be configured for the process.
inline void qqspDiagTo(const QString& path, const QString& msg)
{
        QFile f(path);
        if (!f.open(QIODevice::Append | QIODevice::Text)) {
                return;
        }
        QTextStream s(&f);
        s << QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss.zzz"))
          << QStringLiteral("  ") << msg << QStringLiteral("\n");
}

inline void qqspDiag(const QString& msg)
{
        qqspDiagTo(QDir::tempPath() + QStringLiteral("/qqsp_diag.log"), msg);
        qqspDiagTo(QStringLiteral("C:/qqsp_diag.log"), msg);
        if (!QCoreApplication::applicationDirPath().isEmpty()) {
                qqspDiagTo(QCoreApplication::applicationDirPath() + QStringLiteral("/qqsp_diag.log"), msg);
        }
}

#endif
