#include "miner.h"

Miner::Miner(QObject *parent) : QObject(parent),
    m_hashPower(100)
{
}

int Miner::hashPower() const
{
    return m_hashPower;
}

void Miner::setHashPower(int hashPower)
{
    if (m_hashPower == hashPower)
        return;
    m_hashPower = hashPower;
    emit hashPowerChanged();
}
