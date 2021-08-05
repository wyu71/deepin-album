/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     ZhangYong <zhangyong@uniontech.com>
 *
 * Maintainer: ZhangYong <ZhangYong@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "timelineview.h"
#include "utils/baseutils.h"
#include "utils/imageutils.h"
#include "imageengine/imageengineapi.h"
#include "mainwindow.h"
#include "ac-desktop-define.h"

#include <QScrollBar>
#include <QScroller>
#include <QMimeData>
#include <QGraphicsOpacityEffect>
#include <QDesktopWidget>

#include <DPushButton>
#include <DTableView>
#include <dgiovolumemanager.h>
#include <dgiofile.h>
#include <dgiofileinfo.h>
#include <dgiovolume.h>

#include "batchoperatewidget.h"
#include "noresultwidget.h"

namespace  {
const int VIEW_IMPORT = 0;
const int VIEW_TIMELINE = 1;
const int VIEW_SEARCH = 2;
const int TITLEHEIGHT = 0;
const int TIMELINE_TITLEHEIGHT = 36;
const int SUSPENSION_WIDGET_HEIGHT = 87;//悬浮控件高度
} //namespace

TimeLineView::TimeLineView()
    : m_mainLayout(nullptr)
    , allnum(0)
    , m_oe(nullptr), m_oet(nullptr)
    , fatherwidget(nullptr), m_pStackedWidget(nullptr)
    , m_pStatusBar(nullptr), pSearchView(nullptr), pImportView(nullptr), pTimeLineViewWidget(nullptr)
    , m_pwidget(nullptr), m_selPicNum(0), m_spinner(nullptr)
{
    setAcceptDrops(true);
    QVBoxLayout *pMainBoxLayout = new QVBoxLayout(this);
    pMainBoxLayout->setMargin(0);
    this->setLayout(pMainBoxLayout);

    fatherwidget = new QWidget(this);
    pMainBoxLayout->addWidget(fatherwidget);
    m_oe = new QGraphicsOpacityEffect(this);
    m_oet = new QGraphicsOpacityEffect(this);
    m_oe->setOpacity(0.5);
    m_oet->setOpacity(0.75);

    m_pStackedWidget = new QStackedWidget(this);
    pTimeLineViewWidget = new QWidget();
    pImportView = new ImportView();
    pSearchView = new SearchView();
    m_pStackedWidget->addWidget(pImportView);
    m_pStackedWidget->addWidget(pTimeLineViewWidget);
    m_pStackedWidget->addWidget(pSearchView);

    m_pStatusBar = new StatusBar(this);
    m_pStatusBar->raise();
    m_pStatusBar->setFixedWidth(this->width());
    m_pStatusBar->move(0, this->height() - m_pStatusBar->height());
    QVBoxLayout *pVBoxLayout = new QVBoxLayout();
    pVBoxLayout->setContentsMargins(2, 0, 0, 0);
    pVBoxLayout->addWidget(m_pStackedWidget);
    fatherwidget->setLayout(pVBoxLayout);

    initTimeLineViewWidget();
    initConnections();
    m_pwidget = new QWidget(this);
    m_pwidget->setFocusPolicy(Qt::NoFocus);
    m_pwidget->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_spinner = new DSpinner(this);
    m_spinner->setFixedSize(40, 40);
    m_spinner->hide();
}

void TimeLineView::initConnections()
{
    qRegisterMetaType<DBImgInfoList>("DBImgInfoList &");
    connect(dApp->signalM, &SignalManager::sigLoadOnePhoto, this, &TimeLineView::clearAndStartLayout);
    connect(dApp->signalM, &SignalManager::imagesInserted, this, &TimeLineView::clearAndStartLayout);
    connect(dApp, &Application::sigFinishLoad, this, &TimeLineView::onFinishLoad);
    connect(dApp->signalM, &SignalManager::sigUpdateImageLoader, this, &TimeLineView::updataLayout);
    connect(m_pStatusBar->m_pSlider, &DSlider::valueChanged, dApp->signalM, &SignalManager::sigMainwindowSliderValueChg);
    connect(pSearchView->m_pThumbnailListView, &ThumbnailListView::clicked, this, &TimeLineView::updatePicNum);
    connect(DApplicationHelper::instance(), &DApplicationHelper::themeTypeChanged, this, &TimeLineView::themeChangeSlot);
    connect(pImportView->m_pImportBtn, &DPushButton::clicked, this, &TimeLineView::onImportViewImportBtnClicked);
    connect(dApp->signalM, &SignalManager::sigImportFailedToView, this, &TimeLineView::onImportFailedToView);
    connect(dApp->signalM, &SignalManager::sigShortcutKeyDelete, this, &TimeLineView::onKeyDelete);
    // 重复导入图片选中
    connect(dApp->signalM, &SignalManager::RepeatImportingTheSamePhotos, this, &TimeLineView::onRepeatImportingTheSamePhotos);
}

