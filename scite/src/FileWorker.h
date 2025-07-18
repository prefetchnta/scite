// SciTE - Scintilla based Text Editor
/** @file FileWorker.h
 ** Definition of classes to perform background file tasks as threads.
 **/
// Copyright 2011 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef FILEWORKER_H
#define FILEWORKER_H

/// Base size of file I/O operations.
constexpr size_t blockSize = 128 * 1024;

struct FileWorker : public Worker {
	WorkerListener *pListener;
	FilePath path;
	size_t size;
	int err;
	FILE *fp;
	GUI::ElapsedTime et;
	int sleepTime;
	double nextProgress;

	FileWorker(WorkerListener *pListener_, FilePath path_, size_t size_, FILE *fp_) noexcept;
	~FileWorker() noexcept override;
	virtual double Duration() noexcept;
	void Cancel() noexcept override {
		Worker::Cancel();
	}
	virtual bool IsLoading() const noexcept = 0;
};

class FileLoader : public FileWorker {
public:
	Scintilla::ILoader *pLoader;
	size_t readSoFar;
	UniMode unicodeMode;

	FileLoader(WorkerListener *pListener_, Scintilla::ILoader *pLoader_, const FilePath &path_, size_t size_, FILE *fp_);
	void Execute() noexcept override;
	void Cancel() noexcept override;
	bool IsLoading() const noexcept override {
		return true;
	}
};

class FileStorer : public FileWorker {
public:
	const char *documentBytes;
	size_t writtenSoFar;
	UniMode unicodeMode;
	bool visibleProgress;
	std::unique_ptr<Utf8_16::Writer> convert;

	FileStorer(WorkerListener *pListener_, std::string_view bytes_, const FilePath &path_,
		   FILE *fp_, UniMode unicodeMode_, bool visibleProgress_);
	void Execute() noexcept override;
	void Cancel() noexcept override;
	bool IsLoading() const noexcept override {
		return false;
	}
};

enum {
	WORK_FILEREAD = 1,
	WORK_FILEWRITTEN = 2,
	WORK_FILEPROGRESS = 3,
	WORK_PLATFORM = 100
};

#endif
