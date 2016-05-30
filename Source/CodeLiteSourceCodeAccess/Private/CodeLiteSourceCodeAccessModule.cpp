// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "CodeLiteSourceCodeAccessPrivatePCH.h"
#include "Runtime/Core/Public/Features/IModularFeatures.h"
#include "CodeLiteSourceCodeAccessModule.h"

IMPLEMENT_MODULE( FCodeLiteSourceCodeAccessModule, CodeLiteSourceCodeAccess );

void FCodeLiteSourceCodeAccessModule::StartupModule()
{
	IModularFeatures::Get().RegisterModularFeature(TEXT("SourceCodeAccessor"), &CodeLiteSourceCodeAccessor );
}

void FCodeLiteSourceCodeAccessModule::ShutdownModule()
{
	IModularFeatures::Get().UnregisterModularFeature(TEXT("SourceCodeAccessor"), &CodeLiteSourceCodeAccessor);
}

FCodeLiteSourceCodeAccessor& FCodeLiteSourceCodeAccessModule::GetAccessor()
{
	return CodeLiteSourceCodeAccessor;
}
