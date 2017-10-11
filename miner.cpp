#include "miner.h"
#include <QDebug>

Miner::Miner(QObject *parent) : QObject(parent),
    m_hashPower(100),
    m_blockHeight(-1),
    m_difficulty(-1),
    m_timer(new QTimer(this))
{
    m_timer->setSingleShot(true);
    connect (m_timer, SIGNAL(timeout()), this, SLOT(miningSuccessFull()));
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
    startMining();
    emit hashPowerChanged();
}

void Miner::setBlockHeight(int height)
{
    if (m_blockHeight == height)
        return;
    m_timer->stop();
    m_blockHeight = height;
    startMining();
}

void Miner::setDifficulty(int difficulty)
{
    if (m_difficulty == difficulty)
        return;
    m_difficulty = difficulty;
    startMining();
}

void Miner::miningSuccessFull()
{
    emit blockFound(m_blockHeight + 1);
}

void Miner::startMining()
{
    m_timer->stop();
    if (m_blockHeight < 0 || m_difficulty < 0)
        return;

    // the chance I'll find the block is a random number.
    // adding more hashpower helps, though.
    float chance = qrand() % 1000;
    chance = chance * m_difficulty / m_hashPower ;

    // qDebug() << "miner will find block" << m_blockHeight << "in"  << chance << "difficulty now is" << m_difficulty;
    m_timer->setInterval(qMax(10, (int) chance));
    m_timer->start();
}
