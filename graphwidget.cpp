#include "graphwidget.h"

#include <QDateTime>
#include <QPainter>
#include <QTimer>
#include <QDebug>
#include <qdatetime.h>

#define AVERAGE_SAMPLES 10

GraphWidget::GraphWidget(QWidget *parent) : QWidget(parent),
    m_repaintTimer(new QTimer(this)),
    m_startTime(QDateTime::currentMSecsSinceEpoch()),
    m_pauseStart(0),
    m_pixelsPerSecond(5),
    m_unitStartTime(0),
    m_blocksFoundThisUnit(0)
{
    m_repaintTimer->setInterval(200);
    connect (m_repaintTimer, SIGNAL(timeout()), this, SLOT(repaint()));
    m_repaintTimer->start();

    m_blocksFoundAverageGraph << QPointF();
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
    if (m_pauseStart != 0)
        return;
    const int UnitTime = 600;
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - m_unitStartTime > UnitTime) {// start new unit
        const qint64 relativeTime = now - m_startTime;
        m_blocksFoundGraph.append(QPointF(relativeTime / 500, m_blocksFoundThisUnit));
        m_blocksFoundThisUnit = 1;
        m_unitStartTime = now;
        if (m_blocksFoundGraph.size() >= 10) {
            int total = 0;
            for (int i = 1; i <= AVERAGE_SAMPLES; ++i)
                total += m_blocksFoundGraph.at(m_blocksFoundGraph.size() - i).y();

            m_blocksFoundAverageGraph.append(QPointF(relativeTime / 500, total / (qreal) AVERAGE_SAMPLES));
        } else {
            m_blocksFoundAverageGraph = m_blocksFoundGraph;
        }
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
        m_unitStartTime += now - m_pauseStart;
        m_pauseStart = 0;
        m_repaintTimer->start();
    }
}

void GraphWidget::setGraphZoom(int zoom)
{
    m_pixelsPerSecond = 25. / zoom;
}

