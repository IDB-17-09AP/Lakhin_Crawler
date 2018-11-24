#include <QCoreApplication>
#include <QTextStream>
#include <QFile>
#include "common.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QTextStream out(stdout);
    out.setCodec("UTF-8");
    QTextStream in(stdin);
    QString word;

    QString filename = "index.txt";
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        out << "Error" << endl;
        return 0;
    }
    QTextStream stream(&file);
    stream.setCodec("UTF-8");

    QHash<QString, QString> index;

    while (!stream.atEnd()) {
        QString line;
        line = stream.readLine();

        int pos = line.indexOf(" ");
        if (pos < 1)
            continue;
        index[line.left(pos)] = line.mid(pos + 1);
    }

    while (true) {
        out << QString("Введите поисковый запрос: ");
        out.flush();
        in >> word;
        if (word == "")
            break;

        word = Common::Stem(word);


        out << word << endl;

        if (!index.contains(word)) {
            out << QString("Не найдено") << endl;
            continue;
        }

        QStringList links = index[word].split(' ');
        for (int i = 0; i < links.length(); i++) {
            QStringList linkPart = links[i].split('#');
            out << linkPart[0] << " (" << (linkPart.length() - 1) << QString(" вхождений)") << endl;
        }
    }

    return 0;
}
