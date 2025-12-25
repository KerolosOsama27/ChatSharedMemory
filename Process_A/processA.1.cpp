#include <windows.h>
#include <iostream>
#include <string>
#include <mutex>
#include <thread>
#include <ctime>

using namespace std;

mutex m;


//creATING MUTEX
HANDLE g_hMutex = nullptr;

// Global variables that what can threads arrive
char* g_pBuf = nullptr;
bool g_running = true;

// for not reading the message what the process sended
string g_myLastMessage = "";

// Function for just reading
void ReadThread() {
    string lastMessage = "";

   // cout << "\n[Thread began reading ]" << endl;

    while (g_running) {
        if (g_pBuf != nullptr && g_hMutex != nullptr) {

            DWORD waitResult = WaitForSingleObject(g_hMutex, INFINITE);

            if (waitResult == WAIT_OBJECT_0) {
                // 2. مش فاهم 
                string currentMessage = g_pBuf;

                // 3. release mutex after reading
                ReleaseMutex(g_hMutex);

                // if the message changed
                if (currentMessage != lastMessage && !currentMessage.empty() && currentMessage != g_myLastMessage) {
                 

                    if (!currentMessage.empty()) {
                        
                        time_t now = time(nullptr);
                        tm timeinfo;
                        localtime_s(&timeinfo, &now);
                        char timeStr[20];
                        strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);

                        cout << "\n[ " << timeStr << " ] Process B : " << currentMessage << endl;
                        cout << "Process A : " << flush;
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
    cout << "=== Process A ===" << endl;
    cout << "PID: " << GetCurrentProcessId() << endl << endl;

    g_hMutex = CreateMutex(
        NULL,              // default security attributes
        FALSE,             // initially not owned
        TEXT("MySharedMemoryMutex")  // mutex's name to share the neme with process B
    );

    if (g_hMutex == NULL) {
        cerr << "   Mutex. Error: " << GetLastError() << endl;
        return 1;
    }


    // 1. create shared memory object
    // CreateFileMapping(handle, security, protection, sizeHigh, sizeLow, name)
    HANDLE hMapFile = CreateFileMapping(
        INVALID_HANDLE_VALUE,    // using paging file
        NULL,                    // default security
        PAGE_READWRITE,          // read/write access
        0,                       // maximum object size (high-order DWORD)
        1024,                    // maximum object size (low-order DWORD) = 1KB
        TEXT("MySharedMemory")   // name of shared memory
    );

    if (hMapFile == NULL) {
        cerr << "can't get shared memory. Error: " << GetLastError() << endl;
        return 1;
    }

    cout << "the shared memory created sccessfully" << endl;


    // 2 create ID for the location the shared memory
    // MapViewOfFile(handle, access, offsetHigh, offsetLow, size)
    char* pBuf = (char*)MapViewOfFile(
        hMapFile,           // handle to map object
        FILE_MAP_ALL_ACCESS, // read/write permission
        0,                  // offset high
        0,                  // offset low
        1024                // number of bytes to map
    );

    if (pBuf == NULL) {
        cerr << "can't get connect shared memory. Error: " << GetLastError() << endl;
        CloseHandle(hMapFile);
        CloseHandle(g_hMutex);
        return 1;
    }

    cout << " connected shared memory sccessfully " << endl << endl;
    cout << "Memory Address: " << (void*)pBuf << endl << endl;

    g_pBuf = pBuf;

    // thread for reading
    thread readerThread(ReadThread);
  
    
    // 3. writing in shared memory
    string message; 
    while (true) {
        cout << "Process A :";
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
            // 
            strcpy_s(pBuf, 1024, message.c_str());

            //release the Mutex
            ReleaseMutex(g_hMutex);
            g_myLastMessage = message;

          //  cout << " : \"" << message << "\"" << endl;
        }

       // string currentMessage = pBuf;

        // waiting
      //  Sleep(500);
    }

    // 5.c joining thread 
    if (readerThread.joinable()) {
        readerThread.join();
    }

    

    // 5. clear
    UnmapViewOfFile(pBuf);  //unconnect shared memory with the program
    CloseHandle(hMapFile);   // closing handle
    CloseHandle(g_hMutex);  // close mutex 

    cout << "cleared successuflly" << endl;

    return 0;
}