#include "musicdownloadwidget.h"
#include "ui_musicdownloadwidget.h"
#include "musicuiobject.h"
#include "musicsettingmanager.h"
#include "musicnetworkthread.h"
#include "musicdownloadrecordconfigmanager.h"
#include "musicdatatagdownloadthread.h"
#include "musicmessagebox.h"
#include "musicdownloadqueryfactory.h"
#include "musicstringutils.h"

#include <QLabel>
#include <QFileDialog>

Q_DECLARE_METATYPE(MusicDownloadTableItemRole)

MusicDownloadTableItem::MusicDownloadTableItem(QWidget *parent)
    : QWidget(parent)
{
    m_information = new QLabel(this);
    m_icon = new QLabel(this);
    m_text = new QLabel(this);

    m_text->setGeometry(0, 0, 60, ROW_HEIGHT);
    m_icon->setGeometry(70, 0, 30, ROW_HEIGHT);
    m_information->setGeometry(170, 0, 150, ROW_HEIGHT);
}

MusicDownloadTableItem::~MusicDownloadTableItem()
{
    delete m_information;
    delete m_icon;
    delete m_text;
}

QString MusicDownloadTableItem::getClassName()
{
    return staticMetaObject.className();
}

void MusicDownloadTableItem::setIcon(const QString &name)
{
    m_icon->setPixmap(QPixmap(name).scaled(28, 18));
}

void MusicDownloadTableItem::setInformation(const QString &info)
{
    m_information->setText(info);
}

void MusicDownloadTableItem::setText(const QString &text)
{
    m_text->setText(text);
}



MusicDownloadTableWidget::MusicDownloadTableWidget(QWidget *parent)
    : MusicAbstractTableWidget(parent)
{
    setColumnCount(1);
    QHeaderView *headerview = horizontalHeader();
    headerview->resizeSection(0, 400);
    MusicUtils::Widget::setTransparent(this, 255);
}

MusicDownloadTableWidget::~MusicDownloadTableWidget()
{
    clearAllItems();
}

QString MusicDownloadTableWidget::getClassName()
{
    return staticMetaObject.className();
}

void MusicDownloadTableWidget::clearAllItems()
{
    qDeleteAll(m_items);
    m_items.clear();
    MusicAbstractTableWidget::clear();
    setColumnCount(1);
}

void MusicDownloadTableWidget::createItem(const MusicObject::MusicSongAttribute &attr, const QString &type, const QString &icon)
{
    int index = rowCount();
    setRowCount(index + 1);
    setRowHeight(index, ROW_HEIGHT);
    QTableWidgetItem *it = new QTableWidgetItem;
    MusicDownloadTableItemRole role(attr.m_bitrate, attr.m_format, attr.m_size);
    it->setData(TABLE_ITEM_ROLE, QVariant::fromValue<MusicDownloadTableItemRole>(role));
    setItem(index, 0,  it);

    MusicDownloadTableItem *item = new MusicDownloadTableItem(this);
    item->setIcon(icon);
    item->setInformation(QString("%1/%2KBPS/%3").arg(attr.m_size).arg(attr.m_bitrate).arg(attr.m_format.toUpper()));
    item->setText(type);
    m_items << item;

    setCellWidget(index, 0, item);
}

MusicDownloadTableItemRole MusicDownloadTableWidget::getCurrentItemRole() const
{
   int row = currentRow();
   if(row == -1)
   {
       return MusicDownloadTableItemRole();
   }

   return item(row, 0)->data(TABLE_ITEM_ROLE).value<MusicDownloadTableItemRole>();
}

void MusicDownloadTableWidget::listCellClicked(int row, int column)
{
    Q_UNUSED(row);
    Q_UNUSED(column);
}



