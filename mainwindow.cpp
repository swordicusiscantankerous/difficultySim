#include "mainwindow.h"
#include "minerwidget.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_minersLayout = new QHBoxLayout(ui->minersForm);
    QWidget *dummy = new QWidget(ui->minersForm);
    m_minersLayout->addWidget(dummy, 100);
    addMiner();

    connect(ui->actionNew_Miner, SIGNAL(triggered(bool)), this, SLOT(addMiner()));
    connect (ui->action_Quit, SIGNAL(triggered(bool)), QGuiApplication::instance(), SLOT(quit()));
    connect (ui->action_Pause, SIGNAL(triggered(bool)), &m_chain, SLOT(pause()));
    connect (&m_chain, SIGNAL(newBlock(int)), this, SLOT(newBlockFound(int)));
    connect (&m_chain, SIGNAL(difficultyChanged(int)), ui->graphsFrame, SLOT(setDifficulty(int)));
    ui->graphsFrame->setDifficulty(m_chain.difficulty());
    setStatusBar(nullptr);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::addMiner()
{
    Miner *miner = m_chain.appendNewMiner();
    MinerWidget *widget = new MinerWidget(miner, ui->minersForm);
    m_minersLayout->insertWidget(m_minersLayout->count() - 1, widget);
    connect (widget, SIGNAL(deletePressed(Miner*)), &m_chain, SLOT(deleteMiner(Miner*)));
}

void MainWindow::newBlockFound(int height)
{
    ui->progressBar->setValue(height % 2016);
    ui->blockIndex->setNum(height);
}