void TimeLineView::themeChangeSlot(DGuiApplicationHelper::ColorType themeType)
{
    DPalette pa1 = DApplicationHelper::instance()->palette(m_dateNumItemWidget);
    pa1.setBrush(DPalette::Background, pa1.color(DPalette::Base));
    m_dateNumItemWidget->setForegroundRole(DPalette::Background);
    m_dateNumItemWidget->setPalette(pa1);
    DPalette pa = DApplicationHelper::instance()->palette(m_dateLabel);
    pa.setBrush(DPalette::Text, pa.color(DPalette::ToolTipText));
    m_dateLabel->setForegroundRole(DPalette::Text);
    m_dateLabel->setPalette(pa);

    DPalette pal1 = DApplicationHelper::instance()->palette(m_numLabel);
    QColor color_BT1 = pal1.color(DPalette::BrightText);
    if (themeType == DGuiApplicationHelper::LightType) {
        color_BT1.setAlphaF(0.5);
        pal1.setBrush(DPalette::Text, color_BT1);
        m_numLabel->setForegroundRole(DPalette::Text);
        m_numLabel->setPalette(pal1);
    } else if (themeType == DGuiApplicationHelper::DarkType) {
        color_BT1.setAlphaF(0.75);
        pal1.setBrush(DPalette::Text, color_BT1);
        m_numLabel->setForegroundRole(DPalette::Text);
        m_numLabel->setPalette(pal1);
    }
}

ThumbnailListView *TimeLineView::getThumbnailListView()
{
    return m_timeLineThumbnailListView;
}

void TimeLineView::clearAllSelection()
{
    m_timeLineThumbnailListView->clearSelection();
}

void TimeLineView::updataLayout(QStringList updatePathList)
{
    Q_UNUSED(updatePathList)
    m_spinner->hide();
    m_spinner->stop();
}

