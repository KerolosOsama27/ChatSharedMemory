#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QString>
#include <QMetaObject>
#include <QThread>
#include <QTime>
#include <windows.h>
#include <string>
#include <thread>
#include <mutex>
#include <cstring>

std::mutex m;

// global variables زي الكود القديم
HANDLE g_hMutex = nullptr;
char* g_pBuf = nullptr;
bool g_running = true;
std::string g_myLastMessage = "";

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->textEditChat->setReadOnly(true);

    // فتح Mutex و Shared Memory زي الكود القديم
    g_hMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, TEXT("MySharedMemoryMutex"));
    if (!g_hMutex) {
        ui->textEditChat->append("Failed to open mutex!");
        return;
    }

    HANDLE hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, TEXT("MySharedMemory"));
    if (!hMapFile) {
        ui->textEditChat->append("Failed to open shared memory!");
        return;
    }

    char* pBuf = (char*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 1024);
    if (!pBuf) {
        ui->textEditChat->append("Failed to map view of file!");
        CloseHandle(hMapFile);
        CloseHandle(g_hMutex);
        return;
    }

    g_pBuf = pBuf;

    startReaderThread();
}

MainWindow::~MainWindow()
{
    g_running = false;
    if (readerThread.joinable())
        readerThread.join();

    UnmapViewOfFile(g_pBuf);
    CloseHandle(g_hMutex);
    delete ui;
}

void MainWindow::ReadThread()
{
    std::string lastMessage = "";

    while (g_running) {
        if (g_pBuf && g_hMutex) {
            DWORD waitResult = WaitForSingleObject(g_hMutex, INFINITE);
            if (waitResult == WAIT_OBJECT_0) {
                std::string currentMessage = g_pBuf;
                ReleaseMutex(g_hMutex);

                if (!currentMessage.empty() && currentMessage != lastMessage && currentMessage != g_myLastMessage) {
                    QTime time = QTime::currentTime();
                    QString displayMsg = QString("[%1] Process A : %2").arg(time.toString("HH:mm:ss")).arg(QString::fromStdString(currentMessage));

                    QMetaObject::invokeMethod(ui->textEditChat, "append", Qt::QueuedConnection, Q_ARG(QString, displayMsg));

                    lastMessage = currentMessage;
                }
            }
        }
        QThread::msleep(500);
    }
}

void MainWindow::startReaderThread()
{
    readerThread = std::thread([this]() { ReadThread(); });
}

void MainWindow::on_pushButtonSend_clicked()
{
    QString msg = ui->lineEditMessage->text();
    if (msg.isEmpty())
        return;

    std::string message = msg.toStdString();

    if (g_pBuf && g_hMutex) {
        DWORD waitResult = WaitForSingleObject(g_hMutex, INFINITE);
        if (waitResult == WAIT_OBJECT_0) {
            strcpy_s(g_pBuf, 1024, message.c_str());
            ReleaseMutex(g_hMutex);
            g_myLastMessage = message;

            QString displayMsg = QString("Process B : %1").arg(msg);
            ui->textEditChat->append(displayMsg);
            ui->lineEditMessage->clear();
        }
    }
}
