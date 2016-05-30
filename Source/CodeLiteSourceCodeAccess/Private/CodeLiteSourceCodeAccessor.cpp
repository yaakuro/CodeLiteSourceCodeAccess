// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "CodeLiteSourceCodeAccessPrivatePCH.h"
#include "CodeLiteSourceCodeAccessor.h"
#include "CodeLiteSourceCodeAccessModule.h"
#include "ISourceCodeAccessModule.h"
#include "ModuleManager.h"
#include "DesktopPlatformModule.h"

DEFINE_LOG_CATEGORY_STATIC(LogCodeLiteAccessor, Log, All);

#define LOCTEXT_NAMESPACE "CodeLiteSourceCodeAccessor"


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
	
	FProcHandle Proc = FPlatformProcess::CreateProc(*CodeLitePath, *Solution, true, false, false, nullptr, 0, nullptr, nullptr);
	if(Proc.IsValid())
	{
		FPlatformProcess::CloseProc(Proc);
		return true;
	}
	return false;

}

bool FCodeLiteSourceCodeAccessor::OpenSourceFiles(const TArray<FString>& AbsoluteSourcePaths)
{
	FString CodeLitePath;
	if(!CanRunCodeLite(CodeLitePath))
	{
		UE_LOG(LogCodeLiteAccessor, Warning, TEXT("FCodeLiteSourceCodeAccessor::OpenSourceFiles: Cannot find CodeLite binary"));
		return false;
	}

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

	const FString Path = FString::Printf(TEXT("\"%s --line=%d\""), *FullPath, LineNumber);

	if(FPlatformProcess::CreateProc(*CodeLitePath, *Path, true, true, false, nullptr, 0, nullptr, nullptr).IsValid())
	{
		UE_LOG(LogCodeLiteAccessor, Warning, TEXT("FCodeLiteSourceCodeAccessor::OpenSolution: Cannot find CodeLite binary"));
	}

	UE_LOG(LogCodeLiteAccessor, Warning, TEXT("CodeLiteSourceCodeAccessor::OpenFileAtLine: %s %d"), *FullPath, LineNumber);

	return true;
}

bool FCodeLiteSourceCodeAccessor::AddSourceFiles(const TArray<FString>& AbsoluteSourcePaths, const TArray<FString>& AvailableModules)
{
	// TODO Is this possible without dbus here? Maybe we can add this functionality to CodeLite.
	return false;
}

bool FCodeLiteSourceCodeAccessor::SaveAllOpenDocuments() const
{
	// TODO Is this possible without dbus here? Maybe we can add this functionality to CodeLite.
	return false;
}

void FCodeLiteSourceCodeAccessor::Tick(const float DeltaTime)
{
	// TODO What can we do here?
}


bool FCodeLiteSourceCodeAccessor::CanRunCodeLite(FString& OutPath) const
{
#if PLATFORM_LINUX
	if(!GConfig->GetString(TEXT("/Script/SourceCodeAccess.SourceCodeAccessSettings"), TEXT("FileName"), OutPath, GEngineIni))
	{
		// Use default if nothing is specified in the ini file.
		OutPath = TEXT("/usr/bin/codelite");
	}
	return IFileManager::Get().FileExists(*OutPath);

#elif PLATFORM_MAC
	if(!GConfig->GetString(TEXT("/Script/SourceCodeAccess.SourceCodeAccessSettings"), TEXT("FileName"), OutPath, GEngineIni))
	{
		// Use default if nothing is specified in the ini file.
		OutPath = TEXT("/Applications/codelite.app");
	}
	return IFileManager::Get().DirectoryExists(*OutPath);
#elif PLATFORM_WINDOWS 
	if(!GConfig->GetString(TEXT("/Script/SourceCodeAccess.SourceCodeAccessSettings"), TEXT("FileName"), OutPath, GEngineIni))
	{
		// Use default if nothing is specified in the ini file.
		OutPath = TEXT("C:/Program Files/CodeLite/CodeLite.exe");
	}

	if(!IFileManager::Get().FileExists(*OutPath))
	{
		OutPath = TEXT("C:/Program Files (x86)/CodeLite/CodeLite.exe");
		return IFileManager::Get().FileExists(*OutPath);
	}
#else
	// No supported platform.
	UE_LOG(LogCodeLiteAccessor, Warning, TEXT("This platform is not supported by the CodeLiteSourceCodeAccessor."));
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

void FCodeLiteSourceCodeAccessor::RefreshAvailability()
{
	
}

#undef LOCTEXT_NAMESPACE