#include "processmonitor.h"
#include <Psapi.h>
#include <string>
#include <QtDebug>
#include <QFileInfo>

constexpr int defaultCheckInterval = 500;
ProcessMonitor::ProcessMonitor(const QString &processName, QObject *parent) : QObject(parent), m_processName(processName)
{
    setCheckInterval(defaultCheckInterval);
    connect(&m_checkTimer, &QTimer::timeout, this, &ProcessMonitor::runCheck);
}

QMap<DWORD, QString> ProcessMonitor::getChildProcessInfo(const DWORD &dwParentProcessId)
{
    QMap<DWORD, QString> childrenInfo;
    auto allEntry = getAllProcessEntry();

    for (const auto& entry : qAsConst(allEntry)) {
        if (entry.th32ParentProcessID == dwParentProcessId) { //判断如果父id与其pid相等，
            DWORD dwProcessID = entry.th32ProcessID;
            childrenInfo.insert(dwProcessID, QFileInfo(entry.szExeFile).fileName());
        }
    }

    return childrenInfo;
}

QString ProcessMonitor::getNameFromPid(const DWORD &pid)
{
    HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    QString name;
    if(process) {
        TCHAR buf[BUFSIZ];
        GetProcessImageFileName(process, buf, BUFSIZ);
        name = QFileInfo(buf).fileName();
        CloseHandle(process);
    }
    return name;
}

DWORD ProcessMonitor::getPidFromName(const QString &processName)
{
    DWORD pid = 0;
    auto allEntry = getAllProcessEntry();
    for(const auto& entry : qAsConst(allEntry)) {
        if(entry.szExeFile == processName) {
            pid = entry.th32ProcessID;
        }
    }
    return pid;
}

QList<PROCESSENTRY32> ProcessMonitor::getAllProcessEntry()
{
    QList<PROCESSENTRY32> entryList;

    //进行一个进程快照
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        qDebug() << ("进程快照失败");
        return entryList;
    }

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(pe);
    BOOL bProcess = Process32First(hProcessSnap, &pe);
    //遍历所有进程
    while (bProcess) {
        entryList.append(pe);
        bProcess = Process32Next(hProcessSnap, &pe);
    }
    CloseHandle(hProcessSnap);
    return entryList;
}

void ProcessMonitor::start()
{
    m_checkTimer.start();
}

void ProcessMonitor::setCheckInterval(const int& msec)
{
    m_checkTimer.setInterval(msec);
}

int ProcessMonitor::checkInterval() const
{
    return m_checkTimer.interval();
}

const QString &ProcessMonitor::processName() const
{
    return m_processName;
}

void ProcessMonitor::setProcessName(const QString &newProcessName)
{
    m_processName = newProcessName;
}

QMap<DWORD, QString> ProcessMonitor::lastRunningChildren()
{
    return m_lastChildrenInfo;
}

void ProcessMonitor::runCheck()
{
    DWORD pid = getPidFromName(m_processName);
    bool state = pid != 0;
    if(state != m_lastState) {
        emit processStateChanged(pid, state);
        m_lastState = state;
    }

    QMap<DWORD, QString> newChildren;
    QList<DWORD> removedChildren;
    if(state) {
        auto curChildrenInfo = getChildProcessInfo(pid);
        if(curChildrenInfo != m_lastChildrenInfo) {
            // 存在pid重用可能性, 需要对比前后是否一致
            QSet<DWORD> newPid = QSet<DWORD>(curChildrenInfo.keyBegin(), curChildrenInfo.keyEnd());
            QSet<DWORD> lastPid = QSet<DWORD>(m_lastChildrenInfo.keyBegin(), m_lastChildrenInfo.keyEnd());
            auto samePid = lastPid.intersect(newPid);
            for(const auto& pid : qAsConst(samePid)) {
                if(curChildrenInfo.value(pid) != m_lastChildrenInfo.value(pid)) {
                    removedChildren.append(pid);
                    newChildren.insert(pid, curChildrenInfo.value(pid));
                }
            }

            auto addedPid = newPid - lastPid;
            for(const auto& pid : qAsConst(addedPid)) {
                newChildren.insert(pid, curChildrenInfo.value(pid));
            }

            auto removedPid = lastPid - newPid;
            for(const auto& pid : qAsConst(removedPid)) {
                removedChildren.append(pid);
            }
            m_lastChildrenInfo = curChildrenInfo;
        }
    } else {
        auto iterator = m_lastChildrenInfo.keyValueBegin();
        while (iterator != m_lastChildrenInfo.keyValueEnd()) {
            const auto &pid = iterator->first;
            const auto &name = iterator->second;

            auto entrys = getAllProcessEntry();
            auto entryIterator = std::find_if(entrys.begin(), entrys.end(), [pid](const PROCESSENTRY32 & entry)->bool {
                if(entry.th32ProcessID == pid)
                {
                    return true;
                }
                return false;
            });
            if(entryIterator == entrys.end() || *entryIterator->szExeFile != name) {
                removedChildren.append(pid);
            }

            ++iterator;
        }

        for(const auto& pid : removedChildren) {
            m_lastChildrenInfo.remove(pid);
        }
    }


    if(!removedChildren.isEmpty()) {
        emit subProcessRemoved(removedChildren);
    }
    if(!newChildren.isEmpty()) {
        emit subProcessAdded(newChildren);
    }
}

bool TerminateProcess(DWORD pid)
{
    HANDLE process = OpenProcess(PROCESS_TERMINATE, false, pid);
    if(process != nullptr) {
        return TerminateProcess(process, 0);
    } else {
        return false;
    }
}
