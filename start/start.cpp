// start.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <xamlOM.h>
#include <cstdio>
#include <windows.h>
#include <tlhelp32.h>
#include <shlwapi.h>
#include <sddl.h>
#include <aclapi.h>
#include <direct.h>
#include <Shlobj.h>
#include <string_view>
#include <filesystem>
#include "resource.h"

EXTERN_C IMAGE_DOS_HEADER __ImageBase;
PSECURITY_DESCRIPTOR sd = nullptr, sdFile = nullptr;
HINSTANCE get_instance() { return (HINSTANCE)&__ImageBase; }

bool RegistryGrantAll(HKEY hKey)
{
	bool bResult = false;

	DWORD ok = 0;
	const TCHAR* szSD =
		TEXT("D:")                          // Discretionary ACL
		TEXT("(A;OICI;KR;;;S-1-5-12)")      // NT AUTHORITY\RESTRICTED
		TEXT("(A;OICI;KA;;;S-1-5-18)")      // NT AUTHORITY\SYSTEM
		TEXT("(A;OICI;KA;;;S-1-5-32-545)")  // BUILTIN\Users
		TEXT("(A;OICI;KA;;;S-1-5-32-544)")  // BUILTIN\Administrators
		TEXT("(A;OICI;KA;;;S-1-15-2-1)");   // APPLICATION PACKAGE AUTHORITY\ALL APPLICATION PACKAGES

	if (ConvertStringSecurityDescriptorToSecurityDescriptor((LPCTSTR)szSD, SDDL_REVISION_1, &sd, 0))
	{
		auto result = RegSetKeySecurity(hKey, DACL_SECURITY_INFORMATION, sd);
		if (ERROR_SUCCESS == result)
			bResult = true;
		else
			SetLastError(result);
		// Free the memory allocated for the SECURITY_DESCRIPTOR.
		LocalFree(sd);
	}
	return bResult;
}

int FileGrantAll(LPCWSTR file)
{
	const TCHAR* szSD =
		TEXT("D:AI")                          // Discretionary ACL
		TEXT("(A;OICI;0x1200a9;;;AC)")
		TEXT("(A;OICIID;FA;;;BA)")
		TEXT("(A;OICIID;FA;;;SY)")
		TEXT("(A;OICIID;0x1200a9;;;BU)")
		TEXT("(A;ID;0x1301bf;;;AU)")
		TEXT("(A;OICIIOID;SDGXGWGR;;;AU)");
	if (ConvertStringSecurityDescriptorToSecurityDescriptor((LPCTSTR)szSD, SDDL_REVISION_1, &sdFile, 0))
	{
		SetFileSecurity(file, DACL_SECURITY_INFORMATION, sdFile);
	}
	return 1;
}

std::pair<const void*, size_t> get_resource(WORD type, WORD id)
{
	const auto rc = FindResource(
		get_instance(),
		MAKEINTRESOURCE(id),
		MAKEINTRESOURCE(type)
	);
	if (!rc)
		return { nullptr, 0 };
	const auto rc_data = LoadResource(get_instance(), rc);
	const auto size = SizeofResource(get_instance(), rc);
	if (!rc_data)
		return { nullptr, 0 };
	const auto data = static_cast<const void*>(LockResource(rc_data));
	return { data, size };
}

static USHORT get_native_architecture()
{
	// This is insanity

	static const auto architecture = []
	{
		typedef BOOL(WINAPI* LPFN_ISWOW64PROCESS2) (HANDLE, PUSHORT, PUSHORT);

		const auto kernel32 = GetModuleHandleW(L"kernel32");
		const auto pIsWow64Process2 = kernel32 ? (LPFN_ISWOW64PROCESS2)GetProcAddress(kernel32, "IsWow64Process2") : nullptr;
		USHORT ProcessMachine = 0;
		USHORT NativeMachine = 0;

		// Apparently IsWow64Process2 can fail somehow
		if (pIsWow64Process2 && pIsWow64Process2(GetCurrentProcess(), &ProcessMachine, &NativeMachine))
			return NativeMachine;

		SYSTEM_INFO si;
		// On 64 bit processors that aren't x64 or IA64, GetNativeSystemInfo behaves as GetSystemInfo
		GetNativeSystemInfo(&si);
		switch (si.wProcessorArchitecture)
		{
		case PROCESSOR_ARCHITECTURE_AMD64:
			return (USHORT)IMAGE_FILE_MACHINE_AMD64;
		case PROCESSOR_ARCHITECTURE_ARM:
			return (USHORT)IMAGE_FILE_MACHINE_ARM;
		case PROCESSOR_ARCHITECTURE_ARM64: // according to docs this could never happen
			return (USHORT)IMAGE_FILE_MACHINE_ARM64;
		case PROCESSOR_ARCHITECTURE_IA64:
			return (USHORT)IMAGE_FILE_MACHINE_IA64;
		case PROCESSOR_ARCHITECTURE_INTEL:
			return (USHORT)IMAGE_FILE_MACHINE_I386;
		default:
			break;
		}

		// I wonder why does IsWow64Process exist when GetNativeSystemInfo can provide same and more, plus it cannot fail
		// either unlike IsWow64Process which apparently can do so.

		return (USHORT)IMAGE_FILE_MACHINE_UNKNOWN;
	}();
	return architecture;
}

