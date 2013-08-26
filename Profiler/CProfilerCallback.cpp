/*
 * @ConQAT.Rating YELLOW Hash: 5BC43D3204326C64782428D6C56505E9
 */

#include <windows.h>
#include <stdio.h>
#include "CProfilerCallback.h"
#include <winuser.h>

#pragma intrinsic(strcmp,labs,strcpy,_rotl,memcmp,strlen,_rotr,memcpy,_lrotl,_strset,memset,_lrotr,abs,strcat)

// Constants used for report generation
// TODO [NG]: Is this really needed? If yes, use a normal constant?
#ifdef _WIN64
#define HEADER "Coverage profiler version 0.9.1.2 (x64)"
#endif
#ifndef _WIN64
#define HEADER "Coverage profiler version 0.9.1.2 (x86)"
#endif

// TODO [NG]: Use "normal" constants for the following macros?
#define INFO "Info"
#define ASSEMBLY "Assembly"
#define PROCESS "Process"
#define INLINED "Inlined"
#define JITTED "Jitted"
#define STARTED "Started"
#define STOPPED "Stopped"

/** Constructor. */
CProfilerCallback::CProfilerCallback() : resultFile(INVALID_HANDLE_VALUE) {
		// Make a critical section for synchronization.
		InitializeCriticalSection(&criticalSection);
}

/** Destructor. */
CProfilerCallback::~CProfilerCallback() {
	// Clean up the critical section.
	DeleteCriticalSection(&criticalSection);
}

/** Initializer. Called at profiler startup. */
// TODO [NG]: Chose better name for the parameter?
// -> its the same name as in the function we override here. Thus, I'd rather keep it. 
HRESULT CProfilerCallback::Initialize(IUnknown * pICorProfilerInfoUnk ) {
	CreateOutputFile();

	// Initialize data structures.
	assemblyCounter = 1;
	assemblyMap = new map<int, int>;
	jittedMethods = new vector<FunctionInfo>;
	inlinedMethods = new set<FunctionID>;
	inlinedMethodsList = new vector<FunctionInfo>;

	// Get reference to the ICorProfilerInfo interface 
	HRESULT hr =
		pICorProfilerInfoUnk->QueryInterface( IID_ICorProfilerInfo, (LPVOID *)&pICorProfilerInfo );
	if ( FAILED(hr) ) {
		return E_INVALIDARG;
	}

	hr = pICorProfilerInfoUnk->QueryInterface( IID_ICorProfilerInfo2, (LPVOID *)&pICorProfilerInfo2 );

	if ( FAILED(hr) ) {
		// We still want to work if this call fails, might be an older .NET
		// version than VS2005.
		OutputDebugString("Pre .NET 2 version detected");
		pICorProfilerInfo2.p = NULL;
	}

	// Indicate which events we're interested in.
	DWORD dwEventMask = GetEventMask();

	// Set the event mask for the interfaces of .NET 1 and .NET 2. We currently
	// do not need the features of the profiling interface beyond .NET 2.
	if (pICorProfilerInfo2.p == NULL) {
		// Pre .NET 2.
		pICorProfilerInfo->SetEventMask(dwEventMask);
		// Enable function mapping.
		pICorProfilerInfo->SetFunctionIDMapper(FunctionMapper);
	} else {
		// .NET 2 and beyond.
		pICorProfilerInfo2->SetEventMask(dwEventMask);
		// Enable function mapping.
		pICorProfilerInfo2->SetFunctionIDMapper(FunctionMapper);
	}
	WriteProcessInfoToOutputFile();
	return S_OK;
}

/**
 * Writes information about the profiled process (path to executable) to the
 * output file.
 */