MusicDownloadWidget::MusicDownloadWidget(QWidget *parent)
    : MusicAbstractMoveWidget(parent),
      m_ui(new Ui::MusicDownloadWidget)
{
    m_ui->setupUi(this);

    m_ui->topTitleCloseButton->setIcon(QIcon(":/functions/btn_close_hover"));
    m_ui->topTitleCloseButton->setStyleSheet(MusicUIObject::MToolButtonStyle04);
    m_ui->topTitleCloseButton->setCursor(QCursor(Qt::PointingHandCursor));
    m_ui->topTitleCloseButton->setToolTip(tr("Close"));

    setAttribute(Qt::WA_DeleteOnClose);

    m_ui->downloadPathEdit->setStyleSheet(MusicUIObject::MLineEditStyle01);
    m_ui->pathChangedButton->setStyleSheet(MusicUIObject::MPushButtonStyle03);
    m_ui->settingButton->setStyleSheet(MusicUIObject::MPushButtonStyle03);
    m_ui->downloadButton->setStyleSheet(MusicUIObject::MPushButtonStyle06);
#ifdef Q_OS_UNIX
    m_ui->pathChangedButton->setFocusPolicy(Qt::NoFocus);
    m_ui->settingButton->setFocusPolicy(Qt::NoFocus);
    m_ui->downloadButton->setFocusPolicy(Qt::NoFocus);
#endif

    m_downloadOffset = 0;
    m_downloadTotal = 0;
    m_querySingleInfo = false;
    m_downloadThread = M_DOWNLOAD_QUERY_PTR->getQueryThread(this);

    m_queryType = MusicDownLoadQueryThreadAbstract::MusicQuery;
    m_ui->loadingLabel->setType(MusicGifLabelWidget::Gif_Cicle_Blue);

    connect(m_ui->pathChangedButton, SIGNAL(clicked()), SLOT(downloadDirSelected()));
    connect(m_downloadThread, SIGNAL(downLoadDataChanged(QString)), SLOT(queryAllFinished()));
    connect(m_ui->topTitleCloseButton, SIGNAL(clicked()), SLOT(close()));
    connect(m_ui->downloadButton, SIGNAL(clicked()), SLOT(startToDownload()));
}

MusicDownloadWidget::~MusicDownloadWidget()
{
    delete m_ui;
    delete m_downloadThread;
}

QString MusicDownloadWidget::getClassName()
{
    return staticMetaObject.className();
}

void MusicDownloadWidget::initWidget()
{
    m_ui->loadingLabel->run(true);

    controlEnable(true);

    if(m_queryType == MusicDownLoadQueryThreadAbstract::MovieQuery)
    {
        m_ui->downloadPathEdit->setText(MOVIE_DIR_FULL);
    }
    else
    {
        m_ui->downloadPathEdit->setText(M_SETTING_PTR->value(MusicSettingManager::DownloadMusicPathDirChoiced).toString());
    }
}

void MusicDownloadWidget::controlEnable(bool enable)
{
    m_ui->topTitleCloseButton->setEnabled(enable);
    m_ui->downloadButton->setEnabled(enable);
    m_ui->pathChangedButton->setEnabled(enable);
    m_ui->settingButton->setEnabled(enable);
}

void MusicDownloadWidget::setSongName(const QString &name, MusicDownLoadQueryThreadAbstract::QueryType type)
{
    m_queryType = type;
    initWidget();

    m_ui->downloadName->setText(MusicUtils::Widget::elidedText(font(), name, Qt::ElideRight, 200));
    m_downloadThread->setQueryAllRecords(true);
    m_downloadThread->startToSearch(type, name);
}

void MusicDownloadWidget::setSongName(const MusicObject::MusicSongInformation &info, MusicDownLoadQueryThreadAbstract::QueryType type)
{
    m_queryType = type;
    m_singleSongInfo = info;
    m_querySingleInfo = true;

    initWidget();
    m_ui->downloadName->setText(MusicUtils::Widget::elidedText(font(),
                                QString("%1 - %2").arg(info.m_singerName).arg(info.m_songName), Qt::ElideRight, 200));

    if(type == MusicDownLoadQueryThreadAbstract::MusicQuery)
    {
        queryAllFinishedMusic(info.m_songAttrs);
    }
    else if(type == MusicDownLoadQueryThreadAbstract::MovieQuery)
    {
        queryAllFinishedMovie(info.m_songAttrs);
    }
}

void MusicDownloadWidget::show()
{
    setBackgroundPixmap(m_ui->background, size());
    return MusicAbstractMoveWidget::show();
}

void MusicDownloadWidget::queryAllFinished()
{
    if(!M_NETWORK_PTR->isOnline())
    {
        return;
    }

    m_ui->viewArea->clearAllItems();
    if(m_queryType == MusicDownLoadQueryThreadAbstract::MusicQuery)
    {
        queryAllFinishedMusic();
    }
    else if(m_queryType == MusicDownLoadQueryThreadAbstract::MovieQuery)
    {
        queryAllFinishedMovie();
    }
}

