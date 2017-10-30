#include "mainwindow.h"
#include "minerwidget.h"
#include "ui_mainwindow.h"

#include <QActionGroup>
#include <QSlider>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_minersLayout = new QHBoxLayout(ui->minersForm);
    QWidget *dummy = new QWidget(ui->minersForm);
    m_minersLayout->addWidget(dummy, 100);

    QActionGroup *group = new QActionGroup(this);
    group->addAction(ui->actionSatoshi);
    group->addAction(ui->actionEDA);
    group->addAction(ui->actionNeil);
    ui->actionSatoshi->setChecked(true);

    connect(ui->actionNew_Miner, SIGNAL(triggered(bool)), this, SLOT(addMiner()));
    connect (ui->action_Quit, SIGNAL(triggered(bool)), QGuiApplication::instance(), SLOT(quit()));
    connect (ui->action_Pause, SIGNAL(triggered(bool)), &m_chain, SLOT(pause()));
    connect (ui->action_Pause, SIGNAL(triggered(bool)), ui->graphsFrame, SLOT(pause()));
    connect (&m_chain, SIGNAL(newBlock(int)), this, SLOT(newBlockFound(int)));
    connect (&m_chain, SIGNAL(difficultyChanged(int)), ui->graphsFrame, SLOT(setDifficulty(int)));
    connect (&m_chain, SIGNAL(hashpowerChanged(int)), ui->graphsFrame, SLOT(setHashrate(int)));
    connect (&m_chain, SIGNAL(newBlock(int)), ui->graphsFrame, SLOT(addBlock()));
    ui->graphsFrame->setDifficulty(m_chain.difficulty());
    setStatusBar(nullptr);

    connect (ui->zoomLevel, SIGNAL(valueChanged(int)), ui->graphsFrame, SLOT(setGraphZoom(int)));
    connect (group, SIGNAL(triggered(QAction*)), this, SLOT(algoChanged()));

    addMiner(100);
    addMiner(120);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::addMiner(int strength)
{
    Miner *miner = m_chain.appendNewMiner();
    miner->setHashPower(strength);
    MinerWidget *widget = new MinerWidget(miner, ui->minersForm);
    m_minersLayout->insertWidget(m_minersLayout->count() - 1, widget);
    connect (widget, SIGNAL(deletePressed(Miner*)), &m_chain, SLOT(deleteMiner(Miner*)));
}

void MainWindow::newBlockFound(int height)
{
    ui->progressBar->setValue(height % 2016);
    ui->blockIndex->setNum(height);
}

void MainWindow::algoChanged()
{
    Chain::AdjustmentAlgorithm algo;
    if (ui->actionSatoshi->isChecked())
        algo = Chain::Satoshi;
    else if (ui->actionEDA->isChecked())
        algo = Chain::EDA;
    else
        algo = Chain::Neil;
    m_chain.setAdjustmentAlgorithm(algo);
}
