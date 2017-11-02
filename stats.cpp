#include "stats.h"

#include <QDebug>
#include <qmath.h>

Stats::Stats(QObject *parent) : QObject(parent)
{
}

void Stats::newBlockFound(int height, qint64 timestamp)
{
    m_timestamps.append(timestamp);
    if ((height % 576) == 0) {
        calcStats(576);
        if (height >= 4032)
            calcStats(4032);
    }
}

void Stats::calcStats(const int blocks)
{
    QList<int> blockTimeStamps;
    qint64 prev = 0;
    quint64 totalTime = 0;
    QMap<int, int> times;
    for (int i = m_timestamps.length() - blocks; i < m_timestamps.length(); ++i) {
        if (prev) {
            int timespan = m_timestamps.at(i) - prev;
            blockTimeStamps.push_back(timespan);
            totalTime += timespan;
            times[timespan]++;
        }
        prev = m_timestamps.at(i);
    }


    qDebug() << blocks << "blocks";
    // timing is local, with a multiplication of 6000 times real we deal with 100ms block-times.
    qDebug() << "   Average:" << totalTime * 6 / blockTimeStamps.count() << "seconds per block";

    qSort(blockTimeStamps);
    qDebug() << "      Mean:" << blockTimeStamps.at(blockTimeStamps.size() / 2) * 6 << "seconds";

    const int mean = totalTime / blockTimeStamps.count();
    quint64 sum = 0;
    foreach (auto duration, blockTimeStamps) {
        sum += (duration - mean) * (duration - mean);
    }
    qDebug() << "  Variance:" << qSqrt(sum * 6 / (double) (blockTimeStamps.size() - 1)) << "sec";

}
