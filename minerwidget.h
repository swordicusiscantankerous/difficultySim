#ifndef MINERWIDGET_H
#define MINERWIDGET_H

#include <QPointer>
#include <QWidget>

namespace Ui {
class Miner;
}

class Miner;

class MinerWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MinerWidget(Miner *miner, QWidget *parent = nullptr);
    ~MinerWidget();

signals:
    void deletePressed(Miner *);

private slots:
    void deletePressed();
    void sliderMoved(int);

private:
    Ui::Miner *ui;
    QPointer<Miner> m_miner;
};

#endif // MINERWIDGET_H
