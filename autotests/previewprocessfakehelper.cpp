/*
    SPDX-FileCopyrightText: 2026 Latte Dock Contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <unistd.h>

namespace {
void writeMessage(const QJsonObject &message)
{
    const QByteArray bytes = QJsonDocument(message).toJson(QJsonDocument::Compact) + '\n';
    const auto written = ::write(STDOUT_FILENO, bytes.constData(), static_cast<size_t>(bytes.size()));
    if (written != bytes.size()) {
        QCoreApplication::exit(1);
    }
}
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QByteArray input;
    char buffer[4096];
    int serial = 0;

    while (true) {
        const auto length = ::read(STDIN_FILENO, buffer, sizeof(buffer));
        if (length <= 0) {
            break;
        }
        input.append(buffer, static_cast<qsizetype>(length));
        while (input.contains('\n')) {
            const auto end = input.indexOf('\n');
            const QJsonObject request = QJsonDocument::fromJson(input.left(end)).object();
            input.remove(0, end + 1);
            serial = request.value(QStringLiteral("serial")).toInt();
            const QString type = request.value(QStringLiteral("type")).toString();

            if (type == QLatin1String("hide")) {
                writeMessage({{QStringLiteral("type"), QStringLiteral("heartbeat")},
                              {QStringLiteral("serial"), serial},
                              {QStringLiteral("visible"), false},
                              {QStringLiteral("hovered"), false}});
                continue;
            }
            if (type == QLatin1String("move")) {
                writeMessage({{QStringLiteral("type"), QStringLiteral("heartbeat")},
                              {QStringLiteral("serial"), serial},
                              {QStringLiteral("visible"), true},
                              {QStringLiteral("hovered"), true}});
                continue;
            }
            if (type != QLatin1String("show")) {
                continue;
            }

            writeMessage({{QStringLiteral("type"), QStringLiteral("heartbeat")},
                          {QStringLiteral("serial"), serial},
                          {QStringLiteral("visible"), true},
                          {QStringLiteral("hovered"), false}});

            const QJsonObject window = request.value(QStringLiteral("windows")).toArray().first().toObject();
            const QString command = window.value(QStringLiteral("title")).toString();
            QString uuid = window.value(QStringLiteral("uuid")).toString();
            QString replyType;
            if (command == QLatin1String("close")) {
                replyType = QStringLiteral("close");
            } else if (command == QLatin1String("stale-close")) {
                replyType = QStringLiteral("close");
                uuid = QStringLiteral("ffffffff-ffff-ffff-ffff-ffffffffffff");
            } else if (command == QLatin1String("activate")) {
                replyType = QStringLiteral("activate");
            }
            if (!replyType.isEmpty()) {
                writeMessage({{QStringLiteral("type"), replyType},
                              {QStringLiteral("serial"), serial},
                              {QStringLiteral("uuid"), uuid}});
            }
        }
    }

    writeMessage({{QStringLiteral("type"), QStringLiteral("closed")},
                  {QStringLiteral("serial"), serial}});
    return 0;
}
