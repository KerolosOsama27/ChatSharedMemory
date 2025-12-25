#pragma once

#include <QMainWindow>
#include <thread>
#include <windows.h>
#include <string>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

extern HANDLE g_hMutex;
extern char* g_pBuf;
extern bool g_running;
extern std::string g_myLastMessage;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButtonSend_clicked();

private:
    Ui::MainWindow *ui;
    void startReaderThread();
    std::thread readerThread;
    void ReadThread(); // نفس دالة ReadThread القديمة بس تطبع في QTextEdit
};