void TimeLineView::initTimeLineViewWidget()
{
    m_mainLayout = new QVBoxLayout();
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    pTimeLineViewWidget->setLayout(m_mainLayout);

    DPalette palcolor = DApplicationHelper::instance()->palette(pTimeLineViewWidget);
    palcolor.setBrush(DPalette::Base, palcolor.color(DPalette::Window));
    pTimeLineViewWidget->setPalette(palcolor);

    m_timeLineThumbnailListView = new ThumbnailListView(ThumbnailDelegate::TimeLineViewType, COMMON_STR_VIEW_TIMELINE, pTimeLineViewWidget);
    m_timeLineThumbnailListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_timeLineThumbnailListView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_timeLineThumbnailListView->setContentsMargins(0, 0, 0, 0);
    m_timeLineThumbnailListView->setFrameShape(DTableView::NoFrame);
    m_mainLayout->addWidget(m_timeLineThumbnailListView);

    //初始化筛选无结果窗口
    m_noResultWidget = new NoResultWidget(pTimeLineViewWidget);
    m_mainLayout->addWidget(m_noResultWidget);
    m_noResultWidget->setVisible(false);

    connect(m_timeLineThumbnailListView, &ThumbnailListView::sigShowEvent, this, &TimeLineView::clearAndStartLayout);
    //滑动列表，刷新上方悬浮标题
    connect(m_timeLineThumbnailListView, &ThumbnailListView::sigTimeLineDataAndNum, this, &TimeLineView::slotTimeLineDataAndNum);
    //打开图片
    connect(m_timeLineThumbnailListView, &ThumbnailListView::openImage, this, &TimeLineView::onOpenImage);
    //幻灯片播放
    connect(m_timeLineThumbnailListView, &ThumbnailListView::sigSlideShow, this, &TimeLineView::onSlideShow);
    //筛选显示，当先列表中内容为无结果
    connect(m_timeLineThumbnailListView, &ThumbnailListView::sigNoPicOrNoVideo, this, &TimeLineView::slotNoPicOrNoVideo);

    connect(m_timeLineThumbnailListView, &ThumbnailListView::sigMouseMove, this, &TimeLineView::updatePicNum);
    connect(m_timeLineThumbnailListView, &ThumbnailListView::sigMoveToTrash, this, &TimeLineView::onKeyDelete);//跳转

    //添加悬浮title
    m_dateNumItemWidget = new QWidget(pTimeLineViewWidget);
    m_dateNumItemWidget->setPalette(palcolor);
    QVBoxLayout *titleViewLayout = new QVBoxLayout();
    titleViewLayout->setContentsMargins(18, 0, 0, 0);
    m_dateNumItemWidget->setLayout(titleViewLayout);

    //时间线
    QHBoxLayout *hDateLayout = new QHBoxLayout();
    m_dateLabel = new DLabel();
    hDateLayout->addWidget(m_dateLabel);
    DFontSizeManager::instance()->bind(m_dateLabel, DFontSizeManager::T3, QFont::Medium);
    QFont ft3 = DFontSizeManager::instance()->get(DFontSizeManager::T3);
    ft3.setFamily("SourceHanSansSC");
    ft3.setWeight(QFont::DemiBold);
    DPalette color = DApplicationHelper::instance()->palette(m_dateLabel);
    color.setBrush(DPalette::Text, color.color(DPalette::ToolTipText));

    //bug76892藏语占用更大高度
    if (QLocale::system().language() == QLocale::Tibetan) {
        m_dateLabel->setFixedHeight(TIMELINE_TITLEHEIGHT + 25);
    } else {
        m_dateLabel->setFixedHeight(TIMELINE_TITLEHEIGHT);
    }
    m_dateLabel->setFont(ft3);
    m_dateLabel->setForegroundRole(DPalette::Text);
    m_dateLabel->setPalette(color);

    //右侧批量操作控件
    m_batchOperateWidget = new BatchOperateWidget(m_timeLineThumbnailListView, BatchOperateWidget::NullType, this);
    //进入批量状态
    connect(m_batchOperateWidget, &BatchOperateWidget::signalBatchSelectChanged, this, &TimeLineView::slotBatchSelectChanged);
    hDateLayout->addStretch(1);
    hDateLayout->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    hDateLayout->setContentsMargins(0, 0, 19, 0);
    hDateLayout->addWidget(m_batchOperateWidget);


    //时间线下的计数
    QHBoxLayout *hNumLayout = new QHBoxLayout();
    m_numLabel = new DLabel();
    hNumLayout->addWidget(m_numLabel);
    if (QLocale::system().language() == QLocale::Uighur) {
        m_numLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    }
    DFontSizeManager::instance()->bind(m_numLabel, DFontSizeManager::T6, QFont::Medium);
    QFont ft6 = DFontSizeManager::instance()->get(DFontSizeManager::T6);
    ft6.setFamily("SourceHanSansSC");
    ft6.setWeight(QFont::Medium);
    DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
    DPalette pal = DApplicationHelper::instance()->palette(m_numLabel);
    QColor color_BT = pal.color(DPalette::BrightText);
    if (themeType == DGuiApplicationHelper::LightType) {
        color_BT.setAlphaF(0.5);
        pal.setBrush(DPalette::Text, color_BT);
        m_numLabel->setForegroundRole(DPalette::Text);
        m_numLabel->setPalette(pal);
    } else if (themeType == DGuiApplicationHelper::DarkType) {
        color_BT.setAlphaF(0.75);
        pal.setBrush(DPalette::Text, color_BT);
        m_numLabel->setForegroundRole(DPalette::Text);
        m_numLabel->setPalette(pal);
    }

    m_numLabel->setFixedHeight(TIMELINE_TITLEHEIGHT);
    m_numLabel->setFont(ft6);
    m_numLabel->setForegroundRole(DPalette::Text);
    m_numLabel->setPalette(pal);

    titleViewLayout->addLayout(hDateLayout);
    titleViewLayout->addLayout(hNumLayout);
    titleViewLayout->addStretch(100);

    //右侧选择文字
    m_suspensionChose = new DCommandLinkButton(QObject::tr("Select"));
    m_suspensionChose->setFocusPolicy(Qt::NoFocus);
    AC_SET_OBJECT_NAME(m_suspensionChose, Time_Line_Choose_Button);
    AC_SET_ACCESSIBLE_NAME(m_suspensionChose, Time_Line_Choose_Button);
    DFontSizeManager::instance()->bind(m_suspensionChose, DFontSizeManager::T5);
    m_suspensionChose->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T5));
    m_suspensionChose->setFixedHeight(TIMELINE_TITLEHEIGHT);
    m_suspensionChose->resize(36, 27);
    m_suspensionChose->setVisible(false);

    hNumLayout->addStretch(1);
    hNumLayout->addSpacerItem(new QSpacerItem(0, 27));
    hNumLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    hNumLayout->setContentsMargins(0, 0, 19, 0);
    hNumLayout->addWidget(m_suspensionChose);
    connect(m_suspensionChose, &DCommandLinkButton::clicked, this, &TimeLineView::on_DCommandLinkButton);

    DPalette ppal_light = DApplicationHelper::instance()->palette(m_dateNumItemWidget);
    ppal_light.setBrush(DPalette::Background, ppal_light.color(DPalette::Base));
    QGraphicsOpacityEffect *opacityEffect_light = new QGraphicsOpacityEffect;
    opacityEffect_light->setOpacity(0.95);
    m_dateNumItemWidget->setPalette(ppal_light);
    m_dateNumItemWidget->setGraphicsEffect(opacityEffect_light);
    m_dateNumItemWidget->setAutoFillBackground(true);
    m_dateNumItemWidget->setContentsMargins(0, 0, 0, 0);
    m_dateNumItemWidget->setGeometry(0, 0, this->width() - 15, SUSPENSION_WIDGET_HEIGHT);
}

