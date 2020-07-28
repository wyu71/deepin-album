#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "application.h"
#include "mainwindow.h"
#include "allpicview.h"

#include <QTestEventList>

TEST(allpicview, ini)
{
    QThreadPool::globalInstance()->waitForDone();
    MainWindow *w = dApp->getMainWindow();
    w->showEvent(nullptr);
    QTestEventList event;
    event.addMouseClick(Qt::MouseButton::LeftButton);
    event.simulate(w->getButG()->button(0));
    event.clear();
    AllPicView *a = w->m_pAllPicView;
    for (int i = 0; i < 10; i++) {
        a->m_pStatusBar->m_pSlider->setValue(i);
    }
    a->m_pStatusBar->m_pSlider->setValue(1);
}

TEST(allpicview, open)
{
    MainWindow *w = dApp->getMainWindow();
    QThreadPool::globalInstance()->waitForDone();
    AllPicView *a = w->m_pAllPicView;
    QStringList testPathlist = ImageEngineApi::instance()->get_AllImagePath();

    if (!testPathlist.isEmpty()) {
        emit a->getThumbnailListView()->menuOpenImage(testPathlist.first(), testPathlist, false);
    }
    ASSERT_TRUE(!testPathlist.isEmpty());
}
