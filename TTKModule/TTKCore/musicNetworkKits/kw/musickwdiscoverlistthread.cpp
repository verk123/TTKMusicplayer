#include "musickwdiscoverlistthread.h"
#include "musicdownloadkwinterface.h"
#///QJson import
#include "qjson/parser.h"

MusicKWDiscoverListThread::MusicKWDiscoverListThread(QObject *parent)
    : MusicDownLoadDiscoverListThread(parent)
{

}

QString MusicKWDiscoverListThread::getClassName()
{
    return staticMetaObject.className();
}

void MusicKWDiscoverListThread::startToSearch()
{
    if(!m_manager)
    {
        return;
    }

    M_LOGGER_INFO(QString("%1 startToSearch").arg(getClassName()));
    m_toplistInfo.clear();
    QUrl musicUrl = MusicUtils::Algorithm::mdII(KW_SONG_TOPLIST_URL, false).arg(16);
    deleteAll();
    m_interrupt = true;

    QNetworkRequest request;
    request.setUrl(musicUrl);
    request.setRawHeader("Content-Type", "application/x-www-form-urlencoded");
    request.setRawHeader("User-Agent", MusicUtils::Algorithm::mdII(KW_UA_URL_1, ALG_UA_KEY, false).toUtf8());
    setSslConfiguration(&request);

    m_reply = m_manager->get(request);
    connect(m_reply, SIGNAL(finished()), SLOT(downLoadFinished()));
    connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(replyError(QNetworkReply::NetworkError)));
}

void MusicKWDiscoverListThread::downLoadFinished()
{
    if(!m_reply)
    {
        deleteAll();
        return;
    }

    M_LOGGER_INFO(QString("%1 downLoadFinished").arg(getClassName()));
    m_interrupt = false;

    if(m_reply->error() == QNetworkReply::NoError)
    {
        QByteArray bytes = m_reply->readAll();

        QJson::Parser parser;
        bool ok;
        QVariant data = parser.parse(bytes, &ok);
        if(ok)
        {
            QVariantMap value = data.toMap();
            if(value.contains("musiclist"))
            {
                QVariantList datas = value["musiclist"].toList();
                int where = datas.count();
                where = (where > 0) ? qrand()%where : 0;

                int counter = 0;
                foreach(const QVariant &var, datas)
                {
                    if(m_interrupt) return;

                    if((where != counter++) || var.isNull())
                    {
                        continue;
                    }

                    QVariantMap value = var.toMap();
                    m_toplistInfo = QString("%1 - %2").arg(value["artist"].toString())
                                                      .arg(value["name"].toString());
                }
            }
        }
    }

    emit downLoadDataChanged(m_toplistInfo);
    deleteAll();
    M_LOGGER_INFO(QString("%1 searchToplistInfoFinished deleteAll").arg(getClassName()));
}