void TimeLineView::updateStackedWidget()
{
    if (0 < DBManager::instance()->getImgsCount()) {
        m_pStackedWidget->setCurrentIndex(VIEW_TIMELINE);
    } else {
        m_pStackedWidget->setCurrentIndex(VIEW_IMPORT);
    }
}

void TimeLineView::clearAndStartLayout()
{
    //由于绘制需要使用listview的宽度，但是加载的时候listview还没有显示出来，宽度是不对的，所以在显示出来后用信号通知加载，记载完成后断开信号，
    //后面的listview就有了正确的宽度，该信号槽就不需要再连接
    disconnect(m_timeLineThumbnailListView, &ThumbnailListView::sigShowEvent, this, &TimeLineView::clearAndStartLayout);
    qDebug() << "------" << __FUNCTION__ << "";
    m_spinner->hide();
    m_spinner->stop();
    //获取所有时间线
    m_timelines = DBManager::instance()->getAllTimelines();
    updateStackedWidget();
    addTimelineLayout();
}

void TimeLineView::slotTimeLineDataAndNum(QString data, QString num, QString text)
{
    if (!data.isEmpty()) {
        m_dateLabel->setText(data);
    }
    if (!num.isEmpty()) {
        m_numLabel->setText(num);
    }
    m_suspensionChose->setText(text);
}

void TimeLineView::onFinishLoad()
{
    //todo
//    m_mainListWidget->update();
}

void TimeLineView::onImportViewImportBtnClicked()
{
    emit dApp->signalM->startImprot();
    pImportView->onImprotBtnClicked();
}

void TimeLineView::onImportFailedToView()
{
    if (isVisible()) {
        m_spinner->hide();
        m_spinner->stop();
        updateStackedWidget();
    }
}

void TimeLineView::onRepeatImportingTheSamePhotos(QStringList importPaths, QStringList duplicatePaths, const QString &albumName)
{
    Q_UNUSED(importPaths)
    // 导入的照片重复照片提示
    if (duplicatePaths.size() > 0 && albumName.length() < 1 && dApp->getMainWindow()->getCurrentViewType() == 1) {
        m_timeLineThumbnailListView->selectDuplicatePhotos(duplicatePaths);
    }
}

