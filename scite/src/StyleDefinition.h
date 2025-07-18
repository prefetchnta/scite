// SciTE - Scintilla based Text Editor
/** @file StyleDefinition.h
 ** Definition of style aggregate and helper functions.
 **/
// Copyright 2013 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef STYLEDEFINITION_H
#define STYLEDEFINITION_H

class StyleDefinition {
public:
	std::string font;
	float sizeFractional;
	int size;
	std::string fore;
	std::string back;
	Scintilla::FontWeight weight;
	Scintilla::FontStretch stretch;
	bool italics;
	bool eolfilled;
	bool underlined;
	Scintilla::CaseVisible caseForce;
	bool visible;
	bool changeable;
	std::string invisibleRep;
	enum flags { sdNone = 0, sdFont = 0x1, sdSize = 0x2, sdFore = 0x4, sdBack = 0x8,
		     sdWeight = 0x10, sdItalics = 0x20, sdEOLFilled = 0x40, sdUnderlined = 0x80,
		     sdCaseForce = 0x100, sdVisible = 0x200, sdChangeable = 0x400, sdInvisibleRep=0x800,
		     sdStretch = 0x10000,
		   } specified;
	explicit StyleDefinition(std::string_view definition);
	bool ParseStyleDefinition(std::string_view definition);
	Scintilla::Colour Fore() const noexcept;
	Scintilla::Colour Back() const noexcept;
	int FractionalSize() const noexcept;
	bool IsBold() const noexcept;
};

constexpr Scintilla::Colour ColourRGB(unsigned int red, unsigned int green, unsigned int blue) noexcept {
	return red | (green << 8) | (blue << 16);
}

constexpr Scintilla::ColourAlpha ColourRGBA(unsigned int red, unsigned int green, unsigned int blue, unsigned int alpha=0xff) noexcept {
	return red | (green << 8) | (blue << 16) | (alpha << 24);
}

int IntFromHexByte(std::string_view hexByte) noexcept;

Scintilla::Colour ColourFromString(std::string_view s) noexcept;
Scintilla::ColourAlpha ColourAlphaFromString(std::string_view s) noexcept;

struct IndicatorDefinition {
	Scintilla::IndicatorStyle style;
	Scintilla::Colour colour;
	Scintilla::Alpha fillAlpha;
	Scintilla::Alpha outlineAlpha;
	bool under;
	explicit IndicatorDefinition(std::string_view definition);
	bool ParseIndicatorDefinition(std::string_view definition);
};

struct MarkerDefinition {
	Scintilla::MarkerSymbol style;
	Scintilla::Colour colour;
	Scintilla::Colour back = ColourRGB(0xFF,0xFF,0xFF);
	explicit MarkerDefinition(std::string_view definition);
	bool ParseMarkerDefinition(std::string_view definition);
};

#endif
