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
    Miner* appendNewMiner();

signals:
    void difficultyChanged(int newDifficulty);
    void newBlock(int height);
    void hashpowerChanged(int hashPower);

public slots:
    void addBlock(int height);
    void deleteMiner(Miner* miner);
    void pause();
    void hashpowerChanged();
    void setEdaEnabled(bool enabled);

private:
    QList<Miner*> m_miners;
    QList<qint64> m_blockTimeStamps;
    qint64 m_timeLastPeriodStart;
    qint64 m_pauseStart;

    int m_difficulty;
    int m_baseDifficulty;

    int m_height;

    bool m_edaEnabled;
};

#endif // CHAIN_H
