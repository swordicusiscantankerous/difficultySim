#ifndef MINER_H
#define MINER_H

#include <QObject>
#include <QTimer>

class Miner : public QObject
{
    Q_OBJECT
public:
    explicit Miner(QObject *parent = nullptr);

    int hashPower() const;
    void setHashPower(int hashPower);

signals:
    void hashPowerChanged();

private:
    int m_hashPower;
};

#endif // MINER_H
