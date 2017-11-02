#ifndef GRAPHWIDGET_H
#define GRAPHWIDGET_H

#include <QWidget>

class GraphWidget : public QWidget
{
    Q_OBJECT
public:
    explicit GraphWidget(QWidget *parent = nullptr);

public slots:
    void setDifficulty(int difficulty);
    void setHashrate(int hashrate);
    void addBlock();
    void addMarker();//
    void pause();
    void setGraphZoom(int zoom);

protected:
    void paintEvent(QPaintEvent *event);

private:
    QTimer *m_repaintTimer;
    QPolygonF m_difficultyGraph;
    QPolygonF m_hashrateGraph;
    QPolygonF m_blocksFoundGraph;
    QPolygonF m_blocksFoundAverageGraph;
    QPolygonF m_MarkersGraph;
    // QPainterPath m_blocksFound;
    qint64 m_startTime;
    qint64 m_pauseStart;
    float m_pixelsPerSecond;

    qint64 m_unitStartTime;
    int m_blocksFoundThisUnit;
};

#endif // GRAPHWIDGET_H
