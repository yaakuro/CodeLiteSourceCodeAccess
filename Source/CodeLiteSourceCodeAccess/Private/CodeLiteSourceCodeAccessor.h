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

#pragma once

//#define USE_DBUS


#include "ISourceCodeAccessor.h"

#ifdef USE_DBUS

	#include <dbus/dbus.h>

#endif

class FCodeLiteSourceCodeAccessor : public ISourceCodeAccessor
{
public:

	/**
	 * Can we access source code.
	 */
	virtual bool CanAccessSourceCode() const override;

	/**
	 * Get the name of this accessor. Will be used as a unique identifier.
	 */
	virtual FName GetFName() const override;

	/**
	 * Get the name of this accessor.
	 */
	virtual FText GetNameText() const override;

	/**
	 * Get the descriptor of this accessor.
	 */
	virtual FText GetDescriptionText() const override;

	/**
	 * Open the CodeLite Workspace for editing.
	 */
	virtual bool OpenSolution() override;

	/**
	 * Open a file at a specific line and optional column.
	 */
	virtual bool OpenFileAtLine(const FString& FullPath, int32 LineNumber, int32 ColumnNumber = 0) override;

	/**
	 * Open a group of files.
	 */
	virtual bool OpenSourceFiles(const TArray<FString>& AbsoluteSourcePaths) override;

	/**
	 * Add a group of files.
	 */
	virtual bool AddSourceFiles(const TArray<FString>& AbsoluteSourcePaths, const TArray<FString>& AvailableModules) override;

	/**
	 * Save all open files.
	 */
	virtual bool SaveAllOpenDocuments() const override;

	/**
	 * Tick this source code accessor
	 * @param DeltaTime Delta time (in seconds) since the last call to Tick
	 */
	virtual void Tick(const float DeltaTime) override;

	/**
	 * Initialize the accessor.
	 */
	void Startup();

	/**
	 * Deinitialize the accessor.
	 */
	void Shutdown();

private:

	pid_t FindProcess(const char* name);
	
private:

	/** String storing the solution path obtained from the module manager to avoid having to use it on a thread */
	mutable FString CachedSolutionPath;

	/** Tests if CodeLite is present and returns path to it */
	bool CanRunCodeLite(FString& OutPath) const;

	/** Check if CodeLite is already running */
	bool IsIDERunning();

	/** Gets solution path */
	FString GetSolutionPath() const;

private:
#ifdef USE_DBUS
	DBusConnection* DBusConnection;
	DBusError 		DBusError;
#endif
};
