#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QEventLoop>
#include <QTextStream>
#include <QFile>
#include <QRegularExpression>
#include <QThread>
#include <algorithm>
#include "common.h"

//TODO заменить Ё на Е

QHash<QString, int> urlVisited;
QTextStream out(stdout);

const QString TARGET_SITE_RE = "\\Wstankin\\.ru\\W";
//const QString TARGET_SITE_RE = "\\Wkaiu\\.narod\\.ru\\W";
const int DELAY = 2;
const int MAXLEVEL = 1;

QStringList stopWords = {"об", "или", "на", "под", "из", "от", "как", "без", "по", "для"};

int parseWords(QString content, QString url, QHash<QString, QString> *index) {
    QHash<QString, QString> wordKnown;
    QRegularExpression wordRU("[а-яё]{2,}", QRegularExpression::CaseInsensitiveOption);

    int i = 0;
    QRegularExpressionMatchIterator iter = wordRU.globalMatch(content);
    while (iter.hasNext()) {
        QRegularExpressionMatch match = iter.next();
        if (match.hasMatch()) {
            QString word = match.captured(0).toLower();
            int num = match.capturedStart();
            if (stopWords.contains(word)) {
                continue;
            }

            word = Common::Stem(word);

            if (!wordKnown.contains(word)) {
                wordKnown[word] = url;
            }
            wordKnown[word] = wordKnown[word] + "#" + QString::number(num);
            i++;
        }
    }

    for (QHash<QString, QString>::const_iterator it = wordKnown.cbegin(), end = wordKnown.cend(); it != end; ++it) {
        if (index->contains(it.key())) {
            (*index)[it.key()] = (*index)[it.key()] + " " + it.value();
        } else {
            (*index)[it.key()] = it.value();
        }
    }

    return i;
}


bool crawl(QString url, int level, QHash<QString, QString> *index) {
    int p = url.indexOf("#");
    if (p >= 0)
        url = url.left(p);

    if (!urlVisited.contains(url)) {
        urlVisited[url] = 1;
    } else {
        urlVisited[url] = urlVisited[url] + 1;
        return false;
    }
    QEventLoop loop;
    QNetworkAccessManager nam;
    QUrl u(url);
    QNetworkRequest req(u);
    out << "Requesting url: " << url << endl;
    QNetworkReply *reply = nam.get(req);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    if (reply->error() != QNetworkReply::NoError) {
        out << "An error occurred: " << reply->errorString() << endl;
    }

    QByteArray buffer = reply->readAll();
    QString pageContent = QString::fromUtf8(buffer);
    int count = parseWords(pageContent, url, index);
    out << "Number of words: " << count << endl;

    QThread::sleep(DELAY);

    if (level == MAXLEVEL) {
        return false;
    }

    QRegularExpression reA("<a\\s+href\\s*=\\s*\"([^\"]*)\"", QRegularExpression::CaseInsensitiveOption);

    QRegularExpressionMatchIterator i = reA.globalMatch(pageContent);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        if (match.hasMatch()) {
            QString link = match.captured(1);
            if (link.contains(QRegularExpression("\\.(jpg|jpeg|pdf|mp3|mp4|doc|zip|docx|rar)$"))) {
                out << "Unsupported type skipped: " << link << endl;
                continue;
            }

            if (link.contains(QRegularExpression("^http[s]?://"))) {
                if (link.contains(QRegularExpression(TARGET_SITE_RE))) {
                    out << "Processing " << link << endl;
                    crawl(link, level + 1, index);
                } else {
                    out << "Foreign url skipped: " << link << endl;
                }
            } else {
                if (link.startsWith("/")) {
                    out << "...Absolute url " << url << "..." << link << endl;
                } else {
                    int p = url.lastIndexOf("/");
                    if (p < 0) {
                        out << "Invalid url: " << url << endl;
                        continue;
                    }
                    QString newLink = url.left(p + 1) + link;
                    out << "NewLink: " << newLink << endl;
                    crawl(newLink, level + 1, index);
                }
            }
        }
    }
    return true;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QHash<QString, QString> index;

    crawl("http://stankin.ru", 0, &index);

    QString filename = "index.txt";
    QFile file(filename);
    if (!file.open(QIODevice::ReadWrite | QIODevice::Text)) {
        out << "Error" << endl;
        return 0;
    }
    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    QList<QString> keys = index.keys();

    std::sort(keys.begin(), keys.end());

    for (int j = 0; j < keys.count(); j++) {
        stream << keys[j] << " " << index[keys[j]] << endl;
    }
    out << "ok" << endl;
    file.close();

    return a.exec();
}
