#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QtDebug>
#include <QRegularExpression>
#include <QFileInfo>
#include <QAction>
#include <algorithm>

constexpr auto runningStr = "运行中";
constexpr auto unstartStr = "未运行";
constexpr int parentProcessType = 0;
constexpr int subProcessType = 1;
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle(PROJECT_NAME);

    ui->edt_name->setValidator(new QRegularExpressionValidator(QRegularExpression("[^\\s]*")));
    connect(ui->btn_add, &QPushButton::clicked, this, [ = ]() {
        const QString name = ui->edt_name->text();
        if(!name.isEmpty()) {
            addMonitorProcess(name);
        }
        ui->edt_name->clear();
    });
    ui->tree_monitor->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->tree_monitor->setContextMenuPolicy(Qt::ActionsContextMenu);
    auto removeAction = new QAction("Remove");
    ui->tree_monitor->addAction(removeAction);
    connect(removeAction, &QAction::triggered, this, [ = ]() {
        auto item = ui->tree_monitor->currentItem();
        if(item != nullptr && item->type() == parentProcessType) {
            removeProcess(item->text(0));
        }
    });
    connect(ui->tree_monitor, &QTreeWidget::currentItemChanged, this, [ = ](QTreeWidgetItem * current, QTreeWidgetItem * previous) {
        Q_UNUSED(previous)
        if(current == nullptr || current->type() != parentProcessType) {
            removeAction->setDisabled(true);
        } else {
            removeAction->setDisabled(false);
        }
    });

    if(qApp->arguments().size() > 1) {
        ui->edt_name->setText(qApp->arguments()[1]);
        ui->btn_add->click();
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::addMonitorProcess(const QString &name)
{
    QString actualName = name;
    if(QFileInfo(actualName).suffix() != "exe") {
        actualName.append(".exe");
    }

    auto monitor = new ProcessMonitor(actualName, this);
    connect(monitor, &ProcessMonitor::processStateChanged, this, [ = ](DWORD pid, bool running) {
        treeChangeProcessState(actualName, pid, running);
        if(!running) {
            const auto runningChildren = monitor->lastRunningChildren();
            auto iterator = runningChildren.keyBegin();
            while (iterator != runningChildren.keyEnd()) {
                TerminateProcess(*iterator);
                ++iterator;
            }
        }
    });
    connect(monitor, &ProcessMonitor::subProcessAdded, this, [ = ](QMap<DWORD, QString> info) {
        treeAddSub(actualName, info);
    });
    connect(monitor, &ProcessMonitor::subProcessRemoved, this, [ = ](QList<DWORD> pid) {
        treeRemoveSub(actualName, pid);
    });
    treeAddProcess(actualName);
    monitor->start();
}

void MainWindow::removeProcess(const QString &name)
{
    delete treeFindProcessItem(name);
}

QTreeWidgetItem *MainWindow::treeFindProcessItem(const QString &name)
{
    for (int i = 0, count = ui->tree_monitor->topLevelItemCount(); i < count ; ++i ) {
        if(ui->tree_monitor->topLevelItem(i)->text(0) == name) {
            return ui->tree_monitor->topLevelItem(i);
        }
    }
    return nullptr;
}

void MainWindow::treeAddProcess(const QString &name)
{
    auto item = new QTreeWidgetItem({name, "", unstartStr}, parentProcessType);
    ui->tree_monitor->addTopLevelItem(item);
    item->setExpanded(true);
}

void MainWindow::treeChangeProcessState(const QString &name, DWORD pid, bool running)
{
    auto item = treeFindProcessItem(name);
    if(item != nullptr) {
        item->setText(1, QString::number(pid));
        item->setText(2, running ? runningStr : unstartStr);
    }
}

void MainWindow::treeAddSub(const QString &processName, const QMap<DWORD, QString> &info)
{
    auto item = treeFindProcessItem(processName);
    if(item == nullptr) {
        return;
    }

    auto iterator = info.keyValueBegin();
    while (iterator != info.keyValueEnd()) {
        const auto &pid = iterator->first;
        const auto &name = iterator->second;
        item->addChild(new QTreeWidgetItem({name, QString::number(pid)}, subProcessType));
        ++iterator;
    }

    item->sortChildren(0, Qt::AscendingOrder);
}

void MainWindow::treeRemoveSub(const QString &processName, const QList<DWORD> &pidList)
{
    auto item = treeFindProcessItem(processName);
    if(item == nullptr) {
        return;
    }

    for (const auto &pid : pidList) {
        for (int i = 0; i < item->childCount() ; ++i) {
            if(item->child(i)->text(1) == QString::number(pid)) {
                delete item->child(i);
                break;
            }
        }
    }

}

