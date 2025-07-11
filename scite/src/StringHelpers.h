// SciTE - Scintilla based Text Editor
/** @file StringHelpers.h
 ** Definition of widely useful string functions.
 **/
// Copyright 2010 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef STRINGHELPERS_H
#define STRINGHELPERS_H

// String contain checks

bool Contains(std::string const &s, char ch) noexcept;
bool isprefix(const char *target, const char *prefix) noexcept;

// Modifications: substitution, remove, trim

// Substitute is duplicated instead of templated as it was ambiguous when implemented as a template.
int Substitute(std::wstring &s, std::wstring_view sFind, std::wstring_view sReplace);
int Substitute(std::string &s, std::string_view sFind, std::string_view sReplace);

template <typename T>
int Remove(T &s, const T &sFind) {
	return Substitute(s, sFind, T());
}

bool RemoveStringOnce(std::string &s, const char *marker);
void Trim(std::string &s);
void StripEOL(std::string &s);

// String to/from number conversions

std::string StdStringFromInteger(int i);
std::string StdStringFromSizeT(size_t i);
std::string StdStringFromDouble(double d, int precision);

int IntegerFromString(const std::string &val, int defaultValue);
intptr_t IntPtrFromString(const std::string &val, intptr_t defaultValue);
long long LongLongFromString(const std::string &val, long long defaultValue);
int IntFromString(std::u32string_view s) noexcept;
intptr_t IntegerFromText(const char *s) noexcept;

constexpr int hexBase = 16;

unsigned int IntFromHexDigit(int ch) noexcept;
bool AllBytesHex(std::string_view hexBytes) noexcept;
int IntFromHexByte(std::string_view hexByte) noexcept;
unsigned int IntFromHexBytes(std::string_view hexBytes) noexcept;

// Character type tests

constexpr char MakeUpperCase(char ch) noexcept {
	if (ch < 'a' || ch > 'z')
		return ch;
	else
		return ch - 'a' + 'A';
}

constexpr char MakeLowerCase(char c) noexcept {
	if (c >= 'A' && c <= 'Z') {
		return c - 'A' + 'a';
	} else {
		return c;
	}
}

constexpr bool IsASCII(int ch) noexcept {
	return (ch >= 0) && (ch < 0x80);
}

constexpr bool IsASpace(int ch) noexcept {
	return (ch == ' ') || ((ch >= 0x09) && (ch <= 0x0d));
}

constexpr bool IsSpaceOrTab(int ch) noexcept {
	return (ch == ' ') || (ch == '\t');
}

constexpr bool IsEOLCharacter(int ch) noexcept {
	return ch == '\r' || ch == '\n';
}

constexpr bool IsADigit(int ch) noexcept {
	return (ch >= '0') && (ch <= '9');
}

constexpr bool IsAHexDigit(int ch) noexcept {
	return
		((ch >= '0') && (ch <= '9')) ||
		((ch >= 'a') && (ch <= 'f')) ||
		((ch >= 'A') && (ch <= 'F'));
}

constexpr bool IsUpperCase(int ch) noexcept {
	return (ch >= 'A') && (ch <= 'Z');
}

constexpr bool IsAlphabetic(int ch) noexcept {
	return ((ch >= 'A') && (ch <= 'Z')) || ((ch >= 'a') && (ch <= 'z'));
}

constexpr bool IsAlphaNumeric(int ch) noexcept {
	return
		((ch >= '0') && (ch <= '9')) ||
		((ch >= 'a') && (ch <= 'z')) ||
		((ch >= 'A') && (ch <= 'Z'));
}

// Splitting strings into vectors, sets, and pairs

// StringSplit can be expanded over std::string or GUI::gui_string
template <typename T>
std::vector<T> StringSplit(const T &text, int separator) {
	std::vector<T> vs(text.empty() ? 0 : 1);
	for (typename T::const_iterator it=text.begin(); it!=text.end(); ++it) {
		if (*it == separator) {
			vs.push_back(T());
		} else {
			vs.back() += *it;
		}
	}
	return vs;
}

inline std::vector<GUI::gui_string> ListFromString(const GUI::gui_string &args) {
	return StringSplit(args, '\n');
}

std::set<std::string> SetFromString(std::string_view text, char separator);

using ViewPair = std::tuple<std::string_view, std::string_view>;

// Split view around first separator returning the portion before and after the separator.
// If the separator is not present then return whole view and an empty view.
inline ViewPair ViewSplit(std::string_view view, char separator) noexcept {
	const size_t sepPos = view.find_first_of(separator);
	std::string_view first = view.substr(0, sepPos);
	std::string_view second = sepPos == (std::string_view::npos) ? "" : view.substr(sepPos + 1);
	return { first, second };
}

// Safer version of string copy functions like strcpy, wcsncpy, etc.
// Instantiate over fixed length strings of both char and wchar_t.
// May truncate if source doesn't fit into dest with room for NUL.

template <typename T, size_t count>
void StringCopy(T(&dest)[count], const T *source) noexcept {
	for (size_t i=0; i<count; i++) {
		dest[i] = source[i];
		if (!source[i])
			break;
	}
	dest[count-1] = 0;
}

// Case insensitive comparison

int CompareNoCase(const char *a, const char *b) noexcept;
bool EqualCaseInsensitive(const char *a, const char *b) noexcept;
bool EqualCaseInsensitive(std::string_view a, std::string_view b) noexcept;

// Basic case lowering that converts A-Z to a-z.
// Does not handle non-ASCII characters.
void LowerCaseAZ(std::string &s);

constexpr const char *UTF8BOM = "\xef\xbb\xbf";

// Unicode conversions

std::u32string UTF32FromUTF8(std::string_view s);
unsigned int UTF32Character(std::string_view utf8) noexcept;
std::string UTF8FromUTF32(unsigned int uch);

// DBCS

bool IsDBCSLeadByte(int codePage, char ch) noexcept;

// Escape processing

std::string Slash(const std::string &s, bool quoteQuotes);
std::string UnSlashLowOctalString(std::string_view sv);
std::string UnSlashString(std::string_view sv);
std::string UnicodeUnEscape(std::string_view s);

class ILocalize {
public:
	virtual GUI::gui_string Text(std::string_view sv, bool retainIfNotFound=true) const = 0;
};

/**
 * ComboMemory is a fixed length list of strings suitable for display in combo boxes
 * as a memory of user entries.
 */

constexpr size_t comboMemorySize = 10;

class ComboMemory {
	size_t sz;
	std::vector<std::string> entries;
	bool Present(std::string_view sv) const noexcept;
public:
	explicit ComboMemory(size_t sz_=comboMemorySize);
	void Insert(std::string_view item);
	void InsertDeletePrefix(std::string_view item);
	void Append(std::string_view item);
	size_t Length() const noexcept;
	std::string At(size_t n) const;
	std::vector<std::string> AsVector() const;
};

#endif