MusicObject::MusicSongInformation MusicDownloadWidget::getMatchMusicSongInformation()
{
    MusicObject::MusicSongInformations musicSongInfos(m_downloadThread->getMusicSongInfos());
    if(!musicSongInfos.isEmpty())
    {
        QString filename = m_downloadThread->getSearchedText();
        QString artistName = MusicUtils::String::artistName(filename);
        QString songName = MusicUtils::String::songName(filename);
        MusicObject::MusicSongInformation musicSongInfo;
        foreach(const MusicObject::MusicSongInformation &var, musicSongInfos)
        {
            if(var.m_singerName.contains(artistName, Qt::CaseInsensitive) &&
               var.m_songName.contains(songName, Qt::CaseInsensitive))
            {
                musicSongInfo = var;
                break;
            }
        }
        qSort(musicSongInfo.m_songAttrs);
        return musicSongInfo;
    }
    return MusicObject::MusicSongInformation();
}

void MusicDownloadWidget::queryAllFinishedMusic()
{
    MusicObject::MusicSongInformation musicSongInfo(getMatchMusicSongInformation());
    if(!musicSongInfo.m_songName.isEmpty() || !musicSongInfo.m_singerName.isEmpty())
    {
        queryAllFinishedMusic(musicSongInfo.m_songAttrs);
    }
    else
    {
        close();
        MusicMessageBox message(tr("No Resource found!"));
        message.exec();
    }
}

void MusicDownloadWidget::queryAllFinishedMusic(const MusicObject::MusicSongAttributes &attrs)
{
    MusicObject::MusicSongAttributes attributes = attrs;
    qSort(attributes);
    //to find out the min bitrate

    foreach(const MusicObject::MusicSongAttribute &attr, attributes)
    {
        if(attr.m_bitrate < MB_128)         ///st
        {
            m_ui->viewArea->createItem(attr, tr("ST"), QString(":/quality/lb_st_quality"));
        }
        else if(attr.m_bitrate == MB_128)   ///sd
        {
            m_ui->viewArea->createItem(attr, tr("SD"), QString(":/quality/lb_sd_quality"));
        }
        else if(attr.m_bitrate == MB_192)   ///hd
        {
            m_ui->viewArea->createItem(attr, tr("HQ"), QString(":/quality/lb_hd_quality"));
        }
        else if(attr.m_bitrate == MB_320)   ///sq
        {
            m_ui->viewArea->createItem(attr, tr("SQ"), QString(":/quality/lb_sq_quality"));
        }
        else if(attr.m_bitrate > MB_320)   ///cd
        {
            m_ui->viewArea->createItem(attr, tr("CD"), QString(":/quality/lb_cd_quality"));
        }
        else
        {
            continue;
        }
    }

    resizeWindow();
}

void MusicDownloadWidget::queryAllFinishedMovie()
{
    MusicObject::MusicSongInformation musicSongInfo(getMatchMusicSongInformation());
    if(!musicSongInfo.m_songName.isEmpty() || !musicSongInfo.m_singerName.isEmpty())
    {
        queryAllFinishedMovie(musicSongInfo.m_songAttrs);
    }
    else
    {
        close();
        MusicMessageBox message(tr("No Resource found!"));
        message.exec();
    }
}

void MusicDownloadWidget::queryAllFinishedMovie(const MusicObject::MusicSongAttributes &attrs)
{
    MusicObject::MusicSongAttributes attributes = attrs;
    qSort(attributes);
    //to find out the min bitrate

    foreach(const MusicObject::MusicSongAttribute &attr, attributes)
    {
        if(attr.m_bitrate <= MB_250)       ///st
        {
            m_ui->viewArea->createItem(attr, tr("ST"), QString(":/quality/lb_st_quality"));
        }
        else if(attr.m_bitrate == MB_500)  ///sd
        {
            m_ui->viewArea->createItem(attr, tr("SD"), QString(":/quality/lb_sd_quality"));
        }
        else if(attr.m_bitrate == MB_750)  ///hd
        {
            m_ui->viewArea->createItem(attr, tr("HD"), QString(":/quality/lb_hd_quality"));
        }
        else if(attr.m_bitrate >= MB_1000) ///sq
        {
            m_ui->viewArea->createItem(attr, tr("SQ"), QString(":/quality/lb_sq_quality"));
        }
        else
        {
            continue;
        }
    }

    resizeWindow();
}

