#ifndef CHAIN_H
#define CHAIN_H

#include "miner.h"

#include <QObject>

class Chain : public QObject
{
    Q_OBJECT

public:
    enum AdjustmentAlgorithm {
        Satoshi,
        EDA,
        Neil
    };
    explicit Chain(QObject *parent = nullptr);

    int difficulty() const;
    void setDifficulty(int difficulty);
    Miner* appendNewMiner();
    void addBlock(int height);

signals:
    void difficultyChanged(int newDifficulty);
    void newBlock(int height);
    void hashpowerChanged(int hashPower);

public slots:
    void deleteMiner(Miner* miner);
    void pause();
    void hashpowerChanged();
    void setAdjustmentAlgorithm(AdjustmentAlgorithm algo);

private slots:
    void doMine();
    void miningSuccessfull();

private:
    int neilsAlgo() const;

    QList<Miner*> m_miners;
    QList<qint64> m_blockTimeStamps;
    qint64 m_timeLastPeriodStart;
    qint64 m_pauseStart;

    int m_difficulty;
    int m_baseDifficulty;

    int m_height;

    AdjustmentAlgorithm m_algo;

    QTimer *m_timer;
};

#endif // CHAIN_H
