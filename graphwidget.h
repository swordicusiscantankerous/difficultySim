#ifndef GRAPHWIDGET_H
#define GRAPHWIDGET_H

#include <QWidget>

class GraphWidget : public QWidget
{
    Q_OBJECT
public:
    explicit GraphWidget(QWidget *parent = nullptr);

// signals:

public slots:
    void setDifficulty(int difficulty);
    void setHashrate(int hashrate);
    void pause();

protected:
    void paintEvent(QPaintEvent *event);

private:
    QTimer *m_repaintTimer;
    QPolygonF m_difficultyGraph;
    QPolygonF m_hashrateGraph;
    // QPolygonF m_blocksFoundGraph;
    qint64 m_startTime;
    qint64 m_pauseStart;
    int m_pixelsPerSecond;
};

#endif // GRAPHWIDGET_H