void MusicDownloadWidget::resizeWindow()
{
    m_ui->loadingLabel->run(false);

    int delta = m_ui->viewArea->rowCount();
    delta = ((delta == 0) ? 0 : (delta - 1)*ROW_HEIGHT) - 2*ROW_HEIGHT;

    setFixedHeightWidget(this, delta);
    setFixedHeightWidget(m_ui->backgroundMask, delta);
    setFixedHeightWidget(m_ui->background, delta);
    setFixedHeightWidget(m_ui->viewArea, delta + 2*ROW_HEIGHT);

    setMoveWidget(m_ui->label2, delta);
    setMoveWidget(m_ui->downloadPathEdit, delta);
    setMoveWidget(m_ui->pathChangedButton, delta);
    setMoveWidget(m_ui->settingButton, delta);
    setMoveWidget(m_ui->downloadButton, delta);

    setBackgroundPixmap(m_ui->background, size());
}

void MusicDownloadWidget::setFixedHeightWidget(QWidget *w, int height)
{
    w->setFixedHeight(w->height() + height);
}

void MusicDownloadWidget::setMoveWidget(QWidget *w, int pos)
{
    QRect rect = w->geometry();
    w->move(rect.x(), rect.y() + pos);
}

void MusicDownloadWidget::downloadDirSelected()
{
    QFileDialog dialog;
    dialog.setFileMode(QFileDialog::Directory);
    dialog.setViewMode(QFileDialog::Detail);
    if(dialog.exec())
    {
        QString path;
        if(!(path = dialog.directory().absolutePath()).isEmpty())
        {
            if(m_queryType == MusicDownLoadQueryThreadAbstract::MusicQuery)
            {
                M_SETTING_PTR->setValue(MusicSettingManager::DownloadMusicPathDirChoiced, path + "/");
            }
            m_ui->downloadPathEdit->setText(path + "/");
        }
    }
}

void MusicDownloadWidget::startToDownload()
{
    m_downloadOffset = 0;
    m_downloadTotal = 0;

    hide(); ///hide download widget
    if(m_ui->viewArea->getCurrentItemRole().isEmpty())
    {
        MusicMessageBox message(tr("Please Select One Item First!"));
        message.exec();
        return;
    }

    if(m_queryType == MusicDownLoadQueryThreadAbstract::MusicQuery)
    {
        m_querySingleInfo ? startToDownloadMusic(m_singleSongInfo) : startToDownloadMusic();
    }
    else if(m_queryType == MusicDownLoadQueryThreadAbstract::MovieQuery)
    {
        m_querySingleInfo ? startToDownloadMovie(m_singleSongInfo) : startToDownloadMovie();
    }
    controlEnable(false);
}

void MusicDownloadWidget::dataDownloadFinished()
{
    if(++m_downloadOffset >= m_downloadTotal)
    {
        emit dataDownloadChanged();
        close();
    }
}

void MusicDownloadWidget::startToDownloadMusic()
{
    MusicObject::MusicSongInformation musicSongInfo(getMatchMusicSongInformation());
    if(!musicSongInfo.m_songName.isEmpty() || !musicSongInfo.m_singerName.isEmpty())
    {
        startToDownloadMusic(musicSongInfo);
    }
}

