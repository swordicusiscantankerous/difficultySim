#ifndef MINER_H
#define MINER_H

#include <QObject>
#include <QTimer>

class Miner : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int hashPower READ hashPower WRITE setHashPower NOTIFY hashPowerChanged)
public:
    explicit Miner(QObject *parent = nullptr);

    int hashPower() const;
    void setHashPower(int hashPower);

signals:
    void hashPowerChanged();
    void blockFound(int blockHeight);

public slots:
    void setBlockHeight(int height);
    void setDifficulty(int difficulty);

private slots:
    void miningSuccessFull();

private:
    void startMining();


    int m_hashPower;
    int m_blockHeight;
    int m_difficulty;
    QTimer *m_timer;
};

#endif // MINER_H
