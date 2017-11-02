#ifndef STATS_H
#define STATS_H

#include <QObject>

class Stats : public QObject
{
    Q_OBJECT
public:
    explicit Stats(QObject *parent = nullptr);

public slots:
    void newBlockFound(int height, qint64 timestamp);

private:
    void calcStats(int blocks);

    QList<qint64> m_timestamps;
};

#endif // STATS_H