static int get_needed_dll_resource_id()
{
	switch (get_native_architecture())
	{
	case IMAGE_FILE_MACHINE_AMD64:
		return TAPx64;
	default:
		break;
	}
	return 0;
}

std::pair<const void*, size_t> get_dll_blob()
{
	const auto id = get_needed_dll_resource_id();
	return id ? get_resource(256, id) : std::pair<const void*, size_t>{ nullptr, 0 };
}

DWORD write_file(std::wstring path, const void* data, size_t size)
{
	DWORD error = NO_ERROR;
	const auto file = CreateFileW(
		path.data(),
		FILE_WRITE_DATA,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		nullptr,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		nullptr
	);
	if (file != INVALID_HANDLE_VALUE)
	{
		DWORD written = 0;
		const auto succeeded = WriteFile(
			file,
			data,
			size,
			&written,
			nullptr
		);
		if (!succeeded || written != size)
			error = GetLastError();

		CloseHandle(file);
	}
	else
		error = GetLastError();

	return error;
}

int main()
{
	std::cout << "Initializing...\nอออออออออออออออ\n";
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);
	std::wstring ok = L"StartMenuExperienceHost.exe";
	DWORD pid = 0;
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    if (Process32First(snapshot, &entry) == TRUE)
    {
        while (Process32Next(snapshot, &entry) == TRUE)
        {
            if (wcscmp(entry.szExeFile, ok.c_str())== 0)
            {
                pid = entry.th32ProcessID;
            }
        }
    }
    CloseHandle(snapshot);
    std::cout << "\n- StartMenuExperienceHost.exe PID: " << pid;
    static constexpr GUID temp = { 0x36162bd3, 0x3531, 0x4131, { 0x9b, 0x8b, 0x7f, 0xb1, 0xa9, 0x91, 0xef, 0x51 } };
    typedef HRESULT(*InitializeXamlDiagnosticsExProto)(_In_ LPCWSTR endPointName, _In_ DWORD pid, _In_opt_ LPCWSTR wszDllXamlDiagnostics, _In_ LPCWSTR wszTAPDllName, _In_opt_ CLSID tapClsid, _In_ LPCWSTR wszInitializationData);
    InitializeXamlDiagnosticsExProto InitializeXamlDiagnosticsExFn = (InitializeXamlDiagnosticsExProto)GetProcAddress(LoadLibraryW(L"Windows.UI.Xaml.dll"), "InitializeXamlDiagnosticsEx");
    HKEY subKey = nullptr;
    DWORD disposition;
    DWORD dwSize = sizeof(DWORD), dwInstalled = 0;
    RegCreateKeyEx(HKEY_CURRENT_USER, L"SOFTWARE\\TranslucentSM", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &subKey, &disposition);
    if (disposition == REG_CREATED_NEW_KEY)
    {
        std::cout << "\n- Created HKCU\\SOFTWARE\\TranslucentSM registry key.";
        RegistryGrantAll(subKey);
        DWORD opacity = 3;
        RegSetValueEx(subKey, TEXT("TintOpacity"), 0, REG_DWORD, (const BYTE*)&opacity, sizeof(opacity));
    }
    else
    {
        std::cout << "\n- Opened HKCU\\SOFTWARE\\TranslucentSM registry key.";
    }
    RegCloseKey(subKey);
    SECURITY_ATTRIBUTES st;
    st.bInheritHandle = FALSE;
    st.lpSecurityDescriptor = sd;
    st.nLength = sizeof(SECURITY_ATTRIBUTES);
	// Get the path to the current executable
	wchar_t path[MAX_PATH];
	GetModuleFileNameW(NULL, path, MAX_PATH);
	std::wstring pathStr = path;
	// Get the path to the current directory
	std::wstring dir = pathStr.substr(0, pathStr.find_last_of(L"\\"));
	// Get the path to the DLL
	std::wstring dllPath = dir + L"\\TAPdll.dll";
    // Convert dllPath to WCHAR
	const wchar_t* dllPathW = dllPath.c_str();
    auto nResult = FileGrantAll(dllPathW);
    if (nResult != 0)
    {
        std::cout << "\n- Changed TAPdll.dll permissions";
    }
    InitializeXamlDiagnosticsExFn(L"VisualDiagConnection1", pid, NULL, dllPathW, temp, L"");
	std::cout << "\n- Success!"; // ig always succeeds
	std::cout << "\n\n";
    return 0;
}