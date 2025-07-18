// SciTE - Scintilla based Text Editor
/** @file JobQueue.cxx
 ** Define job queue
 **/
// SciTE & Scintilla copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// Copyright 2007 by Neil Hodgson <neilh@scintilla.org>, from April White <april_white@sympatico.ca>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <ctime>

#include <compare>
#include <tuple>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <memory>
#include <chrono>
#include <atomic>
#include <mutex>

#include <sys/stat.h>

#include "GUI.h"

#include "StringHelpers.h"
#include "FilePath.h"
#include "PropSetFile.h"
#include "SciTE.h"
#include "JobQueue.h"

JobSubsystem SubsystemFromChar(char c) noexcept {
	switch (c) {
	case '1':
		return JobSubsystem::gui;
	case '2':
		return JobSubsystem::shell;
	case '3':
		return JobSubsystem::extension;
	case '4':
		return JobSubsystem::help;
	case '5':
		return JobSubsystem::otherHelp;
	case '6':
		return JobSubsystem::grep;
	case '7':
		return JobSubsystem::immediate;
	default:
		return JobSubsystem::cli;
	}
}

namespace {

void SetOptionFromValidString(bool &option, const std::string &s) noexcept {
	if (s.empty() || s[0] == '1' || s == "yes")
		option = true;
	else if (s[0] == '0' || s == "no")
		option = false;
}

}

JobMode::JobMode(const PropSetFile &props, int item, std::string_view fileNameExt) : jobType(JobSubsystem::cli), saveBefore(0), isFilter(false), flags(0) {
	bool quiet = false;
	int repSel = 0;
	bool groupUndo = false;
	bool lowPriority = false;

	const std::string itemSuffix = StdStringFromInteger(item) + ".";
	std::string propName = std::string("command.mode.") + itemSuffix;
	std::string modeVal(props.GetNewExpandString(propName, fileNameExt));

	std::erase(modeVal, ' ');
	std::vector<std::string> modes = StringSplit(modeVal, ',');
	for (const std::string &mode : modes) {

		std::vector<std::string> optValue = StringSplit(mode, ':');

		if (optValue.empty()) {
			continue;
		}

		const std::string opt = optValue[0];
		const std::string value = (optValue.size() > 1) ? optValue[1] : std::string();

		if (opt == "subsystem" && !value.empty()) {
			if (value[0] == '0' || value == "console")
				jobType = JobSubsystem::cli;
			else if (value[0] == '1' || value == "windows")
				jobType = JobSubsystem::gui;
			else if (value[0] == '2' || value == "shellexec")
				jobType = JobSubsystem::shell;
			else if (value[0] == '3' || value == "lua" || value == "director")
				jobType = JobSubsystem::extension;
			else if (value[0] == '4' || value == "htmlhelp")
				jobType = JobSubsystem::help;
			else if (value[0] == '5' || value == "winhelp")
				jobType = JobSubsystem::otherHelp;
			else if (value[0] == '7' || value == "immediate")
				jobType = JobSubsystem::immediate;
		}

		if (opt == "quiet") {
			if (value.empty() || value[0] == '1' || value == "yes")
				quiet = true;
			else if (value[0] == '0' || value == "no")
				quiet = false;
		}

		if (opt == "savebefore") {
			if (value.empty() || value[0] == '1' || value == "yes")
				saveBefore = 1;
			else if (value[0] == '0' || value == "no")
				saveBefore = 2;
			else if (value == "prompt")
				saveBefore = 0;
		}

		if (opt == "filter") {
			SetOptionFromValidString(isFilter, value);
		}

		if (opt == "replaceselection") {
			if (value.empty() || value[0] == '1' || value == "yes")
				repSel = 1;
			else if (value[0] == '0' || value == "no")
				repSel = 0;
			else if (value == "auto")
				repSel = 2;
		}

		if (opt == "groupundo") {
			SetOptionFromValidString(groupUndo, value);
		}

		if (opt == "lowpriority") {
			SetOptionFromValidString(lowPriority, value);
		}
	}

	// The mode flags also have classic properties with similar effect.
	// If the classic property is specified, it overrides the mode.
	// To see if the property is absent (as opposed to merely evaluating
	// to nothing after variable expansion), use GetWild for the
	// existence check.  However, for the value check, use GetNewExpandString.

	propName = "command.save.before.";
	propName += itemSuffix;
	saveBefore = IntegerFromString(props.GetNewExpandString(propName, fileNameExt), saveBefore);

	propName = "command.is.filter.";
	propName += itemSuffix;
	if (!props.GetWild(propName, fileNameExt).empty())
		isFilter = (props.GetNewExpandString(propName, fileNameExt) == "1");

	propName = "command.subsystem.";
	propName += itemSuffix;
	if (!props.GetWild(propName, fileNameExt).empty()) {
		std::string subsystemVal = props.GetNewExpandString(propName, fileNameExt);
		jobType = SubsystemFromChar(subsystemVal[0]);
	}

	propName = "command.input.";
	propName += itemSuffix;
	if (!props.GetWild(propName, fileNameExt).empty()) {
		input = props.GetNewExpandString(propName, fileNameExt);
		flags |= jobHasInput;
	}

	propName = "command.quiet.";
	propName += itemSuffix;
	if (!props.GetWild(propName, fileNameExt).empty())
		quiet = props.GetNewExpandString(propName, fileNameExt) == "1";
	if (quiet)
		flags |= jobQuiet;

	propName = "command.replace.selection.";
	propName += itemSuffix;
	repSel = IntegerFromString(props.GetNewExpandString(propName, fileNameExt), repSel);

	if (repSel == 1)
		flags |= jobRepSelYes;
	else if (repSel == 2)
		flags |= jobRepSelAuto;

	if (groupUndo)
		flags |= jobGroupUndo;

	if (lowPriority)
		flags |= jobLowPriority;
}

