#include <windows.h>
#include <iostream>
#include <string>
#include <thread>
#include <ctime>

using namespace std;

//create global mutex for main and readthread
HANDLE g_hMutex = nullptr;

// for readThread in global
char* g_pBuf = nullptr;
bool g_running = true;

// for not reading the message what the process sended
string g_myLastMessage = "";

// Function thread for reading
void ReadThread() {
    string lastMessage = "";

    //cout << "\n[Thread began reading ]" << endl;

    while (g_running) {
        if (g_pBuf != nullptr && g_hMutex != nullptr) {


            //  require mutex for readind
            DWORD waitResult = WaitForSingleObject(g_hMutex, INFINITE);

            if (waitResult == WAIT_OBJECT_0) {
                string currentMessage = g_pBuf;

                ReleaseMutex(g_hMutex);

                // if the message changed
                if (currentMessage != lastMessage && !currentMessage.empty() && currentMessage != g_myLastMessage) {
                    if (!currentMessage.empty()) {

                        time_t now = time(nullptr);
                        tm timeinfo;
                        localtime_s(&timeinfo, &now);
                        char timeStr[20];
                        strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);

                        cout << "\n[ " << timeStr << " ] Process A : " << currentMessage << endl;
                        cout << "Process B : " << flush;
                    }
                    lastMessage = currentMessage;
                }
            }
        }

        // waiting
        Sleep(500);
    }
}

int main() {
    cout << "=== Process B ===" << endl;
    cout << "PID: " << GetCurrentProcessId() << endl << endl;

    g_hMutex = OpenMutex(
        MUTEX_ALL_ACCESS,  // request full access
        FALSE,             // handle not inheritable
        TEXT("MySharedMemoryMutex")  // same Mutex what is with process A
    );

    if (g_hMutex == NULL) {
        cerr << "Mutex. Error: " << GetLastError() << endl;
        return 1;
    }

    // 1.open shared memory what proccess A created
    // OpenFileMapping(access, inherit, name)
    HANDLE hMapFile = OpenFileMapping(
        FILE_MAP_ALL_ACCESS,   // read/write access
        FALSE,                 // do not inherit the name
        TEXT("MySharedMemory") //  shared memory (same Writer)
    );

    if (hMapFile == NULL) {
        cerr << "can't connect shared memory. Error: " << GetLastError() << endl;
      //  cerr << " Write!" << endl;
        return 1;
    }

    cout << "connected the shared memory succesfully !" << endl;

    // 2. connect the cpp file with the memory shared memory 
    char* pBuf = (char*)MapViewOfFile(
        hMapFile,           // handle to map object
        FILE_MAP_ALL_ACCESS, // read/write permission
        0,                  // offset high
        0,                  // offset low
        1024                // number of bytes to map
    );

    if (pBuf == NULL) {
        cerr << "   can't connect shared memory. Error: " << GetLastError() << endl;
        CloseHandle(hMapFile);
        CloseHandle(g_hMutex);
        return 1;
    }

    g_pBuf = pBuf;


    // 3. the thread what reading
    thread readerThread(ReadThread);

    string message;
    while (true) {

        cout << "Process B  : ";
        getline(cin, message);

        if (message == "exit") {
            cout << "Getting out ..." << endl;
            g_running = false;
            break;
        }

        if (message.empty()) {
            continue;
        }

        DWORD waitResult = WaitForSingleObject(g_hMutex, INFINITE);


        if (waitResult == WAIT_OBJECT_0) {
            // writting message in shared memory
            strcpy_s(pBuf, 1024, message.c_str());

            ReleaseMutex(g_hMutex);
            g_myLastMessage = message;

            // waiting
            //Sleep(500);
        }
    }

    if (readerThread.joinable()) {
        readerThread.join();
    }

    // 6. clear
    UnmapViewOfFile(pBuf);
    CloseHandle(hMapFile);
    CloseHandle(g_hMutex);

   // cout << "cleared succesfully...." << endl;

    return 0;
}