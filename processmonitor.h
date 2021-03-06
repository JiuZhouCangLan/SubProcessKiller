#ifndef PROCESSMONITOR_H
#define PROCESSMONITOR_H

#include <Windows.h>
#include <tlhelp32.h>
#include <QObject>
#include <QTimer>
#include <QMap>
#include <QString>
#include <QSet>

class ProcessMonitor : public QObject
{
    Q_OBJECT
public:
    explicit ProcessMonitor(const QString& processName, QObject *parent = nullptr);

    /**
     * 获取子进程信息, 返回 Pid - 进程名称
    */
    QMap<DWORD, QString> getChildProcessInfo(const DWORD& dwParentProcessId);

    /**
     * 不算可靠, 进程结束之后, pid被重用之前, 都会返回上一次状态
    */
    static QString getNameFromPid(const DWORD& pid);

    /**
     * 不算可靠, 同一个名称的进程可能有多个, 暂时只返回找到的第一个
    */
    static DWORD getPidFromName(const QString& processName);
    static QList<PROCESSENTRY32> getAllProcessEntry();

    void start();

    int checkInterval() const;
    void setCheckInterval(const int& msec);

    const QString &processName() const;
    void setProcessName(const QString &newProcessName);

    /**
     * 直接添加一个进程名, 将其视为子进程, 用于由调用关系启动, 但是不是子进程的情况
    */
    void addBindSubProcess(const QStringList& subProcessName);

    QMap<DWORD, QString> lastRunningChildren();

signals:
    void processStateChanged(DWORD pid, bool running);
    void subProcessAdded(QMap<DWORD, QString> info);
    void subProcessRemoved(QList<DWORD> pid);

private:
    QString m_processName;
    QTimer m_checkTimer;
    bool m_lastState = false; // 当前进程运行状态
    QMap<DWORD, QString> m_lastChildrenInfo; // 当前子进程信息
    QSet<QString> m_bindedSubProcessName; // 直接视为子进程的进程名称

private slots:
    void runCheck();
};

bool TerminateProcess(DWORD pid);

#endif // PROCESSMONITOR_H