void TimeLineView::onOpenImage(int row, const QString &path, bool bFullScreen)
{
    SignalManager::ViewInfo info;
    info.album = "";
    info.lastPanel = nullptr;
    info.fullScreen = bFullScreen;
    auto imagelist = m_timeLineThumbnailListView->getFileList(row);
    if (imagelist.size() > 0) {
        info.paths << imagelist;
        info.path = path;
    } else {
        info.paths.clear();
    }
    info.itemInfos = m_timeLineThumbnailListView->getAllFileInfo(row);
    info.viewType = utils::common::VIEW_TIMELINE_SRN;
    info.viewMainWindowID = VIEW_MAINWINDOW_TIMELINE;
    emit dApp->signalM->viewImage(info);
    emit dApp->signalM->showImageView(VIEW_MAINWINDOW_TIMELINE);
}

void TimeLineView::onSlideShow(QString path)
{
    SignalManager::ViewInfo info;
    info.album = "";
    info.lastPanel = nullptr;
    auto photolist = m_timeLineThumbnailListView->selectedPaths();
    if (photolist.size() > 1) {
        //如果选中数目大于1，则幻灯片播放选中项
        info.paths = photolist;
        info.path = photolist.at(0);
    } else {
        //如果选中项只有一项，则幻灯片播放全部
        info.paths = m_timeLineThumbnailListView->getFileList(m_timeLineThumbnailListView->getRow(path));
        info.path = path;
    }
    info.fullScreen = true;
    info.slideShow = true;
    info.viewType = utils::common::VIEW_TIMELINE_SRN;
    info.viewMainWindowID = VIEW_MAINWINDOW_TIMELINE;
    emit dApp->signalM->startSlideShow(info);
    emit dApp->signalM->showSlidePanel(VIEW_MAINWINDOW_TIMELINE);
}

void TimeLineView::slotBatchSelectChanged(bool isBatchSelect)
{
    m_suspensionChose->setVisible(isBatchSelect);
}

void TimeLineView:: addTimelineLayout()
{
    m_timeLineThumbnailListView->clearAll();
    for (int timelineIndex = 0; timelineIndex < m_timelines.size(); timelineIndex++) {
        //获取当前时间照片
        DBImgInfoList ImgInfoList = DBManager::instance()->getInfosByTimeline(m_timelines.at(timelineIndex));

        //加时间线标题
        QString data, num;
        QStringList datelist = m_timelines.at(timelineIndex).split(".");
        if (datelist.count() > 2) {
            data = QString(QObject::tr("%1/%2/%3")).arg(datelist[0]).arg(datelist[1]).arg(datelist[2]);
        }
        num = QString(QObject::tr("%1 photo(s)")).arg(ImgInfoList.size());

        if (timelineIndex == 0) {
            m_dateLabel->setText(data);
            m_numLabel->setText(num);
            //加空白栏
            m_timeLineThumbnailListView->insertBlankOrTitleItem(ItemTypeBlank, data, num, SUSPENSION_WIDGET_HEIGHT);
        } else {
            //加时间线标题
            m_timeLineThumbnailListView->insertBlankOrTitleItem(ItemTypeTimeLineTitle, data, num, 90);
        }
        //加当前时间下的图片
        m_timeLineThumbnailListView->insertThumbnailByImgInfos(ImgInfoList);
    }
}

void TimeLineView::on_AddLabel(QString date, QString num)
{
    if ((nullptr != m_dateNumItemWidget)) {
        QList<QLabel *> labelList = m_dateNumItemWidget->findChildren<QLabel *>();
        labelList[0]->setText(date);
        labelList[1]->setText(num);
        m_dateNumItemWidget->setVisible(true);
        m_dateNumItemWidget->move(0, TITLEHEIGHT);
    }
}

void TimeLineView::on_DCommandLinkButton()
{
    bool isSelect = false;
    if (QObject::tr("Select") == m_suspensionChose->text()) {
        m_suspensionChose->setText(QObject::tr("Unselect"));
        isSelect = true;
    } else {
        m_suspensionChose->setText(QObject::tr("Select"));
    }
    QString date_str = m_dateLabel->text();
    //选中当前时间内的所有图片
    m_timeLineThumbnailListView->timeLimeFloatBtnClicked(date_str, isSelect);
}

