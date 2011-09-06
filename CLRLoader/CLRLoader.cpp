#include <Windows.h>
#include <metahost.h>
#include <tchar.h>
#include "scopeguard.h"
#include "odprintf.h"
#pragma comment(lib, "mscoree.lib")

HMODULE module = NULL;

BOOL CALLBACK DllMain(HINSTANCE dll, DWORD reason, LPVOID) {
	if (reason == DLL_PROCESS_ATTACH) {
		module = (HMODULE)dll;
	}
	return TRUE;
}

typedef struct {
	LPCWSTR dotnetVersion;
	LPCWSTR assemblyPath;
	LPCWSTR typeName;
	LPCWSTR methodName;
	LPCWSTR argument;
} CLR_LOAD_ARGS;

DWORD CALLBACK StartCLRThreadProc(CLR_LOAD_ARGS *args) {
	ICLRMetaHost *metahost;
	ICLRRuntimeInfo *runtimeInfo;
	ICLRRuntimeHost *runtimeHost;

    HRESULT hr = CLRCreateInstance(CLSID_CLRMetaHost, IID_ICLRMetaHost, (LPVOID*)&metahost);
    if (!SUCCEEDED(hr)) {
		odprintf("CLRCreateInstance returned %d", hr);
        return 1;
    }
	ON_BLOCK_EXIT_OBJ(*metahost, &ICLRMetaHost::Release);

	hr = metahost->GetRuntime(args->dotnetVersion, IID_ICLRRuntimeInfo, (LPVOID*)&runtimeInfo);
    if (!SUCCEEDED(hr)) {
        odprintf("ICLRMetaHost::GetRuntime returned %d", hr);
        return 1;
    }
	ON_BLOCK_EXIT_OBJ(*runtimeInfo, &ICLRRuntimeInfo::Release);

    hr = runtimeInfo->GetInterface(CLSID_CLRRuntimeHost, IID_ICLRRuntimeHost, (LPVOID*)&runtimeHost);
    if (!SUCCEEDED(hr)) {
        odprintf("ICLRRuntimeInfo::GetInterface returned %d", hr);
        return 1;
    }
	ON_BLOCK_EXIT_OBJ(*runtimeHost, &ICLRRuntimeHost::Release);

    hr = runtimeHost->Start();
    if (!SUCCEEDED(hr)) {
		odprintf("ICLRRuntimeHost::Start returned %d", hr);
        return 1;
    }

    WCHAR pathLoader[MAX_PATH];
    DWORD len = GetModuleFileNameW(module, pathLoader, MAX_PATH);
    if (!len) {
		odprintf("GetModuleFileNameW failed. Last Error: %d", GetLastError());
        return 1;
    }

    DWORD ret;
	hr = runtimeHost->ExecuteInDefaultAppDomain(args->assemblyPath, args->typeName, args->methodName, args->argument, &ret);
    if (!SUCCEEDED(hr)) {
		odprintf("ICLRRuntimeHost::ExecuteInDefaultAppDomain returned %d", hr);
        return 1;
    }

	odprintf("CLR method returned %d", ret);

    return 0;
}