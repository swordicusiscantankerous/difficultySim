#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "chain.h"

#include <QMainWindow>
#include <QHBoxLayout>

namespace Ui {
class MainWindow;
}
class Miner;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void addMiner();
    void newBlockFound(int height);

private:
    Ui::MainWindow *ui;
    QHBoxLayout *m_minersLayout;

    Chain m_chain;
};

#endif // MAINWINDOW_H