void GraphWidget::paintEvent(QPaintEvent *)
{
    const QColor blocksFoundColor(235, 250, 234);
    const QColor difficultyColor(13, 13, 245);
    const QColor hashRateColor(255, 145, 21);

    QPainter painter(this);
    painter.setFont(QFont("Sans", 10));
    painter.fillRect(0, 0, width(), height(), Qt::white);
    QMatrix matrix;
    matrix.translate(0, height());
    matrix.scale(0.5 * m_pixelsPerSecond, -height() / 8000.);
    painter.setRenderHint(QPainter::Antialiasing);

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const qint64 relativeTime = now - m_startTime;
    const double widgetWidthInUnits = width() / (float) m_pixelsPerSecond;
    const double overShoot = relativeTime / 1000. - widgetWidthInUnits;
    QMatrix modelToViewMatrix(matrix);
    if (overShoot > 0) {
        int offset = qRound(overShoot);
        offset = 4 + offset - (offset % 4); // aim for 4 second jumps
        modelToViewMatrix.translate(-offset * 2, 0);
    }

    const int fontHeight = painter.fontMetrics().xHeight();
    if (!m_blocksFoundGraph.isEmpty()) {
        QMatrix m2(modelToViewMatrix);
        m2.scale(1, 120);

        if (!m_blocksFoundAverageGraph.isEmpty()) {
            QPolygonF graph(m_blocksFoundAverageGraph);
            int total = 0;;
            int sampleCount = 0;
            for (int i = 1; i <= AVERAGE_SAMPLES && i < m_blocksFoundGraph.count(); ++i) {
                if (graph.last().x() == m_blocksFoundGraph.at(m_blocksFoundGraph.count() - i).x())
                    break;
                total += m_blocksFoundGraph.at(m_blocksFoundGraph.count() - ++sampleCount).y();
            }
            if (sampleCount > 0)
                graph.append(QPointF(relativeTime / 500, total / (qreal) sampleCount));
            else
                graph.append(QPointF(relativeTime / 500, graph.last().y()));

            painter.setBrush(Qt::NoBrush);
            painter.setPen(blocksFoundColor.darker());
            painter.drawPolyline(m2.map(m_blocksFoundAverageGraph));

            graph.append(QPointF(relativeTime / 500, 0));
            painter.setPen(Qt::NoPen);
            painter.setBrush(blocksFoundColor);
            painter.drawPolygon(m2.map(graph));
        }

        painter.setPen(QPen(QColor(153, 38, 40), 2));
        painter.drawPoints(m2.map(m_blocksFoundGraph));

        m2 = QMatrix(matrix); // skip the offset in time.
        m2.scale(1, 120);
        painter.setPen(QPen(Qt::red, 1, Qt::DashDotDotLine));
        const QPointF tenBPS = m2.map(QPointF(0, 6));
        painter.drawLine(0, tenBPS.y(), width(), tenBPS.y()); // 6 blocks per hour
        painter.setPen(Qt::black);
        painter.drawText(QPoint(10, -1) + m2.map(QPointF(0, 6)), "10-minute-block");
    }

    if (!m_difficultyGraph.isEmpty()) {
        painter.setPen(QPen(difficultyColor));
        QPolygonF graph(m_difficultyGraph);
        graph.append(QPointF(relativeTime / 500, graph.last().y()));
        painter.drawPolyline(modelToViewMatrix.map(graph));
    }

    if (!m_hashrateGraph.isEmpty()) {
        painter.setPen(QPen(hashRateColor));
        QPolygonF graph(m_hashrateGraph);
        graph.append(QPointF(relativeTime / 500, graph.last().y()));
        QMatrix m2(modelToViewMatrix);
        m2.scale(1, 6);
        painter.drawPolyline(m2.map(graph));
    }


    int x = 15;
    painter.setPen(Qt::black);
    painter.fillRect(x, 10, fontHeight, fontHeight, difficultyColor);
    x += fontHeight * 1.5;
    painter.drawText(QPoint(x, 10 + fontHeight), "Difficulty");
    x += painter.fontMetrics().width("Difficulty") + fontHeight;
    painter.fillRect(x, 10, fontHeight, fontHeight, hashRateColor);
    x += fontHeight * 1.5;
    painter.drawText(QPoint(x, 10 + fontHeight), "Hashrate");
    x += painter.fontMetrics().width("Hashrate") + fontHeight;
    painter.fillRect(x, 10, fontHeight, fontHeight, blocksFoundColor.darker());
    x += fontHeight * 1.5;
    painter.drawText(QPoint(x, 10 + fontHeight), "Blocks Found");


    float pixelsPerWeek = m_pixelsPerSecond * 3600 * 24 * 7 / 6000;
    int numDaysDrawn = 7;
    while (numDaysDrawn != 1 && numDaysDrawn * (pixelsPerWeek / 7) > width() / 2) {
        numDaysDrawn--;
    }

    painter.setPen(QPen(Qt::black, 1));
    painter.drawText(QPoint(width() - pixelsPerWeek / 7 * numDaysDrawn, height() - 14 - fontHeight * 2),
                     pixelsPerWeek < 105 ? "1 week" : (numDaysDrawn == 1 ? "day" : "days"));
    painter.setBrush(QColor(255, 255, 255, 180));
    painter.drawRect(width() - (pixelsPerWeek / 7 * numDaysDrawn) - 10, height() - 10 - fontHeight * 2,
                     pixelsPerWeek / 7 * numDaysDrawn, fontHeight * 2);
    for (int i = 1; i < numDaysDrawn; i+=2) {
        painter.fillRect(width() - (pixelsPerWeek / 7) * (i + 1) - 10, height() - 10 - fontHeight * 2,
                         pixelsPerWeek / 7, fontHeight * 2, QColor(0, 0, 0, 180));
    }
}