void MusicDownloadWidget::startToDownloadMusic(const MusicObject::MusicSongInformation &musicSongInfo)
{
    MusicDownloadTableItemRole role = m_ui->viewArea->getCurrentItemRole();
    MusicObject::MusicSongAttributes musicAttrs = musicSongInfo.m_songAttrs;
    foreach(const MusicObject::MusicSongAttribute &musicAttr, musicAttrs)
    {
        if(role.isEqual(MusicDownloadTableItemRole(musicAttr.m_bitrate, musicAttr.m_format, musicAttr.m_size)))
        {
            if(!M_NETWORK_PTR->isOnline())
            {
                return;
            }

            QString musicSong = musicSongInfo.m_singerName + " - " + musicSongInfo.m_songName;
            QString downloadPrefix = m_ui->downloadPathEdit->text().isEmpty() ? MUSIC_DIR_FULL : m_ui->downloadPathEdit->text();
            QString downloadName = QString("%1%2.%3").arg(downloadPrefix).arg(musicSong).arg(musicAttr.m_format);
            ////////////////////////////////////////////////
            MusicDownloadRecords records;
            MusicDownloadRecordConfigManager down(MusicDownloadRecordConfigManager::Normal, this);
            if(!down.readDownloadXMLConfig())
            {
                return;
            }

            down.readDownloadConfig( records );
            MusicDownloadRecord record;
            record.m_name = musicSong;
            record.m_path = QFileInfo(downloadName).absoluteFilePath();
            record.m_size = musicAttr.m_size;
            record.m_time = "-1";
            records << record;
            down.writeDownloadConfig( records );
            ////////////////////////////////////////////////
            QFile file(downloadName);
            if(file.exists())
            {
                for(int i=1; i<99; ++i)
                {
                    if(!QFile::exists(downloadName))
                    {
                        break;
                    }
                    if(i != 1)
                    {
                        musicSong.chop(3);
                    }
                    musicSong += QString("(%1)").arg(i);
                    downloadName = QString("%1%2.%3").arg(downloadPrefix).arg(musicSong).arg(musicAttr.m_format);
                }
            }
            ////////////////////////////////////////////////
            m_downloadTotal = 1;
            MusicDataTagDownloadThread *downSong = new MusicDataTagDownloadThread( musicAttr.m_url, downloadName,
                                                                                   MusicDownLoadThreadAbstract::DownloadMusic, this);
            connect(downSong, SIGNAL(downLoadDataChanged(QString)), SLOT(dataDownloadFinished()));
            downSong->setTags(musicSongInfo.m_smallPicUrl, musicSongInfo.m_songName, musicSongInfo.m_singerName, musicSongInfo.m_albumName);
            downSong->startToDownload();
            break;
        }
    }
}

void MusicDownloadWidget::startToDownloadMovie()
{
    MusicObject::MusicSongInformation musicSongInfo(getMatchMusicSongInformation());
    if(!musicSongInfo.m_songName.isEmpty() || !musicSongInfo.m_singerName.isEmpty())
    {
        startToDownloadMovie(musicSongInfo);
    }
}

void MusicDownloadWidget::startToDownloadMovie(const MusicObject::MusicSongInformation &musicSongInfo)
{
    MusicDownloadTableItemRole role = m_ui->viewArea->getCurrentItemRole();
    MusicObject::MusicSongAttributes musicAttrs = musicSongInfo.m_songAttrs;
    foreach(const MusicObject::MusicSongAttribute &musicAttr, musicAttrs)
    {
        if(role.isEqual(MusicDownloadTableItemRole(musicAttr.m_bitrate, musicAttr.m_format, musicAttr.m_size)))
        {
            if(!M_NETWORK_PTR->isOnline())
            {
                return;
            }

            QString musicSong = musicSongInfo.m_singerName + " - " + musicSongInfo.m_songName;
            QString downloadPrefix = m_ui->downloadPathEdit->text().isEmpty() ? MOVIE_DIR_FULL : m_ui->downloadPathEdit->text();
            ////////////////////////////////////////////////
            QStringList urls = musicAttr.m_multiParts ? musicAttr.m_url.split(STRING_SPLITER) : QStringList(musicAttr.m_url);
            m_downloadTotal = urls.count();
            for(int ul=0; ul<m_downloadTotal; ++ul)
            {
                ////////////////////////////////////////////////
                QString downloadName = (urls.count() == 1) ? QString("%1%2.%3").arg(downloadPrefix).arg(musicSong).arg(musicAttr.m_format)
                                            : QString("%1%2.part%3.%4").arg(downloadPrefix).arg(musicSong).arg(ul+1).arg(musicAttr.m_format);
                QFile file(downloadName);
                if(file.exists())
                {
                    for(int i=1; i<99; ++i)
                    {
                        if(!QFile::exists(downloadName))
                        {
                            break;
                        }
                        if(i != 1)
                        {
                            musicSong.chop(3);
                        }
                        musicSong += QString("(%1)").arg(i);
                        downloadName = (urls.count() == 1) ? QString("%1%2.%3").arg(downloadPrefix).arg(musicSong).arg(musicAttr.m_format)
                                          : QString("%1%2.part%3.%4").arg(downloadPrefix).arg(musicSong).arg(ul+1).arg(musicAttr.m_format);
                    }
                }
                ////////////////////////////////////////////////
                MusicDataDownloadThread *download = new MusicDataDownloadThread(urls[ul], downloadName,
                                                                                MusicDownLoadThreadAbstract::DownloadVideo, this);
                connect(download, SIGNAL(downLoadDataChanged(QString)), SLOT(dataDownloadFinished()));
                download->startToDownload();
            }
            break;
        }
    }
}
