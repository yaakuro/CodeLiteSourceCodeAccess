/*
	The MIT License (MIT)

	Copyright (c) 2015 Cengiz Terzibas

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in
	all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
	THE SOFTWARE.
*/

#include "CodeLiteSourceCodeAccessPrivatePCH.h"
#include "CodeLiteSourceCodeAccessor.h"
#include "CodeLiteSourceCodeAccessModule.h"
#include "ISourceCodeAccessModule.h"
#include "ModuleManager.h"
#include "DesktopPlatformModule.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>

#if WITH_EDITOR
#include "Developer/HotReload/Public/IHotReload.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogCodeLiteAccessor, Log, All);

#define LOCTEXT_NAMESPACE "CodeLiteSourceCodeAccessor"

void FCodeLiteSourceCodeAccessor::Startup()
{
	// Cache this so we don't have to do it on a background thread
	GetSolutionPath();

#ifdef USE_DBUS
	dbus_error_init (&DBusError);

	DBusConnection = dbus_bus_get(DBUS_BUS_SESSION, &DBusError);
	if(dbus_error_is_set(&DBusError))
	{
		printf("Error connecting to the daemon bus: %s",DBusError.message);
	}

	UE_LOG(LogCodeLiteAccessor, Warning, TEXT("Successfully connected to CodeLite DBus server."));
#endif
}

void FCodeLiteSourceCodeAccessor::Shutdown()
{
#ifdef USE_DBUS
	// Free up allocated dbus error object.  
	dbus_error_free(&DBusError);
	
	// Unreference the connections object.
	dbus_connection_unref(DBusConnection);
	
	UE_LOG(LogCodeLiteAccessor, Warning, TEXT("Successfully disconnected from the CodeLite DBus server."));
#endif
}

bool FCodeLiteSourceCodeAccessor::CanAccessSourceCode() const
{
	FString Path;
	return CanRunCodeLite(Path);
}

FName FCodeLiteSourceCodeAccessor::GetFName() const
{
	return FName("CodeLiteSourceCodeAccessor");
}

FText FCodeLiteSourceCodeAccessor::GetNameText() const
{
	return LOCTEXT("CodeLiteDisplayName", "CodeLite 7/8.x");
}

FText FCodeLiteSourceCodeAccessor::GetDescriptionText() const
{
	return LOCTEXT("CodeLiteDisplayDesc", "Open source code files in CodeLite");
}

bool FCodeLiteSourceCodeAccessor::OpenSolution()
{

	FString Filename = FPaths::GetBaseFilename(GetSolutionPath()) + ".workspace";
	FString Directory = FPaths::GetPath(GetSolutionPath());
	FString Solution = "\"" + Directory + "/" + Filename + "\"";
	FString CodeLitePath;
	if(!CanRunCodeLite(CodeLitePath))
	{
		UE_LOG(LogCodeLiteAccessor, Warning, TEXT("FCodeLiteSourceCodeAccessor::OpenSolution: Cannot find CodeLite binary"));
		return false;
	}

	UE_LOG(LogCodeLiteAccessor, Warning, TEXT("FCodeLiteSourceCodeAccessor::OpenSolution: %s %s"), *CodeLitePath, *Solution);
	
#ifdef USE_DBUS

	//
	// TODO Somehow codelite is not opening the workspace using GetWorkspace()->Open(...)
	//
	DBusMessage* message = nullptr;
	DBusMessageIter args;
		
	// Create new message.
	message = dbus_message_new_signal ("/org/codelite/command", "org.codelite.command", "OpenWorkSpace");

	char* fileName = TCHAR_TO_ANSI(*Solution);

	// Add parameters to the message.
	dbus_message_iter_init_append(message, &args);
	if(!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &fileName)) {
		UE_LOG(LogCodeLiteAccessor, Warning, TEXT("Sdbus_message_iter_append_basic failed."));
		return false;
	}
	
	// Send the message.
	dbus_connection_send(DBusConnection, message, nullptr);
	if(dbus_error_is_set(&DBusError))
	{
		UE_LOG(LogCodeLiteAccessor, Warning, TEXT("dbus_connection_send failed: %s"), DBusError.message);
		return false;
	}
	// Free the message resources.
	dbus_message_unref(message);
	
	return true;

#else

	FProcHandle Proc = FPlatformProcess::CreateProc(*CodeLitePath, *Solution, true, false, false, nullptr, 0, nullptr, nullptr);
	if(Proc.IsValid())
	{
		FPlatformProcess::CloseProc(Proc);
		return true;
	}
	return false;

