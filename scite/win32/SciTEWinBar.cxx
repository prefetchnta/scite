// SciTE - Scintilla based Text Editor
/** @file SciTEWinBar.cxx
 ** Bar and menu code for the Windows version of the editor.
 **/
// Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include "SciTEWin.h"

/**
 * Set up properties for FileTime, FileDate, CurrentTime, CurrentDate and FileAttr.
 */
void SciTEWin::SetFileProperties(
	PropSetFile &ps) {			///< Property set to update.

	constexpr int TEMP_LEN = 100;
	wchar_t temp[TEMP_LEN] = L"";
	HANDLE hf = ::CreateFileW(filePath.AsInternal(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, {});
	if (hf != INVALID_HANDLE_VALUE) {
		FILETIME ft = FILETIME();
		::GetFileTime(hf, nullptr, nullptr, &ft);
		::CloseHandle(hf);
		FILETIME lft = FILETIME();
		::FileTimeToLocalFileTime(&ft, &lft);
		SYSTEMTIME st = SYSTEMTIME();
		if (::FileTimeToSystemTime(&lft, &st) == 0)
			st = SYSTEMTIME();
		::GetTimeFormatW(LOCALE_USER_DEFAULT,
				 0, &st,
				 nullptr, temp, TEMP_LEN);
		ps.Set("FileTime", GUI::UTF8FromString(temp));

		::GetDateFormatW(LOCALE_USER_DEFAULT,
				 DATE_SHORTDATE, &st,
				 nullptr, temp, TEMP_LEN);
		ps.Set("FileDate", GUI::UTF8FromString(temp));

		const DWORD attr = ::GetFileAttributesW(filePath.AsInternal());
		std::string fa;
		if (attr & FILE_ATTRIBUTE_READONLY) {
			fa += "R";
		}
		if (attr & FILE_ATTRIBUTE_HIDDEN) {
			fa += "H";
		}
		if (attr & FILE_ATTRIBUTE_SYSTEM) {
			fa += "S";
		}
		ps.Set("FileAttr", fa);
	} else {
		/* Reset values for new buffers with no file */
		ps.Set("FileTime", "");
		ps.Set("FileDate", "");
		ps.Set("FileAttr", "");
	}

	::GetDateFormatW(LOCALE_USER_DEFAULT,
			 DATE_SHORTDATE, nullptr,     	// Current date
			 nullptr, temp, TEMP_LEN);
	ps.Set("CurrentDate", GUI::UTF8FromString(temp));

	::GetTimeFormatW(LOCALE_USER_DEFAULT,
			 0, nullptr,     	// Current time
			 nullptr, temp, TEMP_LEN);
	ps.Set("CurrentTime", GUI::UTF8FromString(temp));
}

/**
 * Update the status bar text.
 */
void SciTEWin::SetStatusBarText(const char *s) {
	GUI::gui_string barText = GUI::StringFromUTF8(s);
	::SendMessage(HwndOf(wStatusBar),
		      SB_SETTEXT, 0, reinterpret_cast<LPARAM>(barText.c_str()));
}

void SciTEWin::UpdateTabs(const std::vector<GUI::gui_string> &tabNames) {
	// Synchronize the tab control titles with those passed in.

	// Find the first element that differs between the two vectors.
	const auto [misNames, misNamesCurrent] = std::ranges::mismatch(
		tabNames,
		tabNamesCurrent);
	size_t tabChange = std::distance(tabNames.begin(), misNames);

	if (tabNames.size() == tabNamesCurrent.size() && tabNames.size() == tabChange) {
		// Most updates change nothing on the tabs so return early.
		return;
	}

	// Avoiding drawing with WM_SETREDRAW here does not improve speed or flashing.

	size_t tabDeleted = 0;
	while (tabNames.size() < tabNamesCurrent.size()) {
		// Remove extra tabs
		TabCtrl_DeleteItem(HwndOf(wTabBar), tabChange);
		tabNamesCurrent.erase(tabNamesCurrent.begin() + tabChange);
		tabDeleted++;
	}

	// Dirty fix for bug #2347
	if (tabDeleted > 0 && tabChange > 0 && tabChange == tabNames.size()) {
		// Already deleted last tab, try to delete and insert the current last tab
		TabCtrl_DeleteItem(HwndOf(wTabBar), tabChange - 1);

		GUI::gui_string tabNameNext = tabNames.at(tabChange - 1);
		TCITEMW tie {};
		tie.mask = TCIF_TEXT | TCIF_IMAGE;
		tie.iImage = -1;
		tie.pszText = tabNameNext.data();
		TabCtrl_InsertItem(HwndOf(wTabBar), tabChange - 1, &tie);
	}

	while (tabNames.size() > tabNamesCurrent.size()) {
		// Add new tabs
		GUI::gui_string tabNameNext = tabNames.at(tabChange);
		TCITEMW tie {};
		tie.mask = TCIF_TEXT | TCIF_IMAGE;
		tie.iImage = -1;
		tie.pszText = tabNameNext.data();
		TabCtrl_InsertItem(HwndOf(wTabBar), tabChange, &tie);
		tabNamesCurrent.insert(tabNamesCurrent.begin() + tabChange, tabNameNext);
		tabChange++;
	}
	assert(tabNames.size() == tabNamesCurrent.size());

	while (tabChange < tabNames.size()) {
		// Update tabs that are different
		if (tabNames.at(tabChange) != tabNamesCurrent.at(tabChange)) {
			GUI::gui_string tabNameCopy(tabNames.at(tabChange));
			TCITEMW tie {};
			tie.mask = TCIF_TEXT | TCIF_IMAGE;
			tie.iImage = -1;
			tie.pszText = tabNameCopy.data();
			TabCtrl_SetItem(HwndOf(wTabBar), tabChange, &tie);
			tabNamesCurrent.at(tabChange) = tabNameCopy;
		}
		tabChange++;
	}
	assert(tabNamesCurrent == tabNames);
}

void SciTEWin::TabInsert(int index, const GUI::gui_char *title) {
	// This is no longer called as UpdateTabs performs all changes to tabs
	TCITEMW tie {};
	tie.mask = TCIF_TEXT | TCIF_IMAGE;
	tie.iImage = -1;
	GUI::gui_string titleCopy(title);
	tie.pszText = titleCopy.data();
	TabCtrl_InsertItem(HwndOf(wTabBar), index, &tie);
}

void SciTEWin::TabSelect(int index) {
	if (index != TabCtrl_GetCurSel(HwndOf(wTabBar))) {
		TabCtrl_SetCurSel(HwndOf(wTabBar), index);
	}
}

void SciTEWin::RemoveAllTabs() {
	// This is no longer called as UpdateTabs performs all changes to tabs
	TabCtrl_DeleteAllItems(HwndOf(wTabBar));
}

GUI::Point PointOfCursor() noexcept {
	POINT ptCursor;
	::GetCursorPos(&ptCursor);
	return GUI::Point(ptCursor.x, ptCursor.y);
}

GUI::Point ClientFromScreen(HWND hWnd, GUI::Point ptScreen) noexcept {
	POINT ptClient = { ptScreen.x, ptScreen.y };
	::ScreenToClient(hWnd, &ptClient);
	return GUI::Point(ptClient.x, ptClient.y);
}

namespace {

int TabAtPoint(HWND hWnd, GUI::Point pt) noexcept {
	TCHITTESTINFO thti {};
	thti.pt.x = pt.x;
	thti.pt.y = pt.y;
	thti.flags = 0;
	return TabCtrl_HitTest(hWnd, &thti);
}

}

/**
 * Manage Windows specific notifications.
 */
void SciTEWin::Notify(SCNotification *notification) {
	switch (notification->nmhdr.code) {
	case TCN_SELCHANGE:
		// Change of tab
		if (notification->nmhdr.idFrom == IDM_TABWIN) {
			const BufferIndex index = TabCtrl_GetCurSel(HwndOf(wTabBar));
			SetDocumentAt(index);
			CheckReload();
		}
		break;

	case NM_RCLICK:
		// Right click on a control
		if (notification->nmhdr.idFrom == IDM_TABWIN) {

			const GUI::Point ptCursor = PointOfCursor();
			const GUI::Point ptClient = ClientFromScreen(HwndOf(wTabBar), ptCursor);
			const int tabbarHitLast = TabAtPoint(HwndOf(wTabBar), ptClient);

			if (buffers.Current() != tabbarHitLast) {
				SetDocumentAt(tabbarHitLast);
				CheckReload();
			}

			// Pop up menu here:
			popup.CreatePopUp();
			AddToPopUp("Close", IDM_CLOSE, true);
			AddToPopUp("");
			AddToPopUp("Save", IDM_SAVE, true);
			AddToPopUp("Save As", IDM_SAVEAS, true);
			AddToPopUp("");

			bool bAddSeparator = false;
			for (int item = 0; item < toolMax; item++) {
				const int itemID = IDM_TOOLS + item;
				std::string prefix = "command.name.";
				prefix += StdStringFromInteger(item);
				prefix += ".";
				std::string commandName = props.GetNewExpandString(prefix, filePath.AsUTF8());
				if (commandName.length()) {
					AddToPopUp(commandName.c_str(), itemID, true);
					bAddSeparator = true;
				}
			}

			if (bAddSeparator)
				AddToPopUp("");

			AddToPopUp("Print", IDM_PRINT, true);
			popup.Show(ptCursor, wSciTE);
		}
		break;

	case NM_CLICK:
		// Click on a control
		if (notification->nmhdr.idFrom == IDM_STATUSWIN) {
			// Click on the status bar
			const NMMOUSE *pNMMouse = reinterpret_cast<NMMOUSE *>(notification);
			switch (pNMMouse->dwItemSpec) {
			case 0: 		/* Display of status */
				sbNum++;
				if (sbNum > props.GetInt("statusbar.number")) {
					sbNum = 1;
				}
				UpdateStatusBar(true);
				break;
			default:
				break;
			}
		}
		break;

	case TTN_GETDISPINFO:
		// Ask for tooltip text
		{
			const GUI::gui_char *ttext = nullptr;
			NMTTDISPINFOW *pDispInfo = reinterpret_cast<NMTTDISPINFOW *>(notification);
			// Toolbar tooltips
			switch (notification->nmhdr.idFrom) {
			case IDM_NEW:
				ttext = GUI_TEXT("New");
				break;
			case IDM_OPEN:
				ttext = GUI_TEXT("Open");
				break;
			case IDM_SAVE:
				ttext = GUI_TEXT("Save");
				break;
			case IDM_CLOSE:
				ttext = GUI_TEXT("Close");
				break;
			case IDM_PRINT:
				ttext = GUI_TEXT("Print");
				break;
			case IDM_CUT:
				ttext = GUI_TEXT("Cut");
				break;
			case IDM_COPY:
				ttext = GUI_TEXT("Copy");
				break;
			case IDM_PASTE:
				ttext = GUI_TEXT("Paste");
				break;
			case IDM_CLEAR:
				ttext = GUI_TEXT("Delete");
				break;
			case IDM_UNDO:
				ttext = GUI_TEXT("Undo");
				break;
			case IDM_REDO:
				ttext = GUI_TEXT("Redo");
				break;
			case IDM_FIND:
				ttext = GUI_TEXT("Find");
				break;
			case IDM_REPLACE:
				ttext = GUI_TEXT("Replace");
				break;
			case IDM_MACRORECORD:
				ttext = GUI_TEXT("Record Macro");
				break;
			case IDM_MACROSTOPRECORD:
				ttext = GUI_TEXT("Stop Recording");
				break;
			case IDM_MACROPLAY:
				ttext = GUI_TEXT("Run Macro");
				break;
			default: {
					// notification->nmhdr.idFrom appears to be the buffer number for tabbar tooltips
					const GUI::Point ptClient = ClientFromScreen(HwndOf(wTabBar), PointOfCursor());
					const int index = TabAtPoint(HwndOf(wTabBar), ptClient);
					if (index >= 0) {
						GUI::gui_string path = buffers.buffers[index].file.AsInternal();
						// Handle '&' characters in path, since they are interpreted in
						// tooltips.
						size_t amp = 0;
						while ((amp = path.find(GUI_TEXT("&"), amp)) != GUI::gui_string::npos) {
							path.insert(amp, GUI_TEXT("&"));
							amp += 2;
						}
						StringCopy(tooltipText, path.c_str());
						pDispInfo->lpszText = tooltipText;
					}
				}
				break;
			}
			if (ttext) {
				GUI::gui_string localised = localiser.Text(GUI::UTF8FromString(ttext));
				StringCopy(tooltipText, localised.c_str());
				pDispInfo->lpszText = tooltipText;
			}
			break;
		}

	case SCN_CHARADDED:
		if ((notification->nmhdr.idFrom == IDM_RUNWIN) &&
				jobQueue.IsExecuting() &&
				hWriteSubProcess) {
			const char chToWrite = static_cast<char>(notification->ch);
			if (chToWrite != '\r') {
				DWORD bytesWrote = 0;
				::WriteFile(hWriteSubProcess, &chToWrite,
					    1, &bytesWrote, nullptr);
			}
		} else {
			SciTEBase::Notify(notification);
		}
		break;

	case SCN_FOCUSIN:
		if ((notification->nmhdr.idFrom == IDM_SRCWIN) ||
				(notification->nmhdr.idFrom == IDM_RUNWIN))
			wFocus = static_cast<HWND>(notification->nmhdr.hwndFrom);
		SciTEBase::Notify(notification);
		break;

	default:     	// Scintilla notification, use default treatment
		SciTEBase::Notify(notification);
		break;
	}
}

void SciTEWin::ShowToolBar() {
	SizeSubWindows();
}

void SciTEWin::ShowTabBar() {
	SizeSubWindows();
}

void SciTEWin::ShowStatusBar() {
	SizeSubWindows();
}

void SciTEWin::ActivateWindow(const char *) {
	// This does nothing as, on Windows, you can no longer activate yourself
}

enum { tickerID = 100 };

void SciTEWin::TimerStart(int mask) {
	const int maskNew = timerMask | mask;
	if (timerMask != maskNew) {
		if (timerMask == 0) {
			// Create a 1 second ticker
			::SetTimer(HwndOf(wSciTE), tickerID, 1000, nullptr);
		}
		timerMask = maskNew;
	}
}

void SciTEWin::TimerEnd(int mask) {
	const int maskNew = timerMask & ~mask;
	if (timerMask != maskNew) {
		if (maskNew == 0) {
			::KillTimer(HwndOf(wSciTE), tickerID);
		}
		timerMask = maskNew;
	}
}

void SciTEWin::ShowOutputOnMainThread() {
	::PostMessage(MainHWND(), SCITE_SHOWOUTPUT, 0, 0);
}

/**
 * Resize the content windows, embedding the editor and output windows.
 */
void SciTEWin::SizeContentWindows() {
	const GUI::Rectangle rcInternal = wContent.GetClientPosition();

	const int w = rcInternal.Width();
	const int h = rcInternal.Height();
	heightOutput = NormaliseSplit(heightOutput);

	if (splitVertical) {
		wEditor.SetPosition(GUI::Rectangle(0, 0, w - heightOutput - heightBar, h));
		wOutput.SetPosition(GUI::Rectangle(w - heightOutput, 0, w, h));
	} else {
		wEditor.SetPosition(GUI::Rectangle(0, 0, w, h - heightOutput - heightBar));
		wOutput.SetPosition(GUI::Rectangle(0, h - heightOutput, w, h));
	}
	wContent.InvalidateAll();
}

/**
 * Resize the sub-windows, ie. the toolbar, tab bar, status bar. And call @a SizeContentWindows.
 */
void SciTEWin::SizeSubWindows() {
	const GUI::Rectangle rcClient = wSciTE.GetClientPosition();
	bool showTab = false;

	visHeightTools = tbVisible ? (tbLarge ? heightToolsBig : heightTools) : 0;
	bands[bandTool].visible = tbVisible;

	if (tabVisible) {	// ? hide one tab only
		showTab = tabHideOne ?
			  TabCtrl_GetItemCount(HwndOf(wTabBar)) > 1 :
			  true;
	}

	bands[bandTab].visible = showTab;
	if (showTab && tabMultiLine) {
		wTabBar.SetPosition(GUI::Rectangle(
					    rcClient.left, rcClient.top + visHeightTools,
					    rcClient.right, rcClient.top + heightTab + visHeightTools));
	}

	const RECT r = { rcClient.left, 0, rcClient.right, 0 };
	TabCtrl_AdjustRect(HwndOf(wTabBar), TRUE, &r);
	bands[bandTab].height = r.bottom - r.top - 4;

	bands[bandBackground].visible = backgroundStrip.visible;
	bands[bandUser].height = userStrip.Height();
	bands[bandUser].visible = userStrip.visible;
	bands[bandSearch].visible = searchStrip.visible;
	bands[bandFind].visible = findStrip.visible;
	bands[bandReplace].visible = replaceStrip.visible;
	bands[bandFilter].visible = filterStrip.visible;

	const GUI::Rectangle rcSB = wStatusBar.GetPosition();
	bands[bandStatus].height = rcSB.Height() - 2;	// -2 hides a top border
	bands[bandStatus].visible = sbVisible;

	int heightContent = rcClient.Height();
	if (heightContent <= 0)
		heightContent = 1;

	for (const Band &band : bands) {
		if (band.visible && !band.expands)
			heightContent -= band.height;
	}
	if (heightContent <= 0) {
		heightContent = rcClient.Height();
		for (size_t i=0; i<bands.size(); i++) {
			if (i != bandContents)
				bands[i].visible = false;
		}
	}
	bands[bandContents].height = heightContent;

	// May need to copy some values out to other variables

	HDWP hdwp = BeginDeferWindowPos(10);

	int yPos = rcClient.top;
	for (const Band &band : bands) {
		if (band.visible) {
			const GUI::Rectangle rcToSet(rcClient.left, yPos, rcClient.right, yPos + band.height);
			if (hdwp)
				hdwp = ::DeferWindowPos(hdwp, HwndOf(band.win),
							{}, rcToSet.left, rcToSet.top, rcToSet.Width(), rcToSet.Height(),
							SWP_NOZORDER|SWP_NOACTIVATE|SWP_SHOWWINDOW);
			yPos += band.height;
		} else {
			const GUI::Rectangle rcToSet(rcClient.left, rcClient.top - 41, rcClient.Width(), rcClient.top - 40);
			if (hdwp)
				hdwp = ::DeferWindowPos(hdwp, HwndOf(band.win),
							{}, rcToSet.left, rcToSet.top, rcToSet.Width(), rcToSet.Height(),
							SWP_NOZORDER|SWP_NOACTIVATE|SWP_HIDEWINDOW);
		}
	}
	if (hdwp)
		::EndDeferWindowPos(hdwp);

	visHeightTools = bands[bandTool].height;
	visHeightTab = bands[bandTab].height;
	visHeightEditor = bands[bandContents].height;
	visHeightStatus = bands[bandStatus].height;

	SizeContentWindows();
}



// Keymod param is interpreted using the same notation (and much the same
// code) as KeyMatch uses in SciTEWin.cxx.



void SciTEWin::SetMenuItem(int menuNumber, int position, int itemID,
			   const GUI::gui_char *text, const GUI::gui_char *mnemonic) {
	// On Windows the menu items are modified if they already exist or are created
	HMENU hmenu = ::GetSubMenu(::GetMenu(MainHWND()), menuNumber);
	GUI::gui_string sTextMnemonic = text;
	long keycode = 0;
	if (mnemonic && *mnemonic) {
		keycode = SciTEKeys::ParseKeyCode(GUI::UTF8FromString(mnemonic));
		if (keycode) {
			sTextMnemonic += GUI_TEXT("\t");
			sTextMnemonic += mnemonic;
		}
		// the keycode could be used to make a custom accelerator table
		// but for now, the menu's item data is used instead for command
		// tools, and for other menu entries it is just discarded.
	}

	const UINT typeFlags = (text[0]) ? MF_STRING : MF_SEPARATOR;
	if (::GetMenuState(hmenu, itemID, MF_BYCOMMAND) == static_cast<UINT>(-1)) {
		// Not present so insert
		::InsertMenuW(hmenu, position, MF_BYPOSITION | typeFlags, itemID, sTextMnemonic.c_str());
	} else {
		::ModifyMenuW(hmenu, itemID, MF_BYCOMMAND | typeFlags, itemID, sTextMnemonic.c_str());
	}

	if (itemID >= IDM_TOOLS && itemID < IDM_TOOLS + toolMax) {
		// Stow the keycode for later retrieval.
		// Do this even if 0, in case the menu already existed (e.g. ModifyMenu)
		MENUITEMINFO mii {};
		mii.cbSize = sizeof(MENUITEMINFO);
		mii.fMask = MIIM_DATA;
		mii.dwItemData = keycode;
		::SetMenuItemInfo(hmenu, itemID, FALSE, &mii);
	}
}

void SciTEWin::RedrawMenu() {
	// Make previous change visible.
	::DrawMenuBar(HwndOf(wSciTE));
}

void SciTEWin::DestroyMenuItem(int menuNumber, int itemID) {
	// On Windows menu items are destroyed as they can not be hidden and they can be recreated in any position
	HMENU hmenuBar = ::GetMenu(MainHWND());
	if (itemID) {
		HMENU hmenu = ::GetSubMenu(hmenuBar, menuNumber);
		::DeleteMenu(hmenu, itemID, MF_BYCOMMAND);
	} else {
		::DeleteMenu(hmenuBar, menuNumber, MF_BYPOSITION);
	}
}

void SciTEWin::CheckAMenuItem(int wIDCheckItem, bool val) {
	if (val)
		CheckMenuItem(::GetMenu(MainHWND()), wIDCheckItem, MF_CHECKED | MF_BYCOMMAND);
	else
		CheckMenuItem(::GetMenu(MainHWND()), wIDCheckItem, MF_UNCHECKED | MF_BYCOMMAND);
}

void EnableButton(HWND wTools, int id, bool enable) noexcept {
	::SendMessage(wTools, TB_ENABLEBUTTON, id, IntFromTwoShorts(enable, 0));
}

void SciTEWin::EnableAMenuItem(int wIDCheckItem, bool val) {
	if (val)
		::EnableMenuItem(::GetMenu(MainHWND()), wIDCheckItem, MF_ENABLED | MF_BYCOMMAND);
	else
		::EnableMenuItem(::GetMenu(MainHWND()), wIDCheckItem, MF_DISABLED | MF_GRAYED | MF_BYCOMMAND);
	::EnableButton(HwndOf(wToolBar), wIDCheckItem, val);
}

void SciTEWin::CheckMenus() {
	if (!MainHWND())
		return;
	SciTEBase::CheckMenus();
	::CheckMenuRadioItem(::GetMenu(MainHWND()), IDM_EOL_CRLF, IDM_EOL_LF,
			     static_cast<int>(wEditor.EOLMode()) - static_cast<int>(SA::EndOfLine::CrLf) + IDM_EOL_CRLF, 0);
	::CheckMenuRadioItem(::GetMenu(MainHWND()), IDM_ENCODING_DEFAULT, IDM_ENCODING_UCOOKIE,
			     static_cast<int>(CurrentBuffer()->unicodeMode) + IDM_ENCODING_DEFAULT, 0);
}

void SciTEWin::LocaliseMenu(HMENU hmenu) {
	for (int i = 0; i <= ::GetMenuItemCount(hmenu); i++) {
		MENUITEMINFOW mii {};
		mii.cbSize = sizeof(mii);
		mii.fMask = MIIM_CHECKMARKS | MIIM_DATA | MIIM_ID |
			    MIIM_STATE | MIIM_SUBMENU | MIIM_TYPE;
		mii.dwTypeData = nullptr;
		if (!::GetMenuItemInfoW(hmenu, i, TRUE, &mii)) {
			continue;
		}
		GUI::gui_string buff(mii.cch, 0);
		mii.dwTypeData = buff.data();
		mii.cch++;
		if (::GetMenuItemInfoW(hmenu, i, TRUE, &mii)) {
			if (mii.hSubMenu) {
				LocaliseMenu(mii.hSubMenu);
			}
			if (mii.fType == MFT_STRING || mii.fType == MFT_RADIOCHECK) {
				if (mii.dwTypeData) {
					GUI::gui_string text(mii.dwTypeData);
					GUI::gui_string accel(mii.dwTypeData);
					const size_t len = text.length();
					const size_t tab = text.find(GUI_TEXT("\t"));
					if (tab != GUI::gui_string::npos) {
						text.erase(tab, len - tab);
						accel.erase(0, tab + 1);
					} else {
						accel = GUI_TEXT("");
					}
					text = localiser.Text(GUI::UTF8FromString(text), true);
					if (text.length()) {
						if (accel != GUI_TEXT("")) {
							text += GUI_TEXT("\t");
							text += accel;
						}
						text.append(1, 0);
						mii.dwTypeData = text.data();
						::SetMenuItemInfoW(hmenu, i, TRUE, &mii);
					}
				}
			}
		}
	}
}

void SciTEWin::LocaliseMenus() {
	LocaliseMenu(::GetMenu(MainHWND()));
	::DrawMenuBar(MainHWND());
}

void SciTEWin::LocaliseControl(HWND w) {
	std::string originalText = GUI::UTF8FromString(TextOfWindow(w));
	GUI::gui_string translatedText = localiser.Text(originalText, false);
	if (translatedText.length())
		::SetWindowTextW(w, translatedText.c_str());
}

void SciTEWin::LocaliseDialog(HWND wDialog) {
	LocaliseControl(wDialog);
	HWND wChild = GetFirstChild(wDialog);
	while (wChild) {
		LocaliseControl(wChild);
		wChild = GetNextSibling(wChild);
	}
}

namespace {

struct BarButton {
	int id;
	int cmd;
};

BarButton bbs[] = {
	{ -1,           0 },
	{ STD_FILENEW,  IDM_NEW },
	{ STD_FILEOPEN, IDM_OPEN },
	{ STD_FILESAVE, IDM_SAVE },
	{ 0,            IDM_CLOSE },
	{ -1,           0 },
	{ STD_PRINT,    IDM_PRINT },
	{ -1,           0 },
	{ STD_CUT,      IDM_CUT },
	{ STD_COPY,     IDM_COPY },
	{ STD_PASTE,    IDM_PASTE },
	{ STD_DELETE,   IDM_CLEAR },
	{ -1,           0 },
	{ STD_UNDO,     IDM_UNDO },
	{ STD_REDOW,    IDM_REDO },
	{ -1,           0 },
	{ STD_FIND,     IDM_FIND },
	{ STD_REPLACE,  IDM_REPLACE },
};

WNDPROC stDefaultTabProc = nullptr;
LRESULT PASCAL TabWndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam) {

	static bool bDragBegin = false;
	static int iDraggingTab = -1;
	static int iLastClickTab = -1;
	static HWND hwndLastFocus {};

	switch (iMessage) {

	case WM_LBUTTONDOWN: {
			const GUI::Point pt = PointFromLong(lParam);
			iLastClickTab = TabAtPoint(hWnd, pt);
		}
		break;
	}

	LRESULT retResult;
	if (stDefaultTabProc != nullptr) {
		retResult = CallWindowProc(stDefaultTabProc, hWnd, iMessage, wParam, lParam);
	} else {
		retResult = ::DefWindowProc(hWnd, iMessage, wParam, lParam);
	}

	switch (iMessage) {

	case WM_MBUTTONDOWN: {
			// Check if on tab bar
			const GUI::Point pt = PointFromLong(lParam);
			const int tab = TabAtPoint(hWnd, pt);
			if (tab >= 0) {
				::SendMessage(::GetParent(hWnd), WM_COMMAND, IDC_TABCLOSE, tab);
			}
		}
		break;

	case WM_LBUTTONUP: {
			iLastClickTab = -1;
			if (bDragBegin) {
				if (hwndLastFocus) ::SetFocus(hwndLastFocus);
				::ReleaseCapture();
				::SetCursor(::LoadCursor({}, IDC_ARROW));
				bDragBegin = false;
				const GUI::Point pt = PointFromLong(lParam);
				const int tab = TabAtPoint(hWnd, pt);
				if (tab > -1 && iDraggingTab > -1 && iDraggingTab != tab) {
					::SendMessage(::GetParent(hWnd),
						      WM_COMMAND,
						      IDC_SHIFTTAB,
						      MAKELPARAM(iDraggingTab, tab));
				}
				iDraggingTab = -1;
			}
		}
		break;

	case WM_KEYDOWN: {
			if (wParam == VK_ESCAPE) {
				if (bDragBegin) {
					if (hwndLastFocus) ::SetFocus(hwndLastFocus);
					::ReleaseCapture();
					::SetCursor(::LoadCursor({}, IDC_ARROW));
					bDragBegin = false;
					iDraggingTab = -1;
					iLastClickTab = -1;
					::InvalidateRect(hWnd, nullptr, FALSE);
				}
			}
		}
		break;

	case WM_MOUSEMOVE: {

			const GUI::Point pt = PointFromLong(lParam);
			const int tab = TabAtPoint(hWnd, pt);
			const int tabcount = TabCtrl_GetItemCount(hWnd);

			if (wParam == MK_LBUTTON &&
					tabcount > 1 &&
					tab > -1 &&
					iLastClickTab == tab &&
					!bDragBegin) {
				iDraggingTab = tab;
				::SetCapture(hWnd);
				hwndLastFocus = ::SetFocus(hWnd);
				bDragBegin = true;
				HCURSOR hcursor = ::LoadCursor(::GetModuleHandle(nullptr),
							       MAKEINTRESOURCE(IDC_DRAGDROP));
				if (hcursor) ::SetCursor(hcursor);
			} else {
				if (bDragBegin) {
					if (tab > -1 && iDraggingTab > -1 /*&& iDraggingTab != tab*/) {
						HCURSOR hcursor = ::LoadCursor(::GetModuleHandle(nullptr),
									       MAKEINTRESOURCE(IDC_DRAGDROP));
						if (hcursor) ::SetCursor(hcursor);
					} else {
						::SetCursor(::LoadCursor({}, IDC_NO));
					}
				}
			}
		}
		break;

	case WM_PAINT: {
			if (bDragBegin && iDraggingTab != -1) {

				const GUI::Point ptClient = ClientFromScreen(hWnd, PointOfCursor());
				const int tab = TabAtPoint(hWnd, ptClient);

				const RECT tabrc {};
				if (tab != -1 &&
						tab != iDraggingTab &&
						TabCtrl_GetItemRect(hWnd, tab, &tabrc)) {

					HDC hDC = ::GetDC(hWnd);
					if (hDC) {

						const int xLeft = tabrc.left + 8;
						const int yLeft = tabrc.top + (tabrc.bottom - tabrc.top) / 2;
						POINT ptsLeftArrow[] = {
							{xLeft, yLeft - 2},
							{xLeft - 2, yLeft - 2},
							{xLeft - 2, yLeft - 5},
							{xLeft - 7, yLeft},
							{xLeft - 2, yLeft + 5},
							{xLeft - 2, yLeft + 2},
							{xLeft, yLeft + 2}
						};

						const int xRight = tabrc.right - 10;
						const int yRight = tabrc.top + (tabrc.bottom - tabrc.top) / 2;
						POINT ptsRightArrow[] = {
							{xRight, yRight - 2},
							{xRight + 2, yRight - 2},
							{xRight + 2, yRight - 5},
							{xRight + 7, yRight},
							{xRight + 2, yRight + 5},
							{xRight + 2, yRight + 2},
							{xRight, yRight + 2}
						};

						HPEN pen = ::CreatePen(0, 1, RGB(255, 0, 0));
						HPEN penOld = SelectPen(hDC, pen);
						const COLORREF colourNearest = ::GetNearestColor(hDC, RGB(255, 0, 0));
						HBRUSH brush = ::CreateSolidBrush(colourNearest);
						HBRUSH brushOld = SelectBrush(hDC, brush);
						::Polygon(hDC, tab < iDraggingTab ? ptsLeftArrow : ptsRightArrow, 7);
						SelectBrush(hDC, brushOld);
						DeleteBrush(brush);
						SelectPen(hDC, penOld);
						DeletePen(pen);
						::ReleaseDC(hWnd, hDC);
					}
				}
			}
		}
		break;
	}

	return retResult;
}

}

