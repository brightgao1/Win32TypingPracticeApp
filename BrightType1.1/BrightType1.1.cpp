#include "framework.h"
#include <shlobj_core.h>
#include "BrightType1.1.h"
#include <d2d1.h>
#include <dwrite.h>
#pragma comment(lib, "d2d1")
#pragma comment(lib, "dwrite")

#include <fstream>
#include <vector>
#include <array>
#include <deque>
#include <map>
#include <string>
#include <random>
#include <chrono>
#include <algorithm>

#define MAX_LOADSTRING 100
#define settingsID 32771

using namespace std::chrono_literals;
void writeText(); void newText(bool retry, bool goback);

std::wstring resstr;
std::vector<std::wstring> quotes, words, fontFamilies; std::vector<int> stk;
std::mt19937 rng; int currQuoteInd, wordPtr = 0, third;
std::chrono::time_point<std::chrono::steady_clock> startTime, endTime;
float userFontSize; std::wstring userFontFamily; short userSML, userTheme;
bool started = false, typingErr = false, doneScreen = false;
double wpm = 0.0, cpm = 0.0; 

HINSTANCE hInst; ATOM settingsATOM;
WNDCLASSEXW wc; WNDPROC editproc;
HWND appWindow, typeHere, newTextButton, settingsWindow, screen, resStat, anotherOneButton, retryButton, settingsFontSizeEdit, settingsFontFamEdit;
HWND lenGroup, shortRadio, medRadio, longRadio, allRadio, themeGroup, lightRadio, darkRadio, retroRadio, prevButton;
WCHAR szTitle[MAX_LOADSTRING]; WCHAR szWindowClass[MAX_LOADSTRING];
HFONT editBoxFont, newTextFont;
HBRUSH whiteHBRUSH = CreateSolidBrush(RGB(255, 255, 255)), blueHBRUSH = CreateSolidBrush(RGB(0, 0, 171)), blackHBRUSH = CreateSolidBrush(RGB(0, 0, 0));
RECT app, eR;

DWRITE_TEXT_RANGE currRange;
ID2D1Factory *d2dFactory;
ID2D1HwndRenderTarget *rendTarget;
ID2D1SolidColorBrush *blackBrush, *redBrush, *whiteBrush, *blueBrush;
IDWriteFactory *writeFactory;
IDWriteTextFormat *currTextFormat; IDWriteTextLayout *currTextLayout;

struct preTree {
    std::vector<std::array<int, 27>> trie; std::vector<std::array<bool, 27>> end;
    std::map<std::array<int, 2>, std::wstring> rcs; int nxt = 0;
    void init(int lenSum) {
        std::array<int, 27> tempi; std::array<bool, 27> tempb;
        for (short i = 0; i < 27; i++) tempi[i] = 0, tempb[i] = false;
        trie.assign(lenSum + 5, tempi), end.assign(lenSum + 5, tempb);
    }
    void dfs(int curr) {
        for (short i = 0; i < 27; i++) {
            if (end[curr][i]) SendMessageW(settingsFontFamEdit, CB_ADDSTRING, 0, (LPARAM)rcs[{curr, i}].c_str()); 
            if (trie[curr][i]) dfs(trie[curr][i]);
        }
    }
    void insert(std::wstring s) {
        int curr = 0;
        for (int i = 0; i < s.length(); i++) s[i] = (char)std::tolower(s[i]);
        for (int i = 0; i < s.length(); i++) {
            int ascii = s[i] - 'a';
            if (s[i] == ' ') ascii = 26;
            if (!trie[curr][ascii]) nxt++, trie[curr][ascii] = nxt;
            curr = trie[curr][ascii];
        }
        end[curr][s.back() - 'a'] = true, rcs[{curr, s.back() - 'a'}] = s;
    }
    int search(std::wstring s) {
        int curr = 0;
        for (int i = 0; i < s.length(); i++) {
            int ascii = s[i] - 'a';
            if (s[i] == ' ') ascii = 26;
            if (!trie[curr][ascii]) return -1;
            curr = trie[curr][ascii];
        }
        return curr;
    }
};
preTree fontTrie;

