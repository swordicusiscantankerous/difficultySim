#ifndef CHAIN_H
#define CHAIN_H

#include "miner.h"

#include <QObject>

class Chain : public QObject
{
    Q_OBJECT

public:
    explicit Chain(QObject *parent = nullptr);

    int difficulty() const;
    void setDifficulty(int difficulty);

signals:
    void difficultyChanged(int newDifficulty);
    void newBlock(int height);

public slots:
    void addBlock(int height);
    void appendNewMiner();
    void deleteMiner(int index);

private:
    QList<Miner*> m_miners;
    QList<qint64> m_blockTimeStamps;
    qint64 m_timeLastPeriodStart;

    int m_difficulty;
    int m_baseDifficulty;

    int m_height;
};

#endif // CHAIN_H