#endif

}

bool FCodeLiteSourceCodeAccessor::OpenSourceFiles(const TArray<FString>& AbsoluteSourcePaths)
{
	FString CodeLitePath;
	if(!CanRunCodeLite(CodeLitePath))
	{
		UE_LOG(LogCodeLiteAccessor, Warning, TEXT("FCodeLiteSourceCodeAccessor::OpenSourceFiles: Cannot find CodeLite binary"));
		return false;
	}

#ifdef USE_DBUS	

	for(const auto& SourcePath : AbsoluteSourcePaths)
	{
		
		DBusMessage* message = nullptr;
		DBusMessageIter args;
		
		// Create new message.
		message = dbus_message_new_signal ("/org/codelite/command", "org.codelite.command", "OpenFile");

		char* fileName = TCHAR_TO_ANSI(*SourcePath);

		// Add parameters to the message.
		dbus_message_iter_init_append(message, &args);
		if(!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &fileName)) {
			UE_LOG(LogCodeLiteAccessor, Warning, TEXT("Sdbus_message_iter_append_basic failed."));
			return false;
		}
		
		// Send the message.
		dbus_connection_send(DBusConnection, message, nullptr);
		if(dbus_error_is_set(&DBusError))
		{
			UE_LOG(LogCodeLiteAccessor, Warning, TEXT("dbus_connection_send failed: %s"), DBusError.message);
			return false;
		}

		// Free the message resources.
		dbus_message_unref(message);

	}

	dbus_connection_flush(DBusConnection);

	return true;

#else

	for(const auto& SourcePath : AbsoluteSourcePaths)
	{
		const FString Path = FString::Printf(TEXT("\"%s\""), *SourcePath);

		FProcHandle Proc = FPlatformProcess::CreateProc(*CodeLitePath, *Path, true, false, false, nullptr, 0, nullptr, nullptr);
		if(Proc.IsValid())
		{
			UE_LOG(LogCodeLiteAccessor, Warning, TEXT("CodeLiteSourceCodeAccessor::OpenSourceFiles: %s"), *Path);
			FPlatformProcess::CloseProc(Proc);
			return true;
		}
	}

#endif

	return false;
}

bool FCodeLiteSourceCodeAccessor::OpenFileAtLine(const FString& FullPath, int32 LineNumber, int32 ColumnNumber)
{
	FString CodeLitePath;
	if(!CanRunCodeLite(CodeLitePath))
	{
		UE_LOG(LogCodeLiteAccessor,Warning, TEXT("FCodeLiteSourceCodeAccessor::OpenFileAtLine: Cannot find CodeLite binary"));
		return false;
	}
	
#ifdef USE_DBUS	
	
	DBusMessage* message = nullptr;
	DBusMessageIter args;
	
	// Create new message.
	message = dbus_message_new_signal ("/org/codelite/command", "org.codelite.command", "OpenFileAtLine");

	char* fileName = TCHAR_TO_ANSI(*FullPath);

	// Add parameters to the message.
	dbus_message_iter_init_append(message, &args);
	if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &fileName)) {
		printf("FCodeLiteSourceCodeAccessor::OpenFileAtLine: dbus_message_iter_append_basic failed.\n");
	}
	if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &LineNumber)) {
		printf("FCodeLiteSourceCodeAccessor::OpenFileAtLine: dbus_message_iter_append_basic failed.\n");
	}
	
	// Send the message.
	dbus_connection_send(DBusConnection, message, nullptr);
	if(dbus_error_is_set(&DBusError))
	{
		printf("dbus_connection_send failed (%s)\n", DBusError.message);
		return false;
	}
	dbus_connection_flush(DBusConnection);
	
	// Free the message resources.
	dbus_message_unref (message);
	
#else

	const FString Path = FString::Printf(TEXT("\"%s --line=%d\""), *FullPath, LineNumber);

	if(FPlatformProcess::CreateProc(*CodeLitePath, *Path, true, true, false, nullptr, 0, nullptr, nullptr).IsValid())
	{
		UE_LOG(LogCodeLiteAccessor, Warning, TEXT("FCodeLiteSourceCodeAccessor::OpenSolution: Cannot find CodeLite binary"));
	}
	
#endif

	UE_LOG(LogCodeLiteAccessor, Warning, TEXT("CodeLiteSourceCodeAccessor::OpenFileAtLine: %s %d"), *FullPath, LineNumber);

	return true;
}

