#include "common.h"
#include <QString>
#include <QRegularExpression>


Common::Common()
{
}

QString Common::Stem(QString word) {
    QString newWord = word.replace(QRegularExpression ("(ая|ый|ые|ое|у|а|и|ии|ого|ой|ую)$"), "");
    if (newWord.length() <= 5) {
        return word;
    }
    return newWord;
}