bool lenComparator(std::wstring& s1, std::wstring& s2) {
    return s1.length() < s2.length();
}
double thousandth(double& d) {
    return std::round(d * 100.0) / 100.0;
}
std::wstring remBackZeros(std::wstring str) {
    while (str.back() == '0') str.pop_back();
    return str;
}
void newQuoteInd() {
    if (currQuoteInd) stk.push_back(currQuoteInd);
    if (!userSML) currQuoteInd = rng() % quotes.size();
    else if (userSML == 1) currQuoteInd = rng() % third;
    else if (userSML == 2) currQuoteInd = (third + (rng() % third));
    else currQuoteInd = ((third * 2) + (rng() % third));
}
void prevQuote() {
    if (stk.empty()) return;
    else newText(false, true);
}
void changeTextFormat(std::wstring font, short weight, short style, short stretch, float sz) {
    if (currTextFormat) currTextFormat->Release(), currTextFormat = NULL;
    writeFactory->CreateTextFormat(font.c_str(), nullptr, (DWRITE_FONT_WEIGHT)weight, (DWRITE_FONT_STYLE)style, (DWRITE_FONT_STRETCH)stretch, sz, L"", &currTextFormat);
}
void start() {
    startTime = std::chrono::steady_clock::now(), SetWindowTextW(appWindow, L"   0"), SetTimer(appWindow, 100, 1000, NULL);
}
void done() {
    endTime = std::chrono::steady_clock::now();
    DestroyWindow(typeHere), DestroyWindow(newTextButton), DestroyWindow(prevButton), SetWindowTextW(appWindow, L"   Good job! :D"), doneScreen = true;
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    int millisecs = (int)(elapsed / 1ms); double seconds = millisecs / 1000.0, mins = seconds / 60;
    cpm = ((double)quotes[currQuoteInd].length()) / mins, wpm = cpm / 5.0;
    resstr = L"Seconds to type:            " + remBackZeros(std::to_wstring(thousandth(seconds))) + L"\n\nWords per minute:         " + remBackZeros(std::to_wstring(thousandth(wpm))) + L"\n\nCharacters per minute:  " + remBackZeros(std::to_wstring(thousandth(cpm)));
    GetClientRect(appWindow, &app), started = false, KillTimer(appWindow, 100);
    resStat = CreateWindowW(L"static", resstr.c_str(), WS_CHILD, app.left + 20, app.bottom - 100, 220, 200, appWindow, NULL, hInst, NULL);
    ShowWindow(resStat, SW_SHOW), UpdateWindow(resStat);
    anotherOneButton = CreateWindowW(L"Button", L"Another text! (Enter or Esc)", WS_CHILD | WS_BORDER | WS_CLIPSIBLINGS, app.right - 185, app.bottom - 55, 165, 35, appWindow, (HMENU)103, hInst, NULL);
    retryButton = CreateWindowW(L"Button", L"Retry same text (r or R)", WS_CHILD | WS_BORDER | WS_CLIPSIBLINGS, app.right - 185, app.bottom - 105, 165, 35, appWindow, (HMENU)105, hInst, NULL);
    SendMessageW(anotherOneButton, WM_SETFONT, (WPARAM)newTextFont, 0), SendMessageW(retryButton, WM_SETFONT, (WPARAM)newTextFont, 0);
    ShowWindow(anotherOneButton, SW_SHOW), ShowWindow(retryButton, SW_SHOW);
}
void ctrlBackspace() {
    int editLen = GetWindowTextLengthW(typeHere) + 1;
    wchar_t* buf = new wchar_t[editLen]; GetWindowTextW(typeHere, buf, editLen);
    std::wstring edit = buf; edit.pop_back();
    while (!edit.empty() && edit.back() == ' ') edit.pop_back();
    while (!edit.empty() && edit.back() != ' ') edit.pop_back();
    editLen = GetWindowTextLengthW(typeHere);
    SetWindowTextW(typeHere, edit.c_str()), SendMessageW(typeHere, EM_SETSEL, (WPARAM)editLen, (LPARAM)editLen);
}
void prevWord() {
    std::wstring temp = words[wordPtr - 1]; int len = temp.length();
    temp.pop_back(), wordPtr--, SetWindowTextW(typeHere, temp.c_str()), currTextLayout->SetUnderline(FALSE, currRange);
    currRange.startPosition -= words[wordPtr].length(), currRange.length = words[wordPtr].length() - 1, writeText();
    SendMessageW(typeHere, EM_SETSEL, len, len), SendMessageW(typeHere, EM_REPLACESEL, TRUE, (LPARAM)L"a");
}
void nxtWord() {
    wordPtr++;
    if (wordPtr >= words.size()) done();
    else {
        currTextLayout->SetUnderline(FALSE, currRange), SetWindowTextW(typeHere, L"");
        currRange.startPosition += words[wordPtr - 1].length();
        if (wordPtr == words.size() - 1) currRange.length = words[wordPtr].length();
        else currRange.length = words[wordPtr].length() - 1;
        writeText();
    }
}
void initUserSettings() {
    wchar_t* adBuffer = new wchar_t[600]; SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, adBuffer);
    std::wstring appDataPath = adBuffer;
    appDataPath += L"\\BrightType";
    if (GetFileAttributes(appDataPath.c_str()) == INVALID_FILE_ATTRIBUTES) CreateDirectoryW(appDataPath.c_str(), NULL);
    std::wstring settingsPath = appDataPath + L"\\settings.txt";
    userFontSize = 20.0, userFontFamily = L"arial", userSML = 0, userTheme = 0;
    std::wifstream settingsInput(settingsPath);
    if (settingsInput.good()) {
        std::wstring line;
        if (std::getline(settingsInput, line)) userFontSize = std::stof(line);
        if (std::getline(settingsInput, line)) userFontFamily = line;
        if (std::getline(settingsInput, line)) userSML = std::stoi(line);
        if (std::getline(settingsInput, line)) userTheme = std::stoi(line);
    }
    else {
        std::wofstream settingsOutput(settingsPath);
        settingsOutput << userFontSize << '\n' << userFontFamily << '\n' << userSML << '\n' << userTheme;
    }
    delete[] adBuffer;
}
void typingError() {
    if (!typingErr) {
        GetWindowRect(typeHere, &eR), MapWindowPoints(HWND_DESKTOP, appWindow, (LPPOINT)&eR, 2);
        D2D1_RECT_F dr; dr.left = eR.left, dr.right = eR.right, dr.top = eR.top, dr.bottom = eR.bottom;
        rendTarget->BeginDraw(), rendTarget->DrawRectangle(&dr, redBrush, 10.0f);
        rendTarget->EndDraw(), typingErr = true;
    }
}
void removeRed() {
    if (typingErr) {
        GetWindowRect(typeHere, &eR), MapWindowPoints(HWND_DESKTOP, appWindow, (LPPOINT)&eR, 2);
        D2D1_RECT_F dr; dr.left = eR.left, dr.right = eR.right, dr.top = eR.top, dr.bottom = eR.bottom;
        rendTarget->BeginDraw();
        if (!userTheme) rendTarget->DrawRectangle(&dr, whiteBrush, 18.0f), rendTarget->DrawRectangle(&dr, blackBrush, .2f);
        else if (userTheme == 1) rendTarget->DrawRectangle(&dr, blackBrush, 18.0f), rendTarget->DrawRectangle(&dr, whiteBrush, .2f);
        else rendTarget->DrawRectangle(&dr, blueBrush, 18.0f), rendTarget->DrawRectangle(&dr, whiteBrush, .2f);
        rendTarget->EndDraw(), typingErr = false;
    }
}
void handleKeypress() {
    int editLen = GetWindowTextLengthW(typeHere) + 1;
    if (editLen < 2) {
        removeRed(); return;
    }
    if (editLen - 1 > words[wordPtr].length()) {
        wchar_t* editStr = new wchar_t[editLen]; GetWindowTextW(typeHere, editStr, editLen);
        std::wstring edit = editStr;
        if ((int)edit.back() == 127) ctrlBackspace();
        else typingError();
    }
    else if (editLen - 1 == words[wordPtr].length()) {
        wchar_t *editStr = new wchar_t[editLen]; GetWindowTextW(typeHere, editStr, editLen);
        if (editStr == words[wordPtr]) nxtWord(), removeRed();
        else typingError();
        delete[] editStr;
    }
    else {
        wchar_t *editStr = new wchar_t[editLen]; GetWindowTextW(typeHere, editStr, editLen);
        std::wstring edit = editStr; bool isPre = true;
        if ((int)edit.back() == 127) ctrlBackspace();
        for (int i = 0; i < edit.length(); i++) {
            if (edit[i] != words[wordPtr][i]) {
                isPre = false; break;
            }
        }
        if (!isPre) typingError();
        else removeRed();
        delete[] editStr;
    }
}
void split() {
    std::wstring temp = L"", currQuote = quotes[currQuoteInd];
    for (int i = 0; i < currQuote.length(); i++) {
        if (currQuote[i] == ' ') temp += ' ', words.push_back(temp), temp = L"";
        else temp += currQuote[i];
    }
    if (temp.length() > 0) words.push_back(temp);
}
void typeHereFont() {
    editBoxFont = CreateFontW(userFontSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, userFontFamily.c_str());
}
void initNewTextFont() {
    newTextFont = CreateFontW(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, L"Arial");
}
void initSettingsWindow() {
    RECT rect; GetClientRect(screen, &rect), EnableWindow(appWindow, FALSE);
    settingsWindow = CreateWindowExW(0, MAKEINTATOM(settingsATOM), L"Settings", WS_POPUPWINDOW | WS_CAPTION | WS_CLIPCHILDREN, (rect.right - rect.left) / 3, (rect.bottom - rect.top) / 7, 380, 500, appWindow, 0, 0, 0);
    ShowWindow(settingsWindow, SW_SHOW), UpdateWindow(settingsWindow);
    HWND fontSiText = CreateWindowW(L"Static", L"Text size (10-90):", WS_CHILD, 10, 11, 120, 20, settingsWindow, NULL, NULL, NULL), fontFamText = CreateWindowW(L"Static", L"Font family:", WS_CHILD, 10, 47, 85, 20, settingsWindow, NULL, NULL, NULL);
    ShowWindow(fontSiText, SW_SHOW), ShowWindow(fontFamText, SW_SHOW), UpdateWindow(fontSiText), UpdateWindow(fontFamText);
    settingsFontSizeEdit = CreateWindowW(L"Edit", L"", WS_CHILD | WS_BORDER | WS_CLIPSIBLINGS | ES_NUMBER, 135, 10, 198, 20, settingsWindow, NULL, NULL, NULL);
    settingsFontFamEdit = CreateWindowW(L"ComboBox", L"", WS_CHILD | WS_VSCROLL | CBS_DROPDOWN | CBS_HASSTRINGS, 135, 45, 200, 160, settingsWindow, NULL, NULL, NULL);
    SetWindowTextW(settingsFontSizeEdit, std::to_wstring((int)userFontSize).c_str()), SetWindowTextW(settingsFontFamEdit, userFontFamily.c_str());
    for (int i = 0; i < fontFamilies.size(); i++) SendMessageW(settingsFontFamEdit, CB_ADDSTRING, 0, (LPARAM)fontFamilies[i].c_str());
    lenGroup = CreateWindowW(L"Button", L"Preferred length", WS_CHILD | BS_GROUPBOX, 40, 90, 130, 100, settingsWindow, NULL, NULL, NULL);
    allRadio = CreateWindowW(L"Button", L"All texts", WS_CHILD | BS_RADIOBUTTON, 50, 110, 110, 15, settingsWindow, (HMENU)106, NULL, NULL);
    shortRadio = CreateWindowW(L"Button", L"Short texts", WS_CHILD | BS_RADIOBUTTON, 50, 130, 110, 15, settingsWindow, (HMENU)107, NULL, NULL);
    medRadio = CreateWindowW(L"Button", L"Medium texts", WS_CHILD | BS_RADIOBUTTON, 50, 150, 110, 15, settingsWindow, (HMENU)108, NULL, NULL);
    longRadio = CreateWindowW(L"Button", L"Long texts", WS_CHILD | BS_RADIOBUTTON, 50,  170, 110, 15, settingsWindow, (HMENU)109, NULL, NULL);
    themeGroup = CreateWindowW(L"Button", L"Theme", WS_CHILD | BS_GROUPBOX, 190, 90, 130, 100, settingsWindow, NULL, NULL, NULL);
    lightRadio = CreateWindowW(L"Button", L"Light", WS_CHILD | BS_RADIOBUTTON, 200, 115, 110, 15, settingsWindow, (HMENU)110, NULL, NULL);
    darkRadio = CreateWindowW(L"Button", L"Dark", WS_CHILD | BS_RADIOBUTTON, 200, 140, 110, 15, settingsWindow, (HMENU)111, NULL, NULL);
    retroRadio = CreateWindowW(L"Button", L"Retro blue", WS_CHILD | BS_RADIOBUTTON, 200, 165, 110, 15, settingsWindow, (HMENU)112, NULL, NULL);    
    ShowWindow(settingsFontSizeEdit, SW_SHOW), ShowWindow(settingsFontFamEdit, SW_SHOW);
    HWND settingsSaveButton = CreateWindowW(L"Button", L"Save", WS_CHILD | WS_CLIPSIBLINGS | WS_BORDER, 290, 420, 60, 25, settingsWindow, (HMENU)101, NULL, NULL);
    HWND settingsCancelButton = CreateWindowW(L"Button", L"Cancel", WS_CHILD | WS_CLIPSIBLINGS | WS_BORDER, 220, 420, 60, 25, settingsWindow, (HMENU)102, NULL, NULL);
    ShowWindow(settingsSaveButton, SW_SHOW), ShowWindow(settingsCancelButton, SW_SHOW), ShowWindow(lenGroup, SW_SHOW), ShowWindow(allRadio, SW_SHOW), ShowWindow(shortRadio, SW_SHOW), ShowWindow(medRadio, SW_SHOW), ShowWindow(longRadio, SW_SHOW);
    ShowWindow(themeGroup, SW_SHOW), ShowWindow(lightRadio, SW_SHOW), ShowWindow(darkRadio, SW_SHOW), ShowWindow(retroRadio, SW_SHOW);
    if (userSML == 0) SendMessageW(allRadio, 241, 1, 0);
    else if (userSML == 1) SendMessageW(shortRadio, 241, 1, 0);
    else if (userSML == 2) SendMessageW(medRadio, 241, 1, 0);
    else SendMessageW(longRadio, 241, 1, 0);
    if (userTheme == 0) SendMessageW(lightRadio, 241, 1, 0);
    else if (userTheme == 1) SendMessageW(darkRadio, 241, 1, 0);
    else SendMessageW(retroRadio, 241, 1, 0);
}
void saveSettings() {
    int fontSiLen = GetWindowTextLengthW(settingsFontSizeEdit) + 1, fontFamLen = GetWindowTextLengthW(settingsFontFamEdit) + 1, left = 0, right = fontFamilies.size() - 1;
    bool validSi = true, found = false;
    wchar_t *siBuffer = new wchar_t[fontSiLen], *famBuffer = new wchar_t[fontFamLen];
    GetWindowTextW(settingsFontSizeEdit, siBuffer, fontSiLen), GetWindowTextW(settingsFontFamEdit, famBuffer, fontFamLen);
    std::wstring fontSi = siBuffer, fontFam = famBuffer;
    while (left <= right) {
        int mid = ((left + right) >> 1); short sh = fontFamilies[mid].compare(fontFam) + 1;
        if (!sh) left = mid + 1;
        else if (sh == 1) { found = true; break; }
        else right = mid - 1;
    }
    if (found && fontFam.length()) userFontFamily = fontFam;
    for (int i = 0; i < fontSi.size(); i++) {
        if ((int)(fontSi[i] - '0') < 0 || (int)(fontSi[i] - '0') > 9) { validSi = false; break; }
    }
    if (!fontSi.size()) validSi = false;
    if (validSi && (fontSi.size() > 3 || std::stoi(fontSi) > 90)) fontSi = L"90";
    else if (validSi && std::stoi(fontSi) < 10) fontSi = L"10";
    if (validSi) userFontSize = std::stoi(fontSi);
    delete[] siBuffer, delete[] famBuffer;
    if (SendMessageW(allRadio, BM_GETCHECK, 0, 0)) userSML = 0; // G1
    else if (SendMessageW(shortRadio, BM_GETCHECK, 0, 0)) userSML = 1;
    else if (SendMessageW(medRadio, BM_GETCHECK, 0, 0)) userSML = 2;
    else userSML = 3;
    if (SendMessageW(lightRadio, BM_GETCHECK, 0, 0)) userTheme = 0; // G2
    else if (SendMessageW(darkRadio, BM_GETCHECK, 0, 0)) userTheme = 1;
    else userTheme = 2;
    wchar_t *adBuffer = new wchar_t[MAX_PATH]; SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, adBuffer);
    std::wstring appDataPath = adBuffer, settingsPath = appDataPath + L"\\BrightType\\settings.txt";
    DeleteFile(settingsPath.c_str()); 
    std::wofstream settingsOutput(settingsPath);
    if (settingsOutput.is_open()) settingsOutput << userFontSize << '\n' << userFontFamily << '\n' << userSML << '\n' << userTheme, settingsOutput.close(); // ***** Add here *****
    changeTextFormat(userFontFamily.c_str(), DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, userFontSize), GetClientRect(appWindow, &app);
    if (currTextLayout) currTextLayout->Release(), currTextLayout = NULL;
    writeFactory->CreateTextLayout(quotes[currQuoteInd].c_str(), quotes[currQuoteInd].length(), currTextFormat, app.right - 40, app.bottom - 40, &currTextLayout), writeText();
    if (!doneScreen && typeHere) DestroyWindow(typeHere), typeHere = CreateWindow(L"Edit", L"", WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | WS_CLIPSIBLINGS, 20, (app.bottom - (userFontSize * 1.3)) - 20, app.right - app.left - 260, userFontSize * 1.3, appWindow, nullptr, hInst, nullptr), ShowWindow(typeHere, SW_SHOW);
    if (doneScreen && resStat) DestroyWindow(resStat), resStat = CreateWindowW(L"static", resstr.c_str(), WS_CHILD, app.left + 20, app.bottom - 100, 220, 200, appWindow, NULL, hInst, NULL), ShowWindow(resStat, SW_SHOW), UpdateWindow(resStat);
    typeHereFont(), SendMessageW(typeHere, WM_SETFONT, (WPARAM)editBoxFont, 0), SendMessageW(appWindow, WM_SIZE, 0, 0), delete[] adBuffer, newText(true, false);
}
LRESULT CALLBACK settingsWindowHandler(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_CLOSE) EnableWindow(appWindow, TRUE), saveSettings(), DestroyWindow(settingsWindow), SetFocus(typeHere);
    else if (message == WM_COMMAND) {
        int id = LOWORD(wParam);
        if (id == 101) EnableWindow(appWindow, TRUE), saveSettings(), DestroyWindow(settingsWindow), SetFocus(typeHere);
        else if (id == 102) EnableWindow(appWindow, TRUE), DestroyWindow(settingsWindow), SetFocus(typeHere);
        else if (id == 106) SendMessageW(allRadio, 241, 1, 0), SendMessageW(shortRadio, 241, 0, 0), SendMessageW(medRadio, 241, 0, 0), SendMessageW(longRadio, 241, 0, 0);
        else if (id == 107) SendMessageW(allRadio, 241, 0, 0), SendMessageW(shortRadio, 241, 1, 0), SendMessageW(medRadio, 241, 0, 0), SendMessageW(longRadio, 241, 0, 0);
        else if (id == 108) SendMessageW(allRadio, 241, 0, 0), SendMessageW(shortRadio, 241, 0, 0), SendMessageW(medRadio, 241, 1, 0), SendMessageW(longRadio, 241, 0, 0);
        else if (id == 109) SendMessageW(allRadio, 241, 0, 0), SendMessageW(shortRadio, 241, 0, 0), SendMessageW(medRadio, 241, 0, 0), SendMessageW(longRadio, 241, 1, 0);
        else if (id == 110) SendMessageW(lightRadio, 241, 1, 0), SendMessageW(darkRadio, 241, 0, 0), SendMessageW(retroRadio, 241, 0, 0);
        else if (id == 111) SendMessageW(lightRadio, 241, 0, 0), SendMessageW(darkRadio, 241, 1, 0), SendMessageW(retroRadio, 241, 0, 0);
        else if (id == 112) SendMessageW(lightRadio, 241, 0, 0), SendMessageW(darkRadio, 241, 0, 0), SendMessageW(retroRadio, 241, 1, 0);
        else if (HIWORD(wParam) == CBN_EDITCHANGE && !GetWindowTextLengthW(settingsFontFamEdit)) {
            for (int i = SendMessage(settingsFontFamEdit, CB_GETCOUNT, 0, 0) - 1; i >= 0; i--) SendMessageW(settingsFontFamEdit, CB_DELETESTRING, i, 0);
            for (int i = 0; i < fontFamilies.size(); i++) SendMessageW(settingsFontFamEdit, CB_ADDSTRING, 0, (LPARAM)fontFamilies[i].c_str());
            SendMessageW(settingsFontFamEdit, CB_SHOWDROPDOWN, TRUE, 0);
        }
        else if (HIWORD(wParam) == CBN_EDITCHANGE && GetWindowTextLengthW(settingsFontFamEdit)) {
            int len = GetWindowTextLengthW(settingsFontFamEdit) + 1; wchar_t* temp = new wchar_t[len];
            GetWindowTextW(settingsFontFamEdit, temp, len); std::wstring ff = temp; 
            for (int i = SendMessageW(settingsFontFamEdit, CB_GETCOUNT, 0, 0) - 1; i >= 0; i--) SendMessageW(settingsFontFamEdit, CB_DELETESTRING, i, 0);
            int start = fontTrie.search(ff);
            if (start != -1) fontTrie.dfs(start);
            SendMessageW(settingsFontFamEdit, CB_SHOWDROPDOWN, TRUE, 0), delete[] temp;
        }
        else return DefWindowProc(hwnd, message, wParam, lParam);
    }
    else if (message == WM_CTLCOLORSTATIC) {
        SetBkColor((HDC)wParam, RGB(255, 255, 255));
        return (INT_PTR)whiteHBRUSH;
    }
    else return DefWindowProc(hwnd, message, wParam, lParam);
}
LRESULT CALLBACK editSubclass(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (wParam == VK_ESCAPE && message == WM_KEYDOWN) newText(false, false);
    else if (message == WM_KEYDOWN && wParam == VK_BACK && !GetWindowTextLengthW(typeHere) && wordPtr) prevWord();
    else if (message == WM_KEYDOWN && wParam == VK_F1) prevQuote();
    else return editproc(hwnd, message, wParam, lParam);
}
void initSettingsClass() {
    wc.cbSize = sizeof(WNDCLASSEXW), wc.style = CS_VREDRAW | CS_HREDRAW;
    wc.lpfnWndProc = settingsWindowHandler, wc.cbClsExtra = 0, wc.cbWndExtra = 0;
    wc.hInstance = hInst, wc.hIcon = 0, wc.hCursor = 0, wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = 0, wc.lpszClassName = L"sName", wc.hIconSm = 0, settingsATOM = RegisterClassExW(&wc);
}
void initRNG() {
    std::random_device seed; rng.seed(seed());
}
void resizeWindow() {
    GetClientRect(appWindow, &app); D2D1_SIZE_U si = D2D1::SizeU(app.right - app.left, app.bottom - app.top);
    if (currTextLayout) currTextLayout->Release(), currTextLayout = NULL;
    rendTarget->Resize(si), writeFactory->CreateTextLayout(quotes[currQuoteInd].c_str(), quotes[currQuoteInd].length(), currTextFormat, app.right - 40, app.bottom - 40, &currTextLayout);
    if (resStat) MoveWindow(resStat, app.top + 20, app.bottom - 100, 220, 200, 1);
    if (anotherOneButton) MoveWindow(anotherOneButton, app.right - 185, app.bottom - 55, 165, 35, 1);
    if (retryButton) MoveWindow(retryButton, app.right - 185, app.bottom - 105, 165, 35, 1);
}
void initQuotes() {
    std::wstring temp; std::wifstream fileInput("text/quotes.txt");
    while (std::getline(fileInput, temp)) quotes.push_back(temp);
    std::sort(quotes.begin(), quotes.end(), lenComparator), third = quotes.size() / 3;
}
void initd2dFactory() {
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2dFactory);
}
void initRenderTarget() {
    GetClientRect(appWindow, &app);
    D2D1_SIZE_U rSi = D2D1::SizeU(app.right - app.left, app.bottom - app.top);
    d2dFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(appWindow, rSi), &rendTarget);
    rendTarget->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.0f, 0.0f), &blackBrush), rendTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 0.0f, 0.0f), &redBrush);
    rendTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f), &whiteBrush), rendTarget->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.0f, 0.67f), &blueBrush);
}
void initWriteFactory() {
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&writeFactory));
}
void initTextLayout() {
    GetClientRect(appWindow, &app);
    if (currTextLayout) currTextLayout->Release(), currTextLayout = NULL;
    writeFactory->CreateTextLayout(quotes[currQuoteInd].c_str(), quotes[currQuoteInd].length(), currTextFormat, app.right - 40, app.bottom - 40, &currTextLayout);
}
void initTextFormat() {
    if (currTextFormat) currTextFormat->Release(), currTextFormat = NULL;
    writeFactory->CreateTextFormat(userFontFamily.c_str(), NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, userFontSize, L"en-us", &currTextFormat);
}
void initFontFamilies() {
    std::wstring temp; std::wifstream fontInput("text/fonts.txt"); int lenSum = 0;
    if (fontInput.good()) {
        while (std::getline(fontInput, temp)) fontFamilies.push_back(temp);
    }
    else fontFamilies.push_back(L"arial");
    if (!std::is_sorted(fontFamilies.begin(), fontFamilies.end())) std::sort(fontFamilies.begin(), fontFamilies.end());
    for (int i = 0; i < fontFamilies.size(); i++) {
        lenSum += fontFamilies[i].length();
        for (int j = 0; j < fontFamilies[i].length(); j++) fontFamilies[i][j] = std::tolower(fontFamilies[i][j]);
    }
    fontTrie.init(lenSum);
    for (int i = 0; i < fontFamilies.size(); i++) fontTrie.insert(fontFamilies[i]);
}
void writeText() {
    GetClientRect(appWindow, &app);
    D2D1_POINT_2F topleft; topleft.x = 20, topleft.y = 20;
    rendTarget->BeginDraw(), currTextLayout->SetUnderline(TRUE, currRange);
    if (!userTheme) rendTarget->Clear(D2D1::ColorF(D2D1::ColorF::White)), rendTarget->DrawTextLayout(topleft, currTextLayout, blackBrush);
    else if (userTheme == 1) rendTarget->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f)), rendTarget->DrawTextLayout(topleft, currTextLayout, whiteBrush);
    else rendTarget->Clear(D2D1::ColorF(0.0f, 0.0f, 0.67f)), rendTarget->DrawTextLayout(topleft, currTextLayout, whiteBrush);
    rendTarget->EndDraw();
}
void newText(bool retry, bool goback) {
    if (started) KillTimer(appWindow, 100);
    wpm = 0.0, cpm = 0.0, started = false, typingErr = false, doneScreen = false, words.clear();
    if (!retry && !goback) newQuoteInd();
    else if (retry) {}
    else if (goback) currQuoteInd = stk.back(), stk.pop_back();
    else newQuoteInd();
    initTextLayout(), split();
    wordPtr = 0, currRange.startPosition = 0, currRange.length = words[0].length() - 1;
    writeText(), ShowWindow(appWindow, SW_SHOW), UpdateWindow(appWindow);
    GetClientRect(appWindow, &app), GetClientRect(typeHere, &eR);
    DestroyWindow(typeHere), DestroyWindow(newTextButton), DestroyWindow(resStat), DestroyWindow(retryButton), DestroyWindow(anotherOneButton), DestroyWindow(prevButton);
    typeHere = CreateWindow(L"Edit", L"", WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | WS_CLIPSIBLINGS, 20, (app.bottom - (userFontSize * 1.3)) - 20, app.right - app.left - 260, userFontSize * 1.3, appWindow, nullptr, hInst, nullptr);
    newTextButton = CreateWindowW(L"Button", L"New text (Esc)", WS_CHILD | BS_CENTER | WS_CLIPSIBLINGS | WS_BORDER, app.right - 110, app.bottom - (eR.bottom - eR.top) - 20, 90, eR.bottom - eR.top, appWindow, (HMENU)100, hInst, nullptr);
    prevButton = CreateWindowW(L"Button", L"Prev (F1)", WS_CHILD | BS_CENTER | WS_CLIPSIBLINGS | WS_BORDER, app.right - 220, app.bottom - (eR.bottom - eR.top) - 20, 90, eR.bottom - eR.top, appWindow, (HMENU)113, hInst, nullptr);

    SendMessage(newTextButton, WM_SETFONT, (WPARAM)newTextFont, 0), SendMessageW(prevButton, WM_SETFONT, (WPARAM)newTextFont, 0);
    ShowWindow(typeHere, SW_SHOW), ShowWindow(newTextButton, SW_SHOW), UpdateWindow(typeHere), UpdateWindow(newTextButton), ShowWindow(prevButton, SW_SHOW), UpdateWindow(prevButton);
    SendMessage(typeHere, WM_SETFONT, (WPARAM)editBoxFont, TRUE), SetFocus(typeHere), SetWindowTextW(appWindow, L"   BrightType");
    editproc = reinterpret_cast<WNDPROC>(GetWindowLongPtrW(typeHere, GWLP_WNDPROC)), SetWindowLongPtrW(typeHere, GWLP_WNDPROC, (LONG_PTR)editSubclass);
    SendMessage(appWindow, WM_SIZE, 0, 0);
}
ATOM MyRegisterClass(HINSTANCE hInstance); BOOL InitInstance(HINSTANCE, int); LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM); // forward decl

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    CoInitialize(NULL);
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_BRIGHTTYPE11, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);
    if (!InitInstance(hInstance, nCmdShow)) return FALSE;
    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_BRIGHTTYPE11));
    MSG msg;
    SendMessage(typeHere, WM_SETFONT, (WPARAM)editBoxFont, TRUE), SetFocus(typeHere);

    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) TranslateMessage(&msg), DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