void CProfilerCallback::WriteProcessInfoToOutputFile(){
	// Get the name of the executing process and write to log.
	szAppPath[0] = 0;
	szAppName[0] = 0;
	if (0 == GetModuleFileNameW(NULL, szAppPath, MAX_PATH)) {
		_wsplitpath_s(szAppPath, NULL, 0, NULL, 0, szAppName, _MAX_FNAME, NULL, 0);
	}
	if (szAppPath[0] == 0) {
		wcscpy_s(szAppPath, MAX_PATH, L"No Application Path Found");
		wcscpy_s(szAppName, _MAX_FNAME, L"No Application Name Found");
	}

	// TODO [NG]: What are the following two lines for?
	char process[NAME_BUFFER_SIZE];
	sprintf_s(process, "%S", szAppPath);
	WriteTupleToFile(PROCESS, process);
}

/** Create the output file and add general information. */
void CProfilerCallback::CreateOutputFile() {
	// Read target directory from environment variable.
	char targetDir[1000];
	if (!GetEnvironmentVariable("COR_PROFILER_TARGETDIR", targetDir,
			sizeof(targetDir))) {
		sprintf_s(targetDir, "c:/profiler/");
	}

	SYSTEMTIME time = GetTime();;

	// Create target file.
	char targetFilename[NAME_BUFFER_SIZE];
	sprintf_s(targetFilename, "%s/coverage_%04d%02d%02d_%02d%02d%02d%04d.txt",
			targetDir, time.wYear, time.wMonth, time.wDay, time.wHour,
			time.wMinute, time.wSecond, time.wMilliseconds);
	_tcscpy_s(pszResultFile, targetFilename);

	EnterCriticalSection(&criticalSection);
	resultFile = CreateFile(pszResultFile, GENERIC_WRITE, FILE_SHARE_READ,
			NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	WriteTupleToFile(INFO, HEADER);

	char timeStamp[NAME_BUFFER_SIZE];
	sprintf_s(timeStamp, "%04d%02d%02d_%02d%02d%02d%04d", time.wYear,
			time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond,
			time.wMilliseconds);
	WriteTupleToFile(STARTED, timeStamp);
	LeaveCriticalSection(&criticalSection);
}

/** Return the current time. */ 
SYSTEMTIME CProfilerCallback::GetTime() {
	SYSTEMTIME time;
	GetSystemTime (&time);
	return time;
}

/** Write coverage information to log file at shutdown. */
HRESULT CProfilerCallback::Shutdown() {
	SYSTEMTIME time = GetTime();

	// Write inlined methods.
	WriteToFile("//%i methods inlined\r\n", inlinedMethodsList->size());
	WriteToLog(INLINED, inlinedMethodsList);

	// Write jitted methods.
	WriteToFile("//%i methods jitted\r\n", jittedMethods->size());
	WriteToLog(JITTED, jittedMethods);

	// Write timestamp.
	char timeStamp[NAME_BUFFER_SIZE];
	sprintf_s(timeStamp, "%04d%02d%02d_%02d%02d%02d%04d", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);
	WriteTupleToFile(STOPPED, timeStamp);

	WriteTupleToFile(INFO, "Shutting down coverage profiler" );

	// Cleanup.
	pICorProfilerInfo2->ForceGC();

	// Close the log file.
	EnterCriticalSection(&criticalSection);
	if(resultFile != INVALID_HANDLE_VALUE) {
		CloseHandle(resultFile);
	}
	LeaveCriticalSection(&criticalSection);

	delete assemblyMap;
	delete jittedMethods;
	delete inlinedMethods;
	delete inlinedMethodsList;

	return S_OK;
}

/**
 * The event mask tells the CLR which callbacks the profiler wants to subscribe
 * to. We enable JIT compilation and assembly loads for coverage profiling. In
 * addition, EnterLeave hooks are enabled to force re-jitting of pre-jitted
 * code, in order to make coverage information independent of pre-jitted code.
 */
DWORD CProfilerCallback::GetEventMask() {
	DWORD dwEventMask = 0;
	dwEventMask |= COR_PRF_MONITOR_JIT_COMPILATION;
	dwEventMask |= COR_PRF_MONITOR_ASSEMBLY_LOADS;
	dwEventMask |= COR_PRF_MONITOR_ENTERLEAVE;

	return dwEventMask;
}

/**
 * We do not register a single function callback in order to not affect
 * performance. In effect, we disable this feature here. It has been tested via
 * a performance benchmark, that this implementation does not impact call
 * performance.
 *
 * For coverage profiling, we do not need this callback. However, the event is
 * enabled in the event mask in order to force JIT-events for each first call to
 * a function, independent of whether a pre-jitted version exists.)
 */
UINT_PTR CProfilerCallback::FunctionMapper(FunctionID functionId,
		BOOL *pbHookFunction) {
	// Disable hooking of functions.
	*pbHookFunction = false;

	// Always return original function id.
	return functionId;
}

/** Store information about jitted method. */
HRESULT CProfilerCallback::JITCompilationFinished(FunctionID functionId,
		HRESULT hrStatus, BOOL fIsSafeToBlock) {
	// Notify monitor that method has been jitted.
	FunctionInfo info;
	GetFunctionInfo(functionId, &info);
	jittedMethods->push_back(info);

	// Always return OK
	return S_OK;
}

/** Write loaded assembly to log file. */
HRESULT CProfilerCallback::AssemblyLoadFinished(AssemblyID assemblyId,
		HRESULT hrStatus) {
	// Store assembly counter for id.
	int assemblyNumber = assemblyCounter++;
	(*assemblyMap)[assemblyId] = assemblyNumber;

	// Log assembly load.
	WCHAR assemblyName[NAME_BUFFER_SIZE];
	ULONG assemblyNameSize = 0;
	AppDomainID appDomainId = 0;
	ModuleID moduleId = 0;
	pICorProfilerInfo->GetAssemblyInfo(assemblyId, NAME_BUFFER_SIZE,
			&assemblyNameSize, assemblyName, &appDomainId, &moduleId);

	// Call GetModuleMetaData to get a MetaDataAssemblyImport object.
	IMetaDataAssemblyImport *pMetaDataAssemblyImport = NULL;
	pICorProfilerInfo->GetModuleMetaData(moduleId, ofRead,
			IID_IMetaDataAssemblyImport, (IUnknown**) &pMetaDataAssemblyImport);

	// Get the assembly token.
	mdAssembly ptkAssembly = NULL;
	pMetaDataAssemblyImport->GetAssemblyFromScope(&ptkAssembly);

	// Call GetAssemblyProps:
	// Allocate memory for the pointers, as GetAssemblyProps seems to
	// dereference the pointers (it crashed otherwise). We allocate the minimum
	// amount of memory as we do not know how much is needed to store the array.
	// This information would be available in the fields metadata.cbLocale,
	// metadata.ulProcessor and metadata.ulOS after the call to
	// GetAssemblyProps. However, we do not need the additional data and save
	// the second call to GetAssemblyProps with the correct amount of memory.
	ASSEMBLYMETADATA metadata;
	// TODO [NG]: Why malloc/free instead of new/delete?
	metadata.szLocale = new WCHAR;//(WCHAR*) malloc(sizeof(WCHAR));
	metadata.rProcessor = new DWORD; //(DWORD*) malloc(sizeof(DWORD));
	metadata.rOS = new OSINFO;//(OSINFO*) malloc(sizeof(OSINFO));
	pMetaDataAssemblyImport->GetAssemblyProps(ptkAssembly, NULL, NULL, NULL,
			NULL, 0, NULL, &metadata, NULL);
	delete metadata.szLocale;
	delete metadata.rProcessor;
	delete metadata.rOS;

	char target[NAME_BUFFER_SIZE];
	sprintf_s(target, "%S:%i Version:%i.%i.%i.%i", assemblyName, assemblyNumber,
			metadata.usMajorVersion, metadata.usMinorVersion,
			metadata.usBuildNumber, metadata.usRevisionNumber);
	WriteTupleToFile(ASSEMBLY, target);

	// Always return OK
	return S_OK;
}

/** Record inlining of method, but generally allow it. */
HRESULT CProfilerCallback::JITInlining(FunctionID callerID, FunctionID calleeId,
		BOOL *pfShouldInline) {
	// Notify monitor that method has been inlined.
	if (inlinedMethods->insert(calleeId).second == true) {
		FunctionInfo info;
		GetFunctionInfo(calleeId, &info);
		inlinedMethodsList->push_back(info);
	}

	// Always allow inlining.
	*pfShouldInline = true;

	// Always return OK
	return S_OK;
}

/** Create method info object for a function id. */
HRESULT CProfilerCallback::GetFunctionInfo(FunctionID functionID,
		FunctionInfo* info) {
	HRESULT hr = E_FAIL; // Assume fail.
	mdToken functionToken = mdTypeDefNil;
	IMetaDataImport *pMDImport = NULL;
	WCHAR funName[NAME_BUFFER_SIZE] = L"UNKNOWN";

	// Get the MetadataImport interface and the metadata token.
	hr = pICorProfilerInfo->GetTokenAndMetaDataFromFunction(functionID,
			IID_IMetaDataImport, (IUnknown **) &pMDImport, &functionToken);
	if (SUCCEEDED(hr)) {
		mdTypeDef classToken = mdTypeDefNil;
		DWORD methodAttr = 0;
		PCCOR_SIGNATURE sigBlob = NULL;
		ULONG sigSize = 0;
		ModuleID moduleId = 0;
		hr = pMDImport->GetMethodProps(functionToken, &classToken, funName,
				NAME_BUFFER_SIZE, 0, &methodAttr, &sigBlob, &sigSize, NULL,
				NULL);
		if (SUCCEEDED(hr)) {
			WCHAR className[NAME_BUFFER_SIZE] = L"UNKNOWN";
			ClassID classId = 0;

			if (pICorProfilerInfo2 != NULL) {
				ULONG32 values = 0;
				hr = pICorProfilerInfo2->GetFunctionInfo2(functionID, 0,
						&classId, &moduleId, &functionToken, 0, &values, NULL);
				if (!SUCCEEDED(hr)) {
					classId = 0;
				}
			}

			int assemblyNumber = -1;
			if (SUCCEEDED(hr) && moduleId != 0) {
				// Get assembly name.
				AssemblyID assemblyId;
				hr = pICorProfilerInfo->GetModuleInfo(moduleId, NULL, NULL,
						NULL, NULL, &assemblyId);
				if (SUCCEEDED(hr)) {
					assemblyNumber = (*assemblyMap)[assemblyId];
				}
			}

			info->assemblyNumber = assemblyNumber;
			info->classToken = classToken;
			info->functionToken = functionToken;
		}

		pMDImport->Release();
	}
	return hr;
}

/** Write name-value pair to log file. */
void CProfilerCallback::WriteTupleToFile(const char* key, const char* value) {
	WriteToFile(key);
	WriteToFile("=");
	WriteToFile(value);
	WriteToFile("\r\n");
}

/** Write to log file. */
int CProfilerCallback::WriteToFile(const char *pszFmtString, ...) {
	int retVal = 0;
	DWORD dwWritten = 0;
	CHAR g_szBuffer[4096];
	memset(g_szBuffer, 0, 4096);

	va_list args;
	va_start(args, pszFmtString);
	retVal = wvsprintf(g_szBuffer, pszFmtString, args);
	va_end(args);

	// Write out to the file if the file is open.
	if (resultFile != INVALID_HANDLE_VALUE) {
		EnterCriticalSection(&criticalSection);
		if (TRUE == WriteFile(resultFile, g_szBuffer,
						(DWORD) strlen(g_szBuffer), &dwWritten, NULL)) {
			retVal = dwWritten;
		} else {
			retVal = 0;
		}
		LeaveCriticalSection(&criticalSection);
	}
	

	return retVal;
}

/** Write a list of functionInfos values to the log. */
void CProfilerCallback::WriteToLog(const char* key,
		vector<FunctionInfo>* functions) {
	for (vector<FunctionInfo>::iterator i = functions->begin(); i != functions->end();
			i++) {
		FunctionInfo info = *i;
		char signature[NAME_BUFFER_SIZE];
		signature[0] = '\0';
		sprintf_s(signature, "%i:%i:%i", info.assemblyNumber, info.classToken,
				info.functionToken);
		WriteTupleToFile(key, signature);
	}
}