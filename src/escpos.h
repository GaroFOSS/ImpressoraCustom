#pragma once

#include <QByteArray>
#include <QString>

namespace EscPos {
// Size: 0 = Small, 1 = Medium, 2 = Big. Alignment: 0 = Left, 1 = Center, 2 = Right.
// Returns an empty payload and sets error when input cannot be printed safely.
QByteArray encode(const QString &text, int size, int alignment, QString *error);
}