bool FCodeLiteSourceCodeAccessor::AddSourceFiles(const TArray<FString>& AbsoluteSourcePaths, const TArray<FString>& AvailableModules)
{
#ifdef USE_DBUS	
	for(const auto& SourcePath : AbsoluteSourcePaths)
	{
		DBusMessage* message = nullptr;
		DBusMessageIter args;
		
		// Create new message.
		message = dbus_message_new_signal ("/org/codelite/command", "org.codelite.command", "AddFile");

		char* fileName = TCHAR_TO_ANSI(*SourcePath);

		// Add parameters to the message.
		dbus_message_iter_init_append(message, &args);
		if(!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &fileName)) {
			printf("FCodeLiteSourceCodeAccessor::dbus_message_iter_append_basic failed.\n");
			return false;
		}
		
		// Send the message.
		dbus_connection_send(DBusConnection, message, nullptr);
		if(dbus_error_is_set(&DBusError))
		{
			printf("dbus_connection_send failed (%s)\n", DBusError.message);
			return false;
		}

		// Free the message resources.
		dbus_message_unref(message);
		
		printf("FCodeLiteSourceCodeAccessor::AddSourceFiles: %s\n", fileName);
	}

	dbus_connection_flush(DBusConnection);

	return true;

#endif

	// TODO Is this possible without dbus here? Maybe we can add this functionality to CodeLite.
	return false;
}

bool FCodeLiteSourceCodeAccessor::SaveAllOpenDocuments() const
{
#ifdef USE_DBUS	

	DBusMessage* message = nullptr;
	DBusMessageIter args;
	
	// Create new message.
	message = dbus_message_new_signal ("/org/codelite/command", "org.codelite.command", "SaveAllFiles");

	// Send the message.
	dbus_connection_send(DBusConnection, message, nullptr);
	if(dbus_error_is_set(&DBusError))
	{
		printf("dbus_connection_send failed (%s)\n", DBusError.message);
		return false;
	}
	dbus_connection_flush(DBusConnection);
	
	// Free the message resources.
	dbus_message_unref (message);

	printf("FCodeLiteSourceCodeAccessor::SaveAllOpenDocuments.\n");
#else
	// TODO Is this possible without dbus here? Maybe we can add this functionality to CodeLite.
	return false;
#endif

	return true;
}

void FCodeLiteSourceCodeAccessor::Tick(const float DeltaTime)
{
	// TODO What can we do here?
}


bool FCodeLiteSourceCodeAccessor::CanRunCodeLite(FString& OutPath) const
{
	// TODO This might be not a good idea to find an executable.
	OutPath = TEXT("/usr/bin/codelite");
	return FPaths::FileExists(OutPath);
}


bool FCodeLiteSourceCodeAccessor::IsIDERunning()
{
#ifdef PLATFORM_LINUX
	pid_t pid = FindProcess("codelite");
	if(pid == -1)
	{
		return false;
	}

	return true;
#endif
	return false;

}

FString FCodeLiteSourceCodeAccessor::GetSolutionPath() const
{
	if(IsInGameThread())
	{
		FString SolutionPath;
		if(FDesktopPlatformModule::Get()->GetSolutionPath(SolutionPath))
		{
			CachedSolutionPath = FPaths::ConvertRelativePathToFull(SolutionPath);
		}
	}
	return CachedSolutionPath;
}

#ifdef PLATFORM_LINUX
pid_t FCodeLiteSourceCodeAccessor::FindProcess(const char* name)
{
	// TODO This is only to test. Will be changed later.

	DIR* dir;
	struct dirent* ent;
	char* endptr;
	char buf[512];

	if(!(dir = opendir("/proc")))
	{
		perror("can't open /proc");
		return -1;
	}

	while((ent = readdir(dir)) != NULL)
	{
		long lpid = strtol(ent->d_name, &endptr, 10);
		if(*endptr != '\0')
		{
			continue;
		}

		// Try to open the cmdline file
		snprintf(buf, sizeof(buf), "/proc/%ld/cmdline", lpid);
		FILE* fp = fopen(buf, "r");
		if(fp)
		{
			if(fgets(buf, sizeof(buf), fp) != NULL)
			{
				// check the first token in the file, the program name.
				char* first = strtok(buf, " ");
				if(!strcmp(first, name))
				{
					fclose(fp);
					closedir(dir);
					return (pid_t)lpid;
				}
			}
			fclose(fp);
		}
	}

	closedir(dir);

	return -1;
}
#endif

#undef LOCTEXT_NAMESPACE
