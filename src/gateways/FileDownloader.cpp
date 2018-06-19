//
// Created by alexis on 6/13/18.
//

#include "FileDownloader.h"

FileDownloader::FileDownloader(const QString &url, QString target, QObject *parent)
        : QObject(parent), url(url), errored(false), working(false), manager(new QNetworkAccessManager(this)),
          file(nullptr), target(target), reply(nullptr) {
    connect(manager, &QNetworkAccessManager::finished, this, &FileDownloader::handleDownloadFinished);
}

bool FileDownloader::isFailed() const {
    return errored;
}

const QString &FileDownloader::getUrl() const {
    return url;
}

const QString &FileDownloader::getTarget() const {
    return target;
}

void FileDownloader::start() {
    if (working)
        throw std::runtime_error("Download already running.");

    working = true;

    file.setFileName(target);
    if (!file.open(QIODevice::WriteOnly))
        throw std::runtime_error("Unable to open file: " + target.toStdString());


    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    reply = manager->get(request);
    connect(reply, &QIODevice::readyRead, this, &FileDownloader::handleReadyRead);

    working = true;
    errored = false;
}

void FileDownloader::handleDownloadFinished(QNetworkReply *reply) {
    working = false;
    errored = reply->error() != QNetworkReply::NoError;

    headers["last-modified"] = reply->header(QNetworkRequest::LastModifiedHeader);

    if (reply->operation() == QNetworkAccessManager::GetOperation) {
        if (errored)
            file.remove();
        else
            handleReadyRead();

        reply->deleteLater();
        file.close();
    }

    emit finished();
}

void FileDownloader::handleReadyRead() {
    const auto data = reply->readAll();
    file.write(data);
}

const QVariantMap &FileDownloader::getHeaders() const {
    return headers;
}

void FileDownloader::queryLastModificationDate() {
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    manager->head(request);

    working = true;
    errored = false;
}

