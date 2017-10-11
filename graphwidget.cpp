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
    m_pixelsPerSecond(5),
    m_unitStartTime(0)
{
    m_repaintTimer->setInterval(200);
    connect (m_repaintTimer, SIGNAL(timeout()), this, SLOT(repaint()));
    m_repaintTimer->start();

    m_blocksFoundGraph << QPointF();
}

void GraphWidget::setDifficulty(int difficulty)
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const qint64 relativeTime = now - m_startTime;
    if (!m_difficultyGraph.isEmpty())
        m_difficultyGraph.append(QPointF(relativeTime / 500, m_difficultyGraph.last().y()));
    m_difficultyGraph.append(QPointF(relativeTime / 500, difficulty));
}

void GraphWidget::setHashrate(int hashrate)
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const qint64 relativeTime = now - m_startTime;
    if (!m_hashrateGraph.isEmpty())
        m_hashrateGraph.append(QPointF(relativeTime / 500, m_hashrateGraph.last().y()));
    m_hashrateGraph.append(QPointF(relativeTime / 500, hashrate));
}

void GraphWidget::addBlock()
{
    const int UnitTime = 1000;
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - m_unitStartTime > UnitTime) {// start new unit
        const qint64 relativeTime = now - m_startTime;
        m_blocksFoundGraph.append(QPointF(relativeTime / 500, m_blocksFoundThisUnit));
        qDebug() << m_blocksFoundThisUnit;
        m_blocksFoundThisUnit = 1;
        m_unitStartTime = now;
    } else {
        ++m_blocksFoundThisUnit;
    }
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
    painter.setFont(QFont("Sans", 10));
    painter.setPen(Qt::black);
    int fontHeight = painter.fontMetrics().xHeight();
    int x = 15;
    painter.fillRect(x, 10, fontHeight, fontHeight, Qt::blue);
    x += fontHeight * 1.5;
    painter.drawText(QPoint(x, 10 + fontHeight), "Difficulty");
    x += painter.fontMetrics().width("Difficulty") + fontHeight;
    painter.fillRect(x, 10, fontHeight, fontHeight, Qt::black);
    x += fontHeight * 1.5;
    painter.drawText(QPoint(x, 10 + fontHeight), "Hashrate");
    x += painter.fontMetrics().width("Hashrate") + fontHeight;
    const QColor blocksFoundColor(255, 226, 76, 170);
    painter.fillRect(x, 10, fontHeight, fontHeight, blocksFoundColor);
    x += fontHeight * 1.5;
    painter.drawText(QPoint(x, 10 + fontHeight), "Blocks Found");

    QMatrix matrix;
    matrix.translate(0, height());
    matrix.scale(0.5 * m_pixelsPerSecond, -height() / 130.);
    painter.setRenderHint(QPainter::Antialiasing);

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const qint64 relativeTime = now - m_startTime;
    const double widgetWidthInUnits = width() / (float) m_pixelsPerSecond;
    const double overShoot = relativeTime / 1000. - widgetWidthInUnits;
    if (overShoot > 0) {
        int offset = qRound(overShoot);
        offset = 4 + offset - (offset % 4); // aim for 4 second jumps
        matrix.translate(-offset * 2, 0);
    }

    if (!m_difficultyGraph.isEmpty()) {
        painter.setPen(QPen(Qt::blue, 2));
        QPolygonF graph(m_difficultyGraph);
        graph.append(QPointF(relativeTime / 500, graph.last().y()));
        painter.drawPolyline(matrix.map(graph));
    }

    if (!m_hashrateGraph.isEmpty()) {
        painter.setPen(QPen(Qt::black, 2));
        QPolygonF graph(m_hashrateGraph);
        graph.append(QPointF(relativeTime / 500, graph.last().y()));
        QMatrix m2(matrix);
        m2.scale(1, 1/12.);
        painter.drawPolyline(m2.map(graph));
    }
    if (!m_blocksFoundGraph.isEmpty()) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 226, 76, 170));
        QPolygonF graph(m_blocksFoundGraph);
        graph.append(QPointF(relativeTime / 500, graph.last().y()));
        graph.append(QPointF(relativeTime / 500, 0));
        QMatrix m2(matrix);
        m2.scale(1, 3);
        painter.drawPolygon(m2.map(graph));
    }
}