void SciTEWin::CreateStrip(LPCWSTR stripName, LPVOID lpParam) {
	const HWND hwnd = ::CreateWindowExW(
		0,
		classNameInternal,
		stripName,
		WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
		0, 0,
		100, 100,
		MainHWND(),
		HmenuID(2001),
		hInstance,
		lpParam);
	if (!hwnd)
		exit(FALSE);
}

/**
 * Create all the needed windows.
 */
void SciTEWin::Creation() {

	constexpr int widthWindow = 100;	// Temporary until stretched to fit

	wContent = ::CreateWindowExW(
			   flatterUI ? 0 : WS_EX_CLIENTEDGE,
			   classNameInternal,
			   TEXT("Source"),
			   WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
			   0, 0,
			   widthWindow, 100,
			   MainHWND(),
			   HmenuID(2000),
			   hInstance,
			   &contents);
	wContent.Show();

	wEditor.SetScintilla(::CreateWindowExW(
				     0,
				     TEXT("Scintilla"),
				     TEXT("Source"),
				     WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
				     0, 0,
				     widthWindow, 100,
				     HwndOf(wContent),
				     HmenuID(IDM_SRCWIN),
				     hInstance,
				     nullptr));
	if (!wEditor.CanCall())
		exit(FALSE);
	wEditor.Show();
	wEditor.UsePopUp(SA::PopUp::Never);
	wEditor.SetCommandEvents(false);
	WindowSetFocus(wEditor);

	wOutput.SetScintilla(::CreateWindowExW(
				     0,
				     TEXT("Scintilla"),
				     TEXT("Run"),
				     WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
				     0, 0,
				     widthWindow, 100,
				     HwndOf(wContent),
				     HmenuID(IDM_RUNWIN),
				     hInstance,
				     nullptr));
	if (!wOutput.CanCall())
		exit(FALSE);
	wOutput.Show();
	wOutput.SetCommandEvents(false);
	// No selection margin on output window
	wOutput.SetMarginWidthN(1, 0);
	//wOutput.SetCaretPeriod(0);
	wOutput.UsePopUp(SA::PopUp::Never);
	::DragAcceptFiles(MainHWND(), true);

	HWND hwndToolBar = ::CreateWindowExW(
				   0,
				   TOOLBARCLASSNAME,
				   TEXT(""),
				   WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
				   TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | CCS_NORESIZE,
				   0, 0,
				   widthWindow, tbLarge ? heightToolsBig : heightTools,
				   MainHWND(),
				   HmenuID(IDM_TOOLWIN),
				   hInstance,
				   nullptr);
	wToolBar = hwndToolBar;

	::SendMessage(hwndToolBar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
	::SendMessage(hwndToolBar, TB_SETBITMAPSIZE, 0, tbLarge ? MAKELPARAM(24, 24) : MAKELPARAM(16, 16));
	::SendMessage(hwndToolBar, TB_LOADIMAGES,
		      tbLarge ? IDB_STD_LARGE_COLOR : IDB_STD_SMALL_COLOR,
		      reinterpret_cast<LPARAM>(HINST_COMMCTRL));

	TBADDBITMAP addbmp = { hInstance, IDR_CLOSEFILE };

	if (tbLarge) {
		addbmp.nID = IDR_CLOSEFILE24;
	}

	::SendMessage(hwndToolBar, TB_ADDBITMAP, 1, reinterpret_cast<LPARAM>(&addbmp));

	TBBUTTON tbb[std::size(bbs)] = {};
	for (unsigned int i = 0; i < std::size(bbs); i++) {
		if (bbs[i].cmd == IDM_CLOSE)
			tbb[i].iBitmap = STD_PRINT + 1;
		else
			tbb[i].iBitmap = bbs[i].id;
		tbb[i].idCommand = bbs[i].cmd;
		tbb[i].fsState = TBSTATE_ENABLED;
		if (-1 == bbs[i].id)
			tbb[i].fsStyle = TBSTYLE_SEP;
		else
			tbb[i].fsStyle = TBSTYLE_BUTTON;
		tbb[i].dwData = 0;
		tbb[i].iString = 0;
	}

	::SendMessage(hwndToolBar, TB_ADDBUTTONS, std::size(bbs), reinterpret_cast<LPARAM>(tbb));

	wToolBar.Show();

	INITCOMMONCONTROLSEX icce {};
	icce.dwSize = sizeof(icce);
	icce.dwICC = ICC_TAB_CLASSES;
	InitCommonControlsEx(&icce);

	WNDCLASS wndClass = {};
	if (::GetClassInfo({}, WC_TABCONTROL, &wndClass) == 0)
		exit(FALSE);
	stDefaultTabProc = wndClass.lpfnWndProc;
	wndClass.lpfnWndProc = TabWndProc;
	wndClass.style = wndClass.style | CS_DBLCLKS;
	wndClass.lpszClassName = TEXT("SciTeTabCtrl");
	wndClass.hInstance = hInstance;
	if (RegisterClass(&wndClass) == 0)
		exit(FALSE);

	wTabBar = ::CreateWindowExW(
			  0,
			  TEXT("SciTeTabCtrl"),
			  TEXT("Tab"),
			  WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
			  TCS_FOCUSNEVER | TCS_TOOLTIPS,
			  0, 0,
			  widthWindow, heightTab,
			  MainHWND(),
			  HmenuID(IDM_TABWIN),
			  hInstance,
			  nullptr);

	if (!wTabBar.Created())
		exit(FALSE);

	LOGFONTW lfIconTitle {};
	if (::SystemParametersInfoW(SPI_GETICONTITLELOGFONT, sizeof(lfIconTitle), &lfIconTitle, FALSE) == 0)
		exit(FALSE);
	fontTabs = ::CreateFontIndirectW(&lfIconTitle);
	SetWindowFont(HwndOf(wTabBar), fontTabs, 0);

	wTabBar.Show();

	CreateStrip(L"BackgroundStrip", &backgroundStrip);
	CreateStrip(L"UserStrip", &userStrip);
	CreateStrip(L"SearchStrip", &searchStrip);
	CreateStrip(L"FindStrip", &findStrip);
	CreateStrip(L"ReplaceStrip", &replaceStrip);
	CreateStrip(L"FilterStrip", &filterStrip);

	wStatusBar = ::CreateWindowExW(
			     0,
			     STATUSCLASSNAME,
			     TEXT(""),
			     WS_CHILD | WS_CLIPSIBLINGS,
			     0, 0,
			     widthWindow, heightStatus,
			     MainHWND(),
			     HmenuID(IDM_STATUSWIN),
			     hInstance,
			     nullptr);
	wStatusBar.Show();
	const int widths[] = { 4000 };
	// Perhaps we can define a syntax to create more parts,
	// but it is probably an overkill for a marginal feature
	::SendMessage(HwndOf(wStatusBar),
		      SB_SETPARTS, 1,
		      reinterpret_cast<LPARAM>(widths));

	bands.emplace_back(true, tbLarge ? heightToolsBig : heightTools, false, wToolBar);
	bands.emplace_back(true, heightTab, false, wTabBar);
	bands.emplace_back(true, 100, true, wContent);
	bands.emplace_back(true, userStrip.Height(), false, userStrip);
	bands.emplace_back(true, backgroundStrip.Height(), false, backgroundStrip);
	bands.emplace_back(true, searchStrip.Height(), false, searchStrip);
	bands.emplace_back(true, findStrip.Height(), false, findStrip);
	bands.emplace_back(true, replaceStrip.Height(), false, replaceStrip);
	bands.emplace_back(true, filterStrip.Height(), false, filterStrip);
	bands.emplace_back(true, heightStatus, false, wStatusBar);

#ifndef NO_LUA
	if (props.GetExpandedString("ext.lua.startup.script").length() == 0)
		DestroyMenuItem(menuOptions, IDM_OPENLUAEXTERNALFILE);
#else
	DestroyMenuItem(menuOptions, IDM_OPENLUAEXTERNALFILE);
#endif
}
