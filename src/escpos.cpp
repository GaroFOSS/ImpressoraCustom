#include "escpos.h"

#include <QList>

#include <array>
#include <algorithm>

namespace {
constexpr std::array<char16_t, 128> cp850 = {
    0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7,
    0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5,
    0x00C9, 0x00E6, 0x00C6, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9,
    0x00FF, 0x00D6, 0x00DC, 0x00F8, 0x00A3, 0x00D8, 0x00D7, 0x0192,
    0x00E1, 0x00ED, 0x00F3, 0x00FA, 0x00F1, 0x00D1, 0x00AA, 0x00BA,
    0x00BF, 0x00AE, 0x00AC, 0x00BD, 0x00BC, 0x00A1, 0x00AB, 0x00BB,
    0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x00C1, 0x00C2, 0x00C0,
    0x00A9, 0x2563, 0x2551, 0x2557, 0x255D, 0x00A2, 0x00A5, 0x2510,
    0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x00E3, 0x00C3,
    0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x00A4,
    0x00F0, 0x00D0, 0x00CA, 0x00CB, 0x00C8, 0x0131, 0x00CD, 0x00CE,
    0x00CF, 0x2518, 0x250C, 0x2588, 0x2584, 0x00A6, 0x00CC, 0x2580,
    0x00D3, 0x00DF, 0x00D4, 0x00D2, 0x00F5, 0x00D5, 0x00B5, 0x00FE,
    0x00DE, 0x00DA, 0x00DB, 0x00D9, 0x00FD, 0x00DD, 0x00AF, 0x00B4,
    0x00AD, 0x00B1, 0x2017, 0x00BE, 0x00B6, 0x00A7, 0x00F7, 0x00B8,
    0x00B0, 0x00A8, 0x00B7, 0x00B9, 0x00B3, 0x00B2, 0x25A0, 0x00A0
};
}

QByteArray EscPos::encode(const QString &text, int size, int alignment, QString *error)
{
    if (error)
        error->clear();
    const auto invalid = [error](const QString &message) {
        if (error)
            *error = message;
        return QByteArray{};
    };
    if (size < 0 || size > 2 || alignment < 0 || alignment > 2)
        return invalid(QStringLiteral("Invalid size or alignment."));
    if (text.trimmed().isEmpty())
        return invalid(QStringLiteral("Enter some text to print."));

    QString normalized = text.normalized(QString::NormalizationForm_C);
    normalized.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    normalized.replace(QChar('\r'), QChar('\n'));
    QByteArray encoded;
    for (const auto character : normalized.toUcs4()) {
        if (character == '\n' || (character >= 0x20 && character <= 0x7E)) {
            encoded.append(static_cast<char>(character));
            continue;
        }
        const auto found = std::find(cp850.begin(), cp850.end(), character);
        if (found == cp850.end())
            return invalid(QStringLiteral("Cannot print character U+%1 using CP850.")
                               .arg(QString::number(character, 16).rightJustified(4, QChar('0')).toUpper()));
        encoded.append(static_cast<char>(0x80 + std::distance(cp850.begin(), found)));
    }

    QByteArray job = QByteArray::fromHex("1b40"); // ESC @: initialize
    job += QByteArray::fromHex("1c2e"); // FS .: leave double-byte character mode
    job += QByteArray::fromHex("1b7402"); // ESC t 2: CP850 (verify on i8 firmware)
    job += QByteArray::fromHex("1b4d00"); // ESC M 0: normal Font A
    job += QByteArray::fromHex("1d21");
    job.append(static_cast<char>(size * 0x11));
    job += QByteArray::fromHex("1b61");
    job.append(static_cast<char>(alignment));
    job += encoded;
    if (!encoded.endsWith('\n'))
        job.append('\n');
    job += QByteArray::fromHex("1d2100"); // restore normal size before feeding
    job += QByteArray::fromHex("1b6406"); // ESC d 6: advance six lines before cutting
    job += QByteArray::fromHex("1d5601"); // GS V 1: partial cut
    return job;
}
