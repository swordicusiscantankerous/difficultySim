#include "chain.h"

#include <QDateTime>
#include <QDebug>

#include <math.h>

Chain::Chain(QObject *parent) : QObject(parent),
    m_timeLastPeriodStart(0),
    m_pauseStart(0),
    m_difficulty(-1),
    m_baseDifficulty(-1),
    m_emergencyCount(0),
    m_height(-1),
    m_algo(Satoshi),
    m_timer(new QTimer(this))
{
    addBlock(0); // genesis
    m_timer->setSingleShot(true);
    connect (m_timer, SIGNAL(timeout()), this, SLOT(miningSuccessfull()));
    m_blockDifficultiescw126.append(20000);
    m_blockDifficultiescw126.append(20000);
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
        m_baseDifficulty = m_difficulty = 20000;
        return;
    }
    emit newBlock(m_height, now);

    if (m_height  % 500 == 0)
        qDebug() << QTime::currentTime().toString() << m_height << "@" << m_difficulty;

    if (m_algo == Satoshi || m_algo == EDA || m_algo == dEDA || m_algo == dEDAnobaseline) {
        m_blockDifficulties.append(m_difficulty);
        if (m_height % 2016 == 0) {
            emit newMarker();
            const qint64 targetTimeSpan = 2016 * 600 * 1000 / 6000; // we aim to be a 6000 times faster than real-time.
            // recalculate base difficulty.
            qint64 actualTimeSpan = now - m_timeLastPeriodStart;
            qDebug() << "Period completed. Wanted time:" << targetTimeSpan << "took:" << actualTimeSpan;
            actualTimeSpan = qBound(targetTimeSpan / 4, actualTimeSpan, targetTimeSpan * 4);
            if(m_algo == dEDA || m_algo == dEDAnobaseline){/////////dEDA////////dEDAnobaseline
                m_emergencyCount = 0;
            }
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
        /////////dualEDA v1: base pow(0.8, n)/////////////
        if (m_algo == dEDA && m_blockTimeStamps.length() > 12) {//////dualEDA v1 pow(0.8, n)
            qint64 mtpTip = m_blockTimeStamps.at(m_blockTimeStamps.count() - 7);
            qint64 mtpTipMinus6 = m_blockTimeStamps.at(m_blockTimeStamps.count() - 7 - 6);
            int tmpemergency = m_emergencyCount;
            if (mtpTip - mtpTipMinus6 > 6 * 12 * 600 * 1000 / 6000) {//1st downward
                m_emergencyCount++;     // 6 blocks ago is 12 hours in the past
            }
            if (mtpTip - mtpTipMinus6 < 6 / 4 * 600 * 1000 / 6000 && m_emergencyCount > 0) {//2nd upward
                m_emergencyCount--;     // 6 blocks ago is 15 minutes in the past
            }
            if (tmpemergency != m_emergencyCount) {
                m_difficulty = qRound(m_baseDifficulty * pow(0.8, m_emergencyCount));// base * (.8)^eda
                emit difficultyChanged(m_difficulty);
            }
        }
        /////////dualEDA modification of Tom's algo: base pow(0.8, n)/////////////
        if (m_algo == dEDAnobaseline && m_blockTimeStamps.length() > 12) {//////dualEDAmodTom pow(0.8, n) + emergencyUP
            qint64 mtpTip = m_blockTimeStamps.at(m_blockTimeStamps.count() - 7);
            qint64 mtpTipMinus6 = m_blockTimeStamps.at(m_blockTimeStamps.count() - 7 - 6);
            int tmpemergency = m_emergencyCount;
            if (mtpTip - mtpTipMinus6 > 6 * 12 * 600 * 1000 / 6000) {//1st downward
                m_emergencyCount++;     // 6 blocks ago is 12 hours in the past
            }
            if (mtpTip - mtpTipMinus6 < 6 / 4 * 600 * 1000 / 6000) {//2nd upward blocks allow above baseline adj
                m_emergencyCount--;     // 6 blocks ago is 15 minutes in the past
            }
            if (tmpemergency != m_emergencyCount) {
                m_difficulty = qRound( m_baseDifficulty * pow(0.8, m_emergencyCount));// base * (.8)^eda
                emit difficultyChanged(m_difficulty);
            }
        }
    }
    else {
        m_blockTimeStamps.append(now);
        m_blockDifficulties.append(m_difficulty);
        int newDifficulty = 0;
        if (m_algo == Neil)
            newDifficulty = neilsAlgo();
        else if (m_algo == DeadalnixOld)
            newDifficulty = deadalnixAlgo();
        else if (m_algo == cw144)
            newDifficulty = cw144Algo();
        else if (m_algo == wt144)
            newDifficulty = wt144Algo();
        else if (m_algo == sword126blocks){
            if (m_height % 126 == 0){
                emit newMarker();
                newDifficulty = swordcw126fastT();
                if (newDifficulty != 0)
                    m_blockDifficultiescw126.append(newDifficulty);
                newDifficulty = swordwt126slowT();
                if (newDifficulty != 0)
                    m_baseDifficulty = newDifficulty;
            }
            newDifficulty = swordwt126Algo();
        }else
            Q_ASSERT(false);

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

        // qDebug() << "expected period;" << targetTimeSpan * 2016 * 95 / 100 << "actual" << t;

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

int Chain::deadalnixAlgo() const //work in progress
{
    const qint64 targetTimeSpan_2016 = 2016 * 600 * 1000 / 6000; // we aim to be a 6000 times faster than real-time.
    const qint64 targetTimeSpan_13 = 13 * 600 * 1000 / 6000; // we aim to be a 6000 times faster than real-time.
    const int ProofOfWorkLimit = 1000;
    int newDifficulty = ProofOfWorkLimit;
    if (m_height <= 15)
        return ProofOfWorkLimit;
    const qint64 last_timestamp = m_blockTimeStamps.at(m_blockTimeStamps.count() - 2);
    int newDifficulty_2016 = ProofOfWorkLimit;
    if (m_height > 2018){
        const qint64 first_timestamp_2016 = m_blockTimeStamps.at(m_blockTimeStamps.count() - 2018);
        qint64 target_2016 = 0;
        for (int i=0; i<2016; i++) {
            const qint64 target_i = m_blockDifficulties.at(m_blockDifficulties.count() - 2018 + i);
            target_2016 = target_2016 + target_i;
        }
        target_2016 = target_2016 / 2016;
        const qint64 timeSpan_2016 = last_timestamp - first_timestamp_2016;
        //int newDifficulty_2016 = target_2016 * targetTimeSpan_2016 / timeSpan_2016;
        newDifficulty_2016 = target_2016 * targetTimeSpan_2016 / timeSpan_2016;
    }
    
    //qint64 prior_timestamp = m_blockTimeStamps.at(m_blockTimeStamps.count() - 16);
    const qint64 first_timestamp_13 = m_blockTimeStamps.at(m_blockTimeStamps.count() - 15);
    int count_13 = 1;
    qint64 time_13 = 0;
    qint64 target_13 = 0;
    //qDebug() << "mheight  " << m_height;
    for (int i=0; i<13; i++) {
        const qint64 target_i = m_blockDifficulties.at(m_blockDifficulties.count() - 15 + i);
        const qint64 time_i = m_blockTimeStamps.at(m_blockTimeStamps.count() - 15 + i);
        time_13 = time_13 + time_i - first_timestamp_13;
        target_13 = target_13 + target_i;
        if (i > 4 && time_13 > targetTimeSpan_13) {
            break;
        }
        count_13++;
        //qDebug() << time_13 << " " << target_13;
    }
    target_13 = target_13 / count_13;
    //qDebug() << "target13  " << target_13;
    //const qint64 timeSpan_13 = last_timestamp - first_timestamp_13;
    //int newDifficulty_13 = target_13 * targetTimeSpan_13 / timeSpan_13;
    int newDifficulty_13 = target_13 * targetTimeSpan_13 / time_13;
    //qDebug() << "newDifficulty_13  " << newDifficulty_13;
    
    int targetdiff  = newDifficulty_2016 - newDifficulty_13;
    //if (abs(targetdiff) <= qRound(0.25 * newDifficulty_2016)) {
    if (abs(targetdiff) >= qRound(0.25 * newDifficulty_2016)) {
        newDifficulty = newDifficulty_2016;
    }else{
        newDifficulty = newDifficulty_13;
    }
    //qDebug() << newDifficulty;
    
    qint64 last_target = m_blockDifficulties.at(m_blockDifficulties.count() - 2);
    int check12p5 = newDifficulty - last_target;
    //the next target is bounded to a maximum 12.5% change from the target of the previous block.
    if(abs(check12p5) > qRound(0.265625 * last_target)) {
        if(check12p5 > 0){
            newDifficulty = qRound(1.265625 * last_target);
        }else
            newDifficulty = qRound(0.765625 * last_target);
    }
    //qDebug() << "  " << newDifficulty;
    // We can't go below the minimum target
    return std::max(newDifficulty, ProofOfWorkLimit);
}

int Chain::cw144Algo() const //work in progress
{
    const qint64 targetTimeSpan = 600 * 1000 / 6000; // we aim to be a 6000 times faster than real-time.
    const int ProofOfWorkLimit = 1000;
    // First 144 blocks have genesis block difficulty
    if (m_height <= 147)
        return ProofOfWorkLimit;
    const qint64 first_timestamp = m_blockTimeStamps.at(m_blockTimeStamps.count() - 144 - 3);
    const qint64 last_timestamp = m_blockTimeStamps.at(m_blockTimeStamps.count() - 2);
    qint64 target_144 = 0;
    for (int i=1; i<=144; i++) {
        const int target_i = m_blockDifficulties.at(m_blockDifficulties.count() - 144 - 2 + i);
        target_144 = target_144 + target_i;
    }
    target_144 = target_144 / 144;
    qint64 timeSpan144 = last_timestamp - first_timestamp;
    timeSpan144 = qBound(72 * targetTimeSpan, timeSpan144, 288 * targetTimeSpan);
    int newDifficulty = target_144 * 144 * targetTimeSpan / timeSpan144;
    // We can't go below the minimum target
    return std::max(newDifficulty, ProofOfWorkLimit);
}

int Chain::wt144Algo() const
{
    const qint64 targetTimeSpan = 144 * 600 * 1000 / 6000; // we aim to be a 6000 times faster than real-time.
    const int ProofOfWorkLimit = 1000;
    // First 144 blocks have genesis block difficulty
    if (m_height <= 147)
        return ProofOfWorkLimit;
    qint64 timeSpan144 = 0;
    qint64 prior_timestamp = m_blockTimeStamps.at(m_blockTimeStamps.count() - 144 - 3);
    qint64 target_144 = 0;
    for (int i=1; i<=144; i++) {
        const qint64 target_i = m_blockDifficulties.at(m_blockDifficulties.count() - 144 - 2 + i);
        const qint64 time_i = m_blockTimeStamps.at(m_blockTimeStamps.count() - 144 - 2 + i);
        const qint64 d_time = time_i - prior_timestamp;
        prior_timestamp = time_i;
        timeSpan144 = timeSpan144 + d_time * (i + 1);
        target_144 = target_144 + target_i;
    }
    target_144 = target_144 / 144;
    timeSpan144 = timeSpan144 * 2 / (144 * 144 + 144) * 144;
    int newDifficulty = target_144 * targetTimeSpan / timeSpan144;
    // We can't go below the minimum target
    return std::max(newDifficulty, ProofOfWorkLimit);
}

//work in progress
//swordwt126Algo() barrowed from cw144, wt144, neil's, and deadalnix's original algo for design elements
// swordwt126 works by calculating difficulty every 6 blocks based on a MTP-6 block range 12-blocks long, and then modifying this new target twice by a scaled difference to two targets: a fast target, and a slow target. the fast target is calculated by a weighted-time sum of 21 6-block samples adding up to 126 blocks. The slow target is calculated by a weighted-time sum of 16 126-block samples adding up to 2016 blocks, where each sample is calculated using a constant-weight sum of 126 blocks recorde every m_height % 126 == 0. the difference of the slow and fast target are each taken compared to the rolling 12-block target, and are multiplied by 1/64 and 1/2 respectively, before all beeing added to the 12-block target to give the new Target Difficulty. currently all 3 targets use MTP-6, but perhaps this is not necessary for all but the rolling 12-block target.
int Chain::swordwt126Algo() const // 16 weighted 126 block groups = 2016 blocks // use mtp_6?
{
    //qDebug() << "swordwt126Algo " << QTime::currentTime().toString() << m_height << "@" << m_difficulty;
    const qint64 targetTimeSpan12 = 12 * 600 * 1000 / 6000; // we aim to be a 6000 times faster than real-time.
    const int ProofOfWorkLimit = 1000;
    if (m_height < 12)
        return ProofOfWorkLimit;
    int newDifficulty = m_difficulty;
    if (m_height % 6 == 0) {//overlapping adjustments based on previous 12 blocks every 6 blocks
        qDebug() << "swordwt126Algo " << QTime::currentTime().toString() << m_height << "@" << m_difficulty;
        const int diffmtp_6 = m_blockDifficulties.at(m_blockDifficulties.count() - 7);
        const int diffmtp_12 = m_blockDifficulties.at(m_blockDifficulties.count() - 13);
        const int average2diffs = (diffmtp_6 + diffmtp_12) / 2;
        const qint64 mtp_6 = m_blockTimeStamps.at(m_blockTimeStamps.count() - 7);
        const qint64 mtp_12 = m_blockTimeStamps.at(m_blockTimeStamps.count() - 7 - 12);
        const int64_t timespan12 = mtp_6 - mtp_12;
        newDifficulty = average2diffs * targetTimeSpan12 / timespan12;
        //qDebug() << newDifficulty;
        // take into account fast126 target and slow2016 target
        const int fasttarget = swordwt126fastT();
        const int fastdifference = fasttarget - newDifficulty;
        const int fastmod = fastdifference >> 1;//1/2;
        const int slowtarget = m_baseDifficulty;
        const int slowdifference = slowtarget - newDifficulty;
        const int slowmod = slowdifference >> 8;//1/64;
        qDebug() << "target: " << newDifficulty << " fast: " << fasttarget << " slow: " << slowtarget;
        qDebug() << "target: " << newDifficulty << " fdiff: " << fastdifference << " sdiff: " << slowdifference;
        qDebug() << "target: " << newDifficulty << " fmod: " << fastmod << " smod: " << slowmod;
        //newDifficulty = newDifficulty;
        //      //no modification new 6 block target
        //newDifficulty = newDifficulty + fastmod;
        //      //new 6 block target modified by 126-block weighted-time target
        newDifficulty = newDifficulty + fastmod + slowmod;
        //      //new 6 block target modified by 126-block weighted-time target & 16*126-block weighted-time target
    }
    return std::max(newDifficulty, ProofOfWorkLimit);
}

int Chain::swordwt126fastT() const // 21 weighted-time 6 block groups = 126 blocks // use mtp_6?
{
    const qint64 targetTimeSpan21 = 126 * 600 * 1000 / 6000; // we aim to be a 6000 times faster than real-time.
    const int ProofOfWorkLimit = 1000;
    if (m_height % 6 == 0 && m_height >= 139) {
        qint64 timeSpan21 = 0;
        qint64 prior_timestamp = m_blockTimeStamps.at(m_blockTimeStamps.count() - 126 - 13);
        qint64 target_21 = 0;
        for (int i=1; i<=21; i++) {
            const qint64 target_i = m_blockDifficulties.at(m_blockDifficulties.count() - 126 - 7 + 6 * i);
            const qint64 time_i = m_blockTimeStamps.at(m_blockTimeStamps.count() - 126 - 7 + 6 * i);
            const qint64 d_time = time_i - prior_timestamp;
            prior_timestamp = time_i;
            timeSpan21 = timeSpan21 + d_time * (i + 1);
            target_21 = target_21 + target_i;
        }
        target_21 = target_21 / 21;
        timeSpan21 = timeSpan21 * 2 / (21 * 21 + 21) * 21;
        int newDifficulty = target_21 * targetTimeSpan21 / timeSpan21;
        //return newDifficulty;
        return std::max(newDifficulty, ProofOfWorkLimit);
    }else
        return 0;
}

int Chain::swordcw126fastT() const // 21 constant weight 6 block groups = 126 blocks // use mtp_6?
{
    const qint64 targetTimeSpan21 = 126 * 600 * 1000 / 6000; // we aim to be a 6000 times faster than real-time.
    const int ProofOfWorkLimit = 1000;
    if (m_height % 6 == 0 && m_height >= 2 * 126) {
        const qint64 lasttime_mtp6 = m_blockTimeStamps.at(m_blockTimeStamps.count() - 7);
        const qint64 lasttime126_mtp12 = m_blockTimeStamps.at(m_blockTimeStamps.count() - 126 - 13);
        int64_t timespan21 = lasttime_mtp6 - lasttime126_mtp12;
        //timespan21 = qBound(targetTimeSpan21 / 4, timespan21, targetTimeSpan21 * 4);//
        qint64 target_21 = 0;
        for (int i=1; i<=21; i++) {
            const qint64 target_i = m_blockDifficulties.at(m_blockDifficulties.count() - 126 - 7 + 6 * i);
            target_21 = target_21 + target_i;
        }
        target_21 = target_21 / 21;
        int newDifficulty = target_21 * targetTimeSpan21 / timespan21;
        //return newDifficulty;
        return std::max(newDifficulty, ProofOfWorkLimit);
    }else
        return 0;
}

int Chain::swordwt126slowT() const // 16 weighted-time 126 block groups = 2016 blocks // use mtp_6?
{
    const qint64 targetTimeSpan16 = 2016 * 600 * 1000 / 6000; // we aim to be a 6000 times faster than real-time.
    const int ProofOfWorkLimit = 1000;
    if (m_height % 126 == 0 && m_height >= 2142) {
        qint64 timeSpan16 = 0;
        qint64 prior_timestamp = m_blockTimeStamps.at(m_blockTimeStamps.count() - 2016 - 13);
        qint64 target_16 = 0;
        for (int i=1; i<=16; i++) {
            const qint64 target_i = m_blockDifficultiescw126.at(m_blockDifficultiescw126.count() - 16 + i);//16x126blocks = 2016
            const qint64 time_i = m_blockTimeStamps.at(m_blockTimeStamps.count() - 2016 - 7 + 126 * i);
            const qint64 d_time = time_i - prior_timestamp;
            prior_timestamp = time_i;
            timeSpan16 = timeSpan16 + d_time * (i + 1);
            target_16 = target_16 + target_i;
        }
        target_16 = target_16 / 16;
        timeSpan16 = timeSpan16 * 2 / (16 * 16 + 16) * 16;
        //timeSpan16 = qBound(timeSpan16 / 4, timeSpan16, timeSpan16 * 4);//
        int newDifficulty = target_16 * targetTimeSpan16 / timeSpan16;
        if (newDifficulty < 0) {
            newDifficulty = 0;
        }
        //return newDifficulty;
        return std::max(newDifficulty, ProofOfWorkLimit);
    }else
        return 0;
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

    double timeTillNextBlock = m_difficulty / hashPower;
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
