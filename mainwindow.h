#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "processmonitor.h"

QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE

class QTreeWidgetItem;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void addMonitorProcess(const QString& name);
    void removeProcess(const QString& name);

private:
    Ui::MainWindow *ui;

    QTreeWidgetItem* treeFindProcessItem(const QString& name);

private slots:
    void treeAddProcess(const QString& name);
    void treeChangeProcessState(const QString& name, DWORD pid, bool running);
    void treeAddSub(const QString& processName, const QMap<DWORD, QString>& info);
    void treeRemoveSub(const QString& processName, const QList<DWORD>& info);
};
#endif // MAINWINDOW_H
