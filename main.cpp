#include <Windows.h>
#include <string>
#include <iostream>
#include <TlHelp32.h>
#include <thread>
#include <cstdlib>

void SetConsoleColor(WORD color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

void changeConsoleTitle(const std::string& title) {
    SetConsoleTitle(title.c_str());
}

void titleChangeThread() {
    std::string titleCharacters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()_-+=";

    size_t index = 0;

    while (true) {
        std::rotate(titleCharacters.begin(), titleCharacters.begin() + index, titleCharacters.end());

        std::string newTitle = titleCharacters.substr(0, 6);

        changeConsoleTitle(newTitle);
        Sleep(10);

        index = (index + 1) % titleCharacters.size();
    }
}

DWORD find_process(const char* process_name) {
    DWORD process_id = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (snapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 process_entry;
        process_entry.dwSize = sizeof(process_entry);

        if (Process32First(snapshot, &process_entry)) {
            do {
                if (_stricmp(process_entry.szExeFile, process_name) == 0) {
                    process_id = process_entry.th32ProcessID;
                    break;
                }
            } while (Process32Next(snapshot, &process_entry));
        }

        CloseHandle(snapshot);
    }

    return process_id;
}

int main() {
    std::thread titleThread(titleChangeThread);


    char process_name[256];
    char dll_path[MAX_PATH];


    std::cout << "Enter the process name: ";
    std::cin.getline(process_name, sizeof(process_name));
    std::cout << "Enter the DLL path: ";
    std::cin.getline(dll_path, sizeof(dll_path));

    char full_dll_path[MAX_PATH];
    GetFullPathNameA(dll_path, MAX_PATH, full_dll_path, nullptr);

    auto process = find_process(process_name);
    if (!process) {
        std::cout << "Process not found." << std::endl;
        std::cin.get();
        return 0;
    } else {
        std::cout << "Process found." << std::endl;
        Sleep(2000);

        auto handle = OpenProcess(PROCESS_ALL_ACCESS, 0, process);
        if (handle && handle != INVALID_HANDLE_VALUE) {
            auto module = LoadLibraryA("ntdll.dll");
            const auto entry = GetProcAddress(module, "NtSetSystemTime");
            if (entry) {
                char byte[5];
                memcpy(byte, entry, 5);
                WriteProcessMemory(handle, entry, byte, 5, nullptr);
            }

            auto* alls = VirtualAllocEx(handle, nullptr, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            WriteProcessMemory(handle, alls, full_dll_path, strlen(full_dll_path) + 1, nullptr);

            auto thread = CreateRemoteThread(handle, nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(LoadLibraryA), alls, 0, nullptr);
            Sleep(5);
            CloseHandle(thread);
            CloseHandle(handle);
            Sleep(5);
        }
    }

    titleThread.join();

    return 0;
}