Job::Job() noexcept : jobType(JobSubsystem::cli), flags(0) {
	Clear();
}

Job::Job(std::string_view command_, FilePath directory_, JobSubsystem jobType_, std::string_view input_, int flags_)
	: command(std::move(command_)), directory(std::move(directory_)), jobType(jobType_), input(input_), flags(flags_) {
}

void Job::Clear() noexcept {
	command.clear();
	directory.Init();
	jobType = JobSubsystem::cli;
	input.clear();
	flags = 0;
}


JobQueue::JobQueue() : jobQueue(commandMax) {
	clearBeforeExecute = false;
	isBuilding = false;
	isBuilt = false;
	executing = false;
	commandCurrent = 0;
	jobUsesOutputPane = false;
	cancelFlag = false;
	timeCommands = false;
}

JobQueue::~JobQueue() = default;

bool JobQueue::TimeCommands() const noexcept {
	return timeCommands;
}

bool JobQueue::ClearBeforeExecute() const noexcept {
	return clearBeforeExecute;
}

bool JobQueue::ShowOutputPane() const noexcept {
	return jobUsesOutputPane;
}

bool JobQueue::IsExecuting() const noexcept {
	return executing;
}

void JobQueue::SetExecuting(bool state) noexcept {
	executing = state;
}

bool JobQueue::HasCommandToRun() const noexcept {
	return commandCurrent > 0;
}

bool JobQueue::SetCancelFlag(bool value) {
	std::lock_guard<std::mutex> guard(mutex);
	const bool cancelFlagPrevious = cancelFlag;
	cancelFlag = value;
	return cancelFlagPrevious;
}

bool JobQueue::Cancelled() noexcept {
	return cancelFlag;
}

void JobQueue::ClearJobs() noexcept {
	for (Job &ic : jobQueue) {
		ic.Clear();
	}
	commandCurrent = 0;
}

void JobQueue::AddCommand(std::string_view command, const FilePath &directory, JobSubsystem jobType, std::string_view input, int flags) {
	if ((commandCurrent < commandMax) && (!command.empty())) {
		if (commandCurrent == 0)
			jobUsesOutputPane = false;
		jobQueue[commandCurrent] = Job(command, directory, jobType, input, flags);
		commandCurrent++;
		if (jobType == JobSubsystem::cli && !(flags & jobQuiet))
			jobUsesOutputPane = true;
		// For JobSubsystem::extension, the Trace() method shows output pane on demand.
	}
}
