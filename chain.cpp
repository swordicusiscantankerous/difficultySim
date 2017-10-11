#include "chain.h"

#include <QDateTime>
#include <QDebug>

Chain::Chain(QObject *parent) : QObject(parent),
  m_timeLastPeriodStart(0),
  m_pauseStart(0),
  m_difficulty(-1),
  m_baseDifficulty(-1),
  m_height(-1)
{
    addBlock(0); // genesis
}

int Chain::difficulty() const
{
    return m_difficulty;
}

void Chain::setDifficulty(int difficulty)
{
    m_difficulty = difficulty;
}

void Chain::addBlock(int height)
{
    /* Time-multiplication is 6000
     *  * Two weeks (2016 blocks) is then 202 sec.
     *  * 12 hours (EDA) is then 7 sec.
     */

    if (m_height >= height)
        return;
    if (m_pauseStart != 0) // we are paused
        return;

    const qint64 now = QDateTime::currentMSecsSinceEpoch();

    m_height = height;
    if (m_height == 0) {
        m_timeLastPeriodStart = now;
        m_baseDifficulty = m_difficulty = 10;
        return;
    }
    emit newBlock(m_height);

    if (m_height  % 500 == 0)
        qDebug() << QTime::currentTime().toString() << m_height << "@" << m_difficulty;

    if (m_height % 2016 == 0) {
        // recalculate base difficulty.
        const qint64 targetTimeSpan = 2016 * 600 * 1000 / 6000; // we aim to be a 6000 times faster than real-time.
        qint64 actualTimeSpan = now - m_timeLastPeriodStart;
        qDebug() << "Period completed. Wanted time:" << targetTimeSpan << "took:" << actualTimeSpan;
        actualTimeSpan = qBound(targetTimeSpan / 4, actualTimeSpan, targetTimeSpan * 4);

        m_baseDifficulty = m_baseDifficulty * targetTimeSpan / actualTimeSpan;
        m_difficulty = m_baseDifficulty;
        m_timeLastPeriodStart = now;
        emit difficultyChanged(m_difficulty);
        return;
    }
    m_blockTimeStamps.append(now);
    if (m_blockTimeStamps.length() > 12) {
        // TODO calculate EDA or reverse EDA

    }
}

Miner *Chain::appendNewMiner()
{
    Miner *newMiner = new Miner(this);
    connect (newMiner, SIGNAL(blockFound(int)), this, SLOT(addBlock(int)));
    connect (this, SIGNAL(difficultyChanged(int)), newMiner, SLOT(setDifficulty(int)));
    connect (this, SIGNAL(newBlock(int)), newMiner, SLOT(setBlockHeight(int)));
    connect (newMiner, SIGNAL(hashPowerChanged()), this, SLOT(hashpowerChanged()));

    newMiner->setBlockHeight(m_height);
    newMiner->setDifficulty(m_difficulty);
    m_miners.append(newMiner);
    hashpowerChanged();
    return newMiner;
}

void Chain::deleteMiner(Miner *miner)
{
    Q_ASSERT(miner);

    m_miners.removeAll(miner);
    disconnect (miner, SIGNAL(blockFound(int)), this, SLOT(addBlock(int)));
    hashpowerChanged();
}

void Chain::pause()
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();

    if (m_pauseStart == 0) { // start pause
        m_pauseStart = now;
    } else {
        m_timeLastPeriodStart += now - m_pauseStart;
        m_pauseStart = 0;
        emit newBlock(m_height);
    }
}

void Chain::hashpowerChanged()
{
    int totalHashpower = 0;
    foreach (auto miner, m_miners) {
        totalHashpower += miner->hashPower();
    }
    emit hashpowerChanged(totalHashpower);
}
