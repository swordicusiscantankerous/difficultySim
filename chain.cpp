#include "chain.h"

#include <QDateTime>
#include <QDebug>

#include <math.h>

Chain::Chain(QObject *parent) : QObject(parent),
    m_timeLastPeriodStart(0),
    m_pauseStart(0),
    m_difficulty(-1),
    m_baseDifficulty(-1),
    m_height(-1),
    m_timer(new QTimer(this))
{
    addBlock(0); // genesis
    m_timer->setSingleShot(true);
    connect (m_timer, SIGNAL(timeout()), this, SLOT(miningSuccessfull()));
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
        m_baseDifficulty = m_difficulty = 1000;
        return;
    }
    emit newBlock(m_height);

    if (m_height  % 500 == 0)
        qDebug() << QTime::currentTime().toString() << m_height << "@" << m_difficulty;

    if (m_algo == Satoshi || m_algo == EDA) {
        if (m_height % 2016 == 0) {
            const qint64 targetTimeSpan = 2016 * 600 * 1000 / 6000; // we aim to be a 6000 times faster than real-time.
            // recalculate base difficulty.
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
        if (m_algo == EDA && m_blockTimeStamps.length() > 12) {
            // remember, time multiplication is 6000.
            qint64 mtpTip = m_blockTimeStamps.at(m_blockTimeStamps.count() - 7);
            qint64 mtpTipMinus6 = m_blockTimeStamps.at(m_blockTimeStamps.count() - 7 - 6);
            // qDebug() << "diff" << (mtpTip - mtpTipMinus6);
            if (mtpTip - mtpTipMinus6 > 1000 * 12 * 3600 / 6000) {
                m_difficulty = qRound(m_difficulty * 0.8);
                // qDebug() << "Adjustsing difficulty downards" << m_difficulty;
                emit difficultyChanged(m_difficulty);
            }
        }
    }
    else if (m_algo == Neil) {
        m_blockTimeStamps.append(now);
        int newDifficulty = neilsAlgo();
        if (newDifficulty != m_difficulty) {
            m_difficulty = newDifficulty;
            emit difficultyChanged(m_difficulty);
        }
    }
}

int Chain::neilsAlgo() const
{
    const qint64 targetTimeSpan = 600 * 1000 / 6000; // we aim to be a 6000 times faster than real-time.
    const int ProofOfWorkLimit = 1000;

    // Height of next block
    // uint32_t nHeight = pindexPrev ? pindexPrev->nHeight + 1 : 0;

    // First 6 blocks have genesis block difficulty
    if (m_height <= 6)
        return ProofOfWorkLimit;

    int newDifficulty = m_difficulty;

    // Long-term block production time stabiliser that should rarely
    // trigger.  Should miners produce blocks considerably faster than
    // target spacing but collude to fudge timestamps to stay above the 30
    // minute window, ensure difficulty will rise to compensate anyway.
    if (m_height > 2016) {
        qint64 t = m_blockTimeStamps.last() - m_blockTimeStamps.at(m_blockTimeStamps.size() - 2017);

        qDebug() << "expected period;" << targetTimeSpan * 2016 * 95 / 100 << "actual" << t;

        // If too fast, decrease the target by 1/256.  Std deviation over
        // 2016 blocks is 2.2%.
        if (t < targetTimeSpan * 2016 * 95 / 100) {
            newDifficulty += (newDifficulty >> 8);
            // qDebug() << "long term nudging, increasing difficulty" << m_difficulty << newDifficulty;
        }
    }

    // Determine the MTP difference - the difference between the MTP
    // of the previous block, and the block 6 blocks earlier

    const qint64 mtpTip = m_blockTimeStamps.at(m_blockTimeStamps.count() - 7);
    const qint64 mtpTipMinus6 = m_blockTimeStamps.at(m_blockTimeStamps.count() - 7 - 6);
    const int64_t mtp6blocks = mtpTip - mtpTipMinus6;

    // If too fast (< 30 mins), decrease the target by 1/256.
    if (mtp6blocks < targetTimeSpan * 30 / 10) {
        // qDebug() << "blocks too fast" << m_difficulty << newDifficulty;
        newDifficulty += (newDifficulty >> 8);
    }
    // If too slow (> 128 mins), increase the target by 1/64.
    else if (mtp6blocks > targetTimeSpan * 128 / 10) {
        // qDebug() << "blocks too slow" << m_difficulty << newDifficulty;
        newDifficulty -= (newDifficulty >> 6);
    }

    // We can't go below the minimum target
    return std::max(newDifficulty, ProofOfWorkLimit);
}

Miner *Chain::appendNewMiner()
{
    Miner *newMiner = new Miner(this);
    connect (newMiner, SIGNAL(hashPowerChanged()), this, SLOT(hashpowerChanged()));

    m_miners.append(newMiner);
    hashpowerChanged();
    doMine();
    return newMiner;
}

void Chain::deleteMiner(Miner *miner)
{
    Q_ASSERT(miner);

    m_miners.removeAll(miner);
    hashpowerChanged();
}

void Chain::pause()
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();

    if (m_pauseStart == 0) { // start pause
        m_pauseStart = now;
        m_timer->stop();
    } else {
        m_timeLastPeriodStart += now - m_pauseStart;
        m_pauseStart = 0;
        doMine();
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

void Chain::setAdjustmentAlgorithm(Chain::AdjustmentAlgorithm algo)
{
    m_algo = algo;
}

void Chain::doMine()
{
    // Calculate the time till a new block is expected.
    m_timer->stop();

    int hashPower = 0;
    foreach (const Miner *miner, m_miners) {
        hashPower += miner->hashPower();
    }
    if (hashPower == 0 || m_pauseStart)
        return;

    double timeTillNextBlock = 20 * m_difficulty / hashPower;
    double sample = (qrand() % (int) 1E6) / 1E6;
    double lambda = 1 / timeTillNextBlock;
    int sleep = 0.5 + log(1. - sample) / -lambda;
    if (!m_blockTimeStamps.isEmpty()) {
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        const qint64 last = m_blockTimeStamps.last();
        sleep = last + sleep - now;
    }

    // qDebug() << "miners will find block" << m_height << "in" << qMax(1, sleep) * 6 << "s, difficulty:" << m_difficulty << "HP:" << hashPower;
    m_timer->start(qMax(1, sleep));
}

void Chain::miningSuccessfull()
{
    addBlock(m_height + 1);
    doMine();
}
