#include "graphwidget.h"

#include <QDateTime>
#include <QPainter>
#include <QTimer>
#include <QDebug>
#include <qdatetime.h>

GraphWidget::GraphWidget(QWidget *parent) : QWidget(parent),
    m_repaintTimer(new QTimer(this)),
    m_startTime(QDateTime::currentMSecsSinceEpoch()),
    m_pauseStart(0),
    m_pixelsPerSecond(5)
{
    m_repaintTimer->setInterval(200);
    connect (m_repaintTimer, SIGNAL(timeout()), this, SLOT(repaint()));
    m_repaintTimer->start();
}

void GraphWidget::setDifficulty(int difficulty)
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const qint64 relativeTime = now - m_startTime;
    if (!m_difficultyGraph.isEmpty())
        m_difficultyGraph.append(QPointF(relativeTime / 500, m_difficultyGraph.last().y()));
    m_difficultyGraph.append(QPointF(relativeTime / 500, difficulty));
}

void GraphWidget::pause()
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (m_pauseStart == 0) { // start pause
        m_pauseStart = now;
        m_repaintTimer->stop();
    } else {
        m_startTime += now - m_pauseStart;
        m_pauseStart = 0;
        m_repaintTimer->start();
    }
}

void GraphWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(0, 0, width(), height(), Qt::white);
    painter.translate(0, height());
    painter.scale(0.5 * m_pixelsPerSecond, -height() / 100.);
    painter.setRenderHint(QPainter::Antialiasing);

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const qint64 relativeTime = now - m_startTime;
    const double widgetWidthInUnits = width() / (float) m_pixelsPerSecond;
    const double overShoot = relativeTime / 1000. - widgetWidthInUnits;
    if (overShoot > 0) {
        int offset = qRound(overShoot);
        offset = 4 + offset - (offset % 4); // aim for 4 second jumps
        painter.translate(-offset * 2, 0);
    }

    if (!m_difficultyGraph.isEmpty()) {
        painter.setPen(QPen(Qt::blue, 1));
        QPolygonF graph(m_difficultyGraph);
        graph.append(QPointF(relativeTime / 500, graph.last().y()));
        painter.drawPolyline(graph);
    }
}
