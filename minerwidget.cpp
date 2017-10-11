#include "minerwidget.h"

#include "ui_miner.h"
#include "miner.h"

MinerWidget::MinerWidget(Miner *miner, QWidget *parent) : QWidget(parent),
    ui(new Ui::Miner),
    m_miner(miner)
{
    ui->setupUi(this);
    connect (ui->dial, SIGNAL(valueChanged(int)), this, SLOT(sliderMoved(int)));
    ui->dial->setValue(miner->hashPower());
    connect (ui->toolButton, SIGNAL(clicked()), this, SLOT(deletePressed()));
}

MinerWidget::~MinerWidget()
{
    delete ui;
}

void MinerWidget::deletePressed()
{
    emit deletePressed(m_miner);
    deleteLater();
}

void MinerWidget::sliderMoved(int value)
{
    if (m_miner.data())
        m_miner.data()->setHashPower(value);
}
