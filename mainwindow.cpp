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
    group->addAction(ui->actiondEDA);
    group->addAction(ui->actiondEDAmodTom);
    group->addAction(ui->actionDeadalnix);
    group->addAction(ui->actioncw144);
    group->addAction(ui->actionwt144);
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
    if (ui->actionSatoshi->isChecked()){
        algo = Chain::Satoshi;
        ui->label_Algo->setText("Satoshi");
    }else if (ui->actionEDA->isChecked()){
        algo = Chain::EDA;
        ui->label_Algo->setText("EDA (Deadalnix)");
    }else if (ui->actionNeil->isChecked()){
        algo = Chain::Neil;
        ui->label_Algo->setText("k-1 (Neil)");
    }else if (ui->actiondEDA->isChecked()){
        algo = Chain::dEDA;
        ui->label_Algo->setText("dualEDA");
    }else if (ui->actiondEDAmodTom->isChecked()){
        algo = Chain::dEDAmodTom;
        ui->label_Algo->setText("dualEDAmod");
    }else if (ui->actionDeadalnix->isChecked()){
        algo = Chain::DeadalnixOld;
        ui->label_Algo->setText("Deadalnix (old)");
    }else if (ui->actioncw144->isChecked()){
        algo = Chain::cw144;
        ui->label_Algo->setText("cw144 (Deadalnix)");
    }else if (ui->actionwt144->isChecked()){
        algo = Chain::wt144;
        ui->label_Algo->setText("wt144 (TomH)");
    }
    m_chain.setAdjustmentAlgorithm(algo);
}
