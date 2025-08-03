#include "file_dialog.h"

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#include <string>
#include <vector>
#include <algorithm>

namespace NativeFileDialog {
    namespace {
        std::wstring utf8_to_wide(const std::string& utf8_string) {
            if (utf8_string.empty()) return std::wstring();
            int size_needed = MultiByteToWideChar(CP_UTF8, 0, &utf8_string[0], (int)utf8_string.size(), NULL, 0);
            std::wstring wstrTo(size_needed, 0);
            MultiByteToWideChar(CP_UTF8, 0, &utf8_string[0], (int)utf8_string.size(), &wstrTo[0], size_needed);
            return wstrTo;
        }

        std::string wide_to_utf8(const std::wstring& wide_string) {
            if (wide_string.empty()) return std::string();
            int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wide_string[0], (int)wide_string.size(), NULL, 0, NULL, NULL);
            std::string strTo(size_needed, 0);
            WideCharToMultiByte(CP_UTF8, 0, &wide_string[0], (int)wide_string.size(), &strTo[0], size_needed, NULL, NULL);
            return strTo;
        }
    }

    std::string OpenFile(const char* filter) {
        wchar_t filename[MAX_PATH] = { 0 };
        OPENFILENAMEW ofn = { 0 };
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = NULL;
        ofn.lpstrFile = filename;
        ofn.nMaxFile = MAX_PATH;

        std::wstring wide_filter = utf8_to_wide(filter);
        std::vector<wchar_t> filter_buffer(wide_filter.begin(), wide_filter.end());
        for (auto& c : filter_buffer) if (c == L'\0') c = L'|';
        std::replace(filter_buffer.begin(), filter_buffer.end(), L'|', L'\0');
        ofn.lpstrFilter = filter_buffer.data();

        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = NULL;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

        if (GetOpenFileNameW(&ofn) == TRUE) {
            return wide_to_utf8(ofn.lpstrFile);
        }
        return std::string();
    }

    std::string SaveFile(const char* filter) {
        wchar_t filename[MAX_PATH] = { 0 };
        OPENFILENAMEW ofn = { 0 };
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = NULL;
        ofn.lpstrFile = filename;
        ofn.nMaxFile = MAX_PATH;

        std::wstring wide_filter = utf8_to_wide(filter);
        std::vector<wchar_t> filter_buffer(wide_filter.begin(), wide_filter.end());
        for (auto& c : filter_buffer) if (c == L'\0') c = L'|';
        std::replace(filter_buffer.begin(), filter_buffer.end(), L'|', L'\0');
        ofn.lpstrFilter = filter_buffer.data();

        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = NULL;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
        ofn.lpstrDefExt = L"lua";

        if (GetSaveFileNameW(&ofn) == TRUE) {
            return wide_to_utf8(ofn.lpstrFile);
        }
        return std::string();
    }
}
#endif