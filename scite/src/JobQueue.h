// SciTE - Scintilla based Text Editor
/** @file JobQueue.h
 ** Define job queue
 **/
// SciTE & Scintilla copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// Copyright 2007 by Neil Hodgson <neilh@scintilla.org>, from April White <april_white@sympatico.ca>
// The License.txt file describes the conditions under which this software may be distributed.

// TODO: see http://www.codeproject.com/threads/cppsyncstm.asp

#ifndef JOBQUEUE_H
#define JOBQUEUE_H

enum class JobSubsystem {
	cli = 0, gui = 1, shell = 2, extension = 3, help = 4, otherHelp = 5, grep = 6, immediate = 7
};

JobSubsystem SubsystemFromChar(char c) noexcept;

enum JobFlags {
	jobForceQueue = 1,
	jobHasInput = 2,
	jobQuiet = 4,
	// 8 reserved for jobVeryQuiet
	jobRepSelMask = 48,
	jobRepSelYes = 16,
	jobRepSelAuto = 32,
	jobGroupUndo = 64,
	jobLowPriority = 128
};

struct JobMode {
	JobSubsystem jobType;
	int saveBefore;
	bool isFilter;
	int flags;
	std::string input;
	JobMode(const PropSetFile &props, int item, std::string_view fileNameExt);
};

class Job {
public:
	std::string command;
	FilePath directory;
	JobSubsystem jobType;
	std::string input;
	int flags;

	Job() noexcept;
	Job(std::string_view command_, FilePath directory_, JobSubsystem jobType_, std::string_view input_, int flags_);
	void Clear() noexcept;
};

class JobQueue {
	std::atomic_bool cancelFlag;
public:
	std::mutex mutex;
	std::atomic_bool clearBeforeExecute;
	std::atomic_bool isBuilding;
	std::atomic_bool isBuilt;
	std::atomic_bool executing;
	static constexpr size_t commandMax = 2;
	std::atomic_size_t commandCurrent;
	std::vector<Job> jobQueue;
	std::atomic_bool jobUsesOutputPane;
	std::atomic_bool timeCommands;

	JobQueue();
	// Deleted so JobQueue objects can not be copied.
	JobQueue(const JobQueue &) = delete;
	JobQueue(JobQueue &&) = delete;
	JobQueue &operator=(const JobQueue &) = delete;
	JobQueue &operator=(JobQueue &&) = delete;
	~JobQueue();
	bool TimeCommands() const noexcept;
	bool ClearBeforeExecute() const noexcept;
	bool ShowOutputPane() const noexcept;
	bool IsExecuting() const noexcept;
	void SetExecuting(bool state) noexcept;
	bool HasCommandToRun() const noexcept;
	bool SetCancelFlag(bool value);
	bool Cancelled() noexcept;

	void ClearJobs() noexcept;
	void AddCommand(std::string_view command, const FilePath &directory, JobSubsystem jobType, std::string_view input, int flags);
};

#endif