void TimeLineView::resizeEvent(QResizeEvent *ev)
{
    Q_UNUSED(ev);
    m_spinner->move(width() / 2 - 20, (height() - 50) / 2 - 20);
    m_dateNumItemWidget->setGeometry(0, 0, width() - 15, SUSPENSION_WIDGET_HEIGHT);
    m_pwidget->setFixedSize(this->width(), this->height() - 23);
    m_pwidget->move(0, 0);
    m_pStatusBar->setFixedWidth(this->width());
    m_pStatusBar->move(0, this->height() - m_pStatusBar->height());
}

void TimeLineView::dragEnterEvent(QDragEnterEvent *e)
{
    const QMimeData *mimeData = e->mimeData();
    if (!utils::base::checkMimeData(mimeData)) {
        return;
    }
    e->setDropAction(Qt::CopyAction);
    e->accept();
}

void TimeLineView::dropEvent(QDropEvent *event)
{
    QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty()) {
        return;
    }
    ImageEngineApi::instance()->ImportImagesFromUrlList(urls, nullptr, this);
    event->accept();
}

void TimeLineView::dragMoveEvent(QDragMoveEvent *event)
{
    event->accept();
}

void TimeLineView::dragLeaveEvent(QDragLeaveEvent *e)
{
    Q_UNUSED(e);
}

void TimeLineView::mousePressEvent(QMouseEvent *e)
{
    if (QApplication::keyboardModifiers() != Qt::ControlModifier && e->button() == Qt::LeftButton) {
        m_timeLineThumbnailListView->clearSelection();
        updatePicNum();
        updateChoseText();
    }
    DWidget::mousePressEvent(e);
}

void TimeLineView::slotNoPicOrNoVideo(bool isNoResult)
{
    qDebug() << __FUNCTION__ << "---" << isNoResult;
    m_noResultWidget->setVisible(isNoResult);
    m_timeLineThumbnailListView->setVisible(!isNoResult);
    if (isNoResult) {
        m_pStatusBar->m_pAllPicNumLabel->setText(QObject::tr("No results"));
    } else {
        updatePicNum();
    }
    m_suspensionChose->setVisible(!isNoResult);
    m_dateLabel->setVisible(!isNoResult);
    m_numLabel->setVisible(!isNoResult);
}

void TimeLineView::updatePicNum()
{
    QString str = QObject::tr("%1 photo(s) selected");

    if (VIEW_SEARCH == m_pStackedWidget->currentIndex()) {
        QStringList paths = pSearchView->m_pThumbnailListView->selectedPaths();
        m_selPicNum = paths.length();
        m_pStatusBar->m_pAllPicNumLabel->setText(str.arg(m_selPicNum));
    } else {
        allnum = m_timeLineThumbnailListView->selectedPaths().size();

        if (0 == allnum) {
            restorePicNum();
        } else {
            QString str1 = QObject::tr("%1 photo(s) selected");
            m_pStatusBar->m_pAllPicNumLabel->setText(str1.arg(allnum));
        }
    }
}

void TimeLineView::updateChoseText()
{
#ifdef tablet_PC
    return;
#endif
}

void TimeLineView::restorePicNum()
{
    QString str = QObject::tr("%1 photo(s)");
    int selPicNum = 0;

    if (VIEW_TIMELINE == m_pStackedWidget->currentIndex()) {
        selPicNum = DBManager::instance()->getImgsCount();

    } else if (VIEW_SEARCH == m_pStackedWidget->currentIndex()) {
        selPicNum = pSearchView->m_searchPicNum;
    }

    m_pStatusBar->m_pAllPicNumLabel->setText(str.arg(QString::number(selPicNum)));
}

void TimeLineView::onKeyDelete()
{
    if (!isVisible()) return;
    if (VIEW_SEARCH == m_pStackedWidget->currentIndex()) return;

    QStringList paths;
    paths.clear();

    //获取当前所有选中的
    paths = m_timeLineThumbnailListView->selectedPaths();
    if (0 >= paths.length()) {
        return;
    }

    if (m_timeLineThumbnailListView->isAllSelected()) {
        m_pStackedWidget->setCurrentIndex(VIEW_IMPORT);
    }

    ImageEngineApi::instance()->moveImagesToTrash(paths);
}