ATOM MyRegisterClass(HINSTANCE hInstance) {
    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(WNDCLASSEX), wcex.style = CS_HREDRAW | CS_VREDRAW, wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0, wcex.cbWndExtra = 0, wcex.hInstance = hInstance, wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SMALL));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW), wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1), wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_BRIGHTTYPE11);
    wcex.lpszClassName = szWindowClass, wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
    hInst = hInstance;
    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);
    GetClientRect(hWnd, &app);
    appWindow = hWnd, SetWindowTextW(appWindow, L"BrightType");
    initUserSettings(), typeHere = CreateWindow(L"Edit", L"", WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | WS_CLIPSIBLINGS, 20, (app.bottom - (userFontSize * 1.3)) - 20, app.right - app.left - 260, userFontSize * 1.3, appWindow, nullptr, hInst, nullptr);
    GetClientRect(typeHere, &eR), newTextButton = CreateWindowW(L"Button", L"New text (Esc)", WS_CHILD | BS_CENTER | WS_CLIPSIBLINGS | WS_BORDER, app.right - 110, app.bottom - (eR.bottom - eR.top) - 20, 90, eR.bottom - eR.top, appWindow, (HMENU)100, hInst, nullptr);
    prevButton = CreateWindowW(L"Button", L"Prev (F1)", WS_CHILD | BS_CENTER | WS_CLIPSIBLINGS | WS_BORDER, app.right - 220, app.bottom - (eR.bottom - eR.top) - 20, 90, eR.bottom - eR.top, appWindow, (HMENU)113, hInst, nullptr);
    initQuotes(), initRNG(), typeHereFont(), initd2dFactory(), initRenderTarget(), initWriteFactory(), initTextFormat(), initTextLayout();
    initSettingsClass(), screen = GetDesktopWindow(), initNewTextFont(), initFontFamilies(), SendMessage(newTextButton, WM_SETFONT, (WPARAM)newTextFont, 0), SendMessageW(prevButton, WM_SETFONT, (WPARAM)newTextFont, 0);
    newQuoteInd(), split(), wordPtr = 0;
    currRange.startPosition = 0, currRange.length = words[0].length() - 1;

    ShowWindow(appWindow, nCmdShow), UpdateWindow(appWindow), ShowWindow(typeHere, nCmdShow), UpdateWindow(typeHere);
    ShowWindow(newTextButton, nCmdShow), UpdateWindow(newTextButton), ShowWindow(prevButton, SW_SHOW), UpdateWindow(prevButton);
    editproc = reinterpret_cast<WNDPROC>(GetWindowLongPtrW(typeHere, GWLP_WNDPROC)), SetWindowLongPtrW(typeHere, GWLP_WNDPROC, (LONG_PTR)editSubclass);
    
    d2dFactory->Release(), d2dFactory = NULL; 
    return TRUE;
}
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_COMMAND) {
        int wmId = LOWORD(wParam);
        if (wmId == IDM_ABOUT) MessageBox(appWindow, L"BrightType Version 1.1\n\nMade by Bright Gao, w/ C/C++ and Win32\n\nContact me: brightgao1234@gmail.com", L"About BrightType", MB_OK);
        else if (wmId == settingsID) initSettingsWindow();
        else if (HIWORD(wParam) == EN_CHANGE && lParam == (LPARAM)typeHere) {
            if (!started) started = true, start();
            handleKeypress();
        }
        else if (LOWORD(wParam) == 100 || LOWORD(wParam) == 103) newText(false, false); // clicked newtext or anotherOne w/ mouse
        else if (LOWORD(wParam) == 105) newText(true, false);
        else if (LOWORD(wParam) == 113) prevQuote(), SetFocus(typeHere);
        else return DefWindowProc(hWnd, message, wParam, lParam);
    }
    else if (message == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        writeText(), EndPaint(hWnd, &ps);
    }
    else if (message == WM_SIZE) {
        resizeWindow();
        GetClientRect(typeHere, &eR), GetClientRect(appWindow, &app);
        if (typeHere) MoveWindow(typeHere, 20, (app.bottom - (userFontSize * 1.3)) - 20, app.right - app.left - 260, userFontSize * 1.3, 1);
        if (newTextButton) MoveWindow(newTextButton, app.right - 110, app.bottom - (eR.bottom - eR.top) - 20, 90, eR.bottom - eR.top, 1);
        if (prevButton) MoveWindow(prevButton, app.right - 220, app.bottom - (eR.bottom - eR.top) - 20, 90, eR.bottom - eR.top, 1);
    }
    else if (message == WM_GETMINMAXINFO) {
        LPMINMAXINFO mmi = (LPMINMAXINFO)lParam;
        mmi->ptMinTrackSize.x = 300, mmi->ptMinTrackSize.y = 200;
    }
    else if (message == WM_CTLCOLORSTATIC) {
        if (!userTheme) SetBkColor((HDC)wParam, RGB(255, 255, 255)), SetTextColor((HDC)wParam, RGB(0, 0, 0));
        else if (userTheme == 1) SetBkColor((HDC)wParam, RGB(0, 0, 0)), SetTextColor((HDC)wParam, RGB(255, 255, 255));
        else SetBkColor((HDC)wParam, RGB(0, 0, 171)), SetTextColor((HDC)wParam, RGB(255, 255, 255));
        if (!userTheme) return (INT_PTR)whiteHBRUSH;
        else if (userTheme == 1) return (INT_PTR)blackHBRUSH;
        else return (INT_PTR)blueHBRUSH;
    }
    else if (message == WM_CTLCOLOREDIT) {
        HDC dc = (HDC)wParam;
        if (!userTheme) {
            SetBkColor(dc, RGB(255, 255, 255)); return (INT_PTR)whiteHBRUSH;
        }
        else if (userTheme == 1) {
            SetBkColor(dc, RGB(0, 0, 0)), SetTextColor(dc, RGB(255, 255, 255)); return (INT_PTR)blackHBRUSH;
        }
        else {
            SetBkColor(dc, RGB(0, 0, 171)), SetTextColor(dc, RGB(255, 255, 255)); return (INT_PTR)blueHBRUSH;
        }
    }
    else if (message == WM_TIMER && wParam == 100) {
        wchar_t* buf = new wchar_t[30]; GetWindowTextW(appWindow, buf, 30); std::wstring newTime = buf; delete[] buf;
        newTime = std::to_wstring(std::stoi(newTime) + 1), SetWindowTextW(appWindow, (L"   " + newTime).c_str());
    }
    else if (message == WM_KEYDOWN && wParam == VK_ESCAPE) newText(false, false);
    else if (doneScreen && message == WM_KEYDOWN && (wParam == VK_RETURN || wParam == VK_ESCAPE)) newText(false, false);
    else if (doneScreen && message == WM_KEYDOWN && (wParam == 0x52)) newText(true, false);
    else if (message == WM_DESTROY) PostQuitMessage(0);
    else return DefWindowProc(hWnd, message, wParam, lParam);

    return 0;
}