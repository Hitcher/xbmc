/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

// Tests for CGUITextLayout::ParseText label-formatting tag parsing.
//
// These tests cover the logic introduced and fixed by:
//   PR #28280  – replaced XOR-toggle logic with per-style reference counters
//               (boldDepth, italicDepth, etc.) for [B], [I], [UPPERCASE],
//               [LOWERCASE], [CAPITALIZE], [LIGHT] tags.
//   PR #28356  – added lookahead guard: an opening tag's depth counter is only
//               incremented when a matching closing tag exists later in the
//               string; unmatched closing tags at depth 0 are silently ignored.
//
// CGUITextLayout::Filter(std::string&) is used as the test entry point.
// It is public and static, calls ParseText internally, and strips all style
// bits to return the plain UTF-8 text — exactly what we need to observe
// whether tags are being recognised and consumed correctly.
//
// NOTE: [COLOR] tags call CServiceBroker::GetGUI()->GetColorManager() and
// therefore cannot be tested in this unit-test harness without a full service
// broker setup.  [COLOR] tag handling is covered by integration tests.

#include "guilib/GUITextLayout.h"

#include <string>

#include <gtest/gtest.h>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace
{

// Run Filter on a UTF-8 string and return the resulting plain text.
// Filter calls ParseText internally and strips all style/colour bits,
// leaving only the plain character content.
std::string GetPlainText(const std::string& rawLabel)
{
  std::string text = rawLabel;
  CGUITextLayout::Filter(text);
  return text;
}

// Returns true when rawLabel filters to exactly expectedPlain.
bool FiltersTo(const std::string& rawLabel, const std::string& expectedPlain)
{
  return GetPlainText(rawLabel) == expectedPlain;
}

// Returns true when the filtered plain text does NOT contain the given substring.
bool PlainTextLacks(const std::string& rawLabel, const std::string& fragment)
{
  return GetPlainText(rawLabel).find(fragment) == std::string::npos;
}

// Returns true when the filtered plain text DOES contain the given substring.
bool PlainTextHas(const std::string& rawLabel, const std::string& fragment)
{
  return GetPlainText(rawLabel).find(fragment) != std::string::npos;
}

} // anonymous namespace

// ===========================================================================
// 1. Baseline: plain text with no tags
// ===========================================================================

TEST(TestGUITextLayoutParseText, EmptyString)
{
  EXPECT_TRUE(FiltersTo("", ""));
}

TEST(TestGUITextLayoutParseText, PlainTextPassesThrough)
{
  EXPECT_TRUE(FiltersTo("Hello world", "Hello world"));
}

TEST(TestGUITextLayoutParseText, PlainTextWithNumbers)
{
  EXPECT_TRUE(FiltersTo("Track 07 of 12", "Track 07 of 12"));
}

TEST(TestGUITextLayoutParseText, SquareBracketsInContent)
{
  // Inline brackets that don't form a recognised tag pass through unchanged
  EXPECT_TRUE(FiltersTo("price [3.99] per item", "price [3.99] per item"));
}

// ===========================================================================
// 2. Single matched tag pairs – tags stripped, content preserved
// ===========================================================================

TEST(TestGUITextLayoutParseText, MatchedBoldTagStripped)
{
  EXPECT_TRUE(FiltersTo("[B]bold[/B]", "bold"));
}

TEST(TestGUITextLayoutParseText, MatchedItalicTagStripped)
{
  EXPECT_TRUE(FiltersTo("[I]italic[/I]", "italic"));
}

TEST(TestGUITextLayoutParseText, MatchedUppercaseTagStripped)
{
  // [UPPERCASE] transforms content to upper and strips the tag
  EXPECT_TRUE(FiltersTo("[UPPERCASE]text[/UPPERCASE]", "TEXT"));
}

TEST(TestGUITextLayoutParseText, MatchedLowercaseTagStripped)
{
  // [LOWERCASE] transforms content to lower and strips the tag
  EXPECT_TRUE(FiltersTo("[LOWERCASE]TEXT[/LOWERCASE]", "text"));
}

TEST(TestGUITextLayoutParseText, MatchedCapitalizeTagStripped)
{
  // [CAPITALIZE] capitalizes words and strips the tag
  EXPECT_TRUE(FiltersTo("[CAPITALIZE]hello world[/CAPITALIZE]", "Hello World"));
}

TEST(TestGUITextLayoutParseText, MatchedLightTagStripped)
{
  EXPECT_TRUE(FiltersTo("[LIGHT]text[/LIGHT]", "text"));
}

TEST(TestGUITextLayoutParseText, MatchedBoldWithSurroundingText)
{
  EXPECT_TRUE(FiltersTo("prefix [B]bold[/B] suffix", "prefix bold suffix"));
}

// ===========================================================================
// 3. Nested matched tag pairs (core fix in PR #28280)
//    Depth counter must reach N then drain back to 0 correctly.
//    Plain text content must be fully preserved.
// ===========================================================================

TEST(TestGUITextLayoutParseText, DoublyNestedBoldTagsStripped)
{
  // depth: 0 -> 1 -> 2 -> 1 -> 0
  EXPECT_TRUE(FiltersTo("[B][B]doubly bold[/B][/B]", "doubly bold"));
}

TEST(TestGUITextLayoutParseText, TriplyNestedBoldTagsStripped)
{
  EXPECT_TRUE(FiltersTo("[B][B][B]triple[/B][/B][/B]", "triple"));
}

TEST(TestGUITextLayoutParseText, DoublyNestedItalicTagsStripped)
{
  EXPECT_TRUE(FiltersTo("[I][I]text[/I][/I]", "text"));
}

TEST(TestGUITextLayoutParseText, DoublyNestedUppercaseTagsStripped)
{
  EXPECT_TRUE(FiltersTo("[UPPERCASE][UPPERCASE]text[/UPPERCASE][/UPPERCASE]", "TEXT"));
}

TEST(TestGUITextLayoutParseText, DoublyNestedLowercaseTagsStripped)
{
  EXPECT_TRUE(FiltersTo("[LOWERCASE][LOWERCASE]TEXT[/LOWERCASE][/LOWERCASE]", "text"));
}

TEST(TestGUITextLayoutParseText, DoublyNestedCapitalizeTagsStripped)
{
  EXPECT_TRUE(FiltersTo("[CAPITALIZE][CAPITALIZE]hello world[/CAPITALIZE][/CAPITALIZE]",
                        "Hello World"));
}

TEST(TestGUITextLayoutParseText, DoublyNestedLightTagsStripped)
{
  EXPECT_TRUE(FiltersTo("[LIGHT][LIGHT]text[/LIGHT][/LIGHT]", "text"));
}

// ===========================================================================
// 4. Unmatched OPENING tags (no closing counterpart in the string)
//    Per #28356 lookahead: the depth counter must NOT be incremented.
//    Tag characters must not appear in the plain-text output.
// ===========================================================================

TEST(TestGUITextLayoutParseText, UnmatchedOpenBold_TagNotInOutput)
{
  EXPECT_TRUE(PlainTextLacks("[B]text with no close", "[B]"));
}

TEST(TestGUITextLayoutParseText, UnmatchedOpenBold_ContentPreserved)
{
  EXPECT_TRUE(PlainTextHas("[B]text with no close", "text with no close"));
}

TEST(TestGUITextLayoutParseText, UnmatchedOpenItalic_TagNotInOutput)
{
  EXPECT_TRUE(PlainTextLacks("[I]text with no close", "[I]"));
}

TEST(TestGUITextLayoutParseText, UnmatchedOpenUppercase_TagNotInOutput)
{
  EXPECT_TRUE(PlainTextLacks("[UPPERCASE]text with no close", "[UPPERCASE]"));
}

TEST(TestGUITextLayoutParseText, UnmatchedOpenLowercase_TagNotInOutput)
{
  EXPECT_TRUE(PlainTextLacks("[LOWERCASE]text with no close", "[LOWERCASE]"));
}

TEST(TestGUITextLayoutParseText, UnmatchedOpenCapitalize_TagNotInOutput)
{
  EXPECT_TRUE(PlainTextLacks("[CAPITALIZE]text with no close", "[CAPITALIZE]"));
}

TEST(TestGUITextLayoutParseText, UnmatchedOpenLight_TagNotInOutput)
{
  EXPECT_TRUE(PlainTextLacks("[LIGHT]text with no close", "[LIGHT]"));
}

TEST(TestGUITextLayoutParseText, UnmatchedOpenBold_AtEndOfString)
{
  EXPECT_TRUE(PlainTextLacks("text[B]", "[B]"));
  EXPECT_TRUE(PlainTextHas("text[B]", "text"));
}

// ===========================================================================
// 5. Unmatched CLOSING tags (no prior open; depth is 0)
//    Per #28356: stray closes are silently ignored; content is preserved.
// ===========================================================================

TEST(TestGUITextLayoutParseText, UnmatchedCloseBold_TagNotInOutput)
{
  EXPECT_TRUE(PlainTextLacks("text [/B] more text", "[/B]"));
}

TEST(TestGUITextLayoutParseText, UnmatchedCloseBold_ContentPreserved)
{
  EXPECT_TRUE(PlainTextHas("text [/B] more text", "text"));
  EXPECT_TRUE(PlainTextHas("text [/B] more text", "more text"));
}

TEST(TestGUITextLayoutParseText, UnmatchedCloseItalic_TagNotInOutput)
{
  EXPECT_TRUE(PlainTextLacks("text [/I] more text", "[/I]"));
}

TEST(TestGUITextLayoutParseText, UnmatchedCloseUppercase_TagNotInOutput)
{
  EXPECT_TRUE(PlainTextLacks("text [/UPPERCASE] more text", "[/UPPERCASE]"));
}

TEST(TestGUITextLayoutParseText, UnmatchedCloseLowercase_TagNotInOutput)
{
  EXPECT_TRUE(PlainTextLacks("text [/LOWERCASE] more text", "[/LOWERCASE]"));
}

TEST(TestGUITextLayoutParseText, UnmatchedCloseCapitalize_TagNotInOutput)
{
  EXPECT_TRUE(PlainTextLacks("text [/CAPITALIZE] more text", "[/CAPITALIZE]"));
}

TEST(TestGUITextLayoutParseText, UnmatchedCloseLight_TagNotInOutput)
{
  EXPECT_TRUE(PlainTextLacks("text [/LIGHT] more text", "[/LIGHT]"));
}

TEST(TestGUITextLayoutParseText, UnmatchedCloseAtStartOfString)
{
  EXPECT_TRUE(PlainTextLacks("[/B]text", "[/B]"));
  EXPECT_TRUE(PlainTextHas("[/B]text", "text"));
}

TEST(TestGUITextLayoutParseText, MultipleStrayClosesBeforeValidPair)
{
  EXPECT_TRUE(FiltersTo("[/B][/B][/B][B]valid[/B]", "valid"));
}

// ===========================================================================
// 6. Extra stray close after a valid matched pair (depth returns to 0)
// ===========================================================================

TEST(TestGUITextLayoutParseText, ExtraCloseAfterMatchedPair)
{
  // [B]text[/B][/B] – second [/B] is stray, must not corrupt output
  EXPECT_TRUE(FiltersTo("[B]text[/B][/B]", "text"));
}

TEST(TestGUITextLayoutParseText, ExtraCloseAfterNestedMatchedPair)
{
  // [B][B]text[/B][/B][/B] – third [/B] is stray
  EXPECT_TRUE(FiltersTo("[B][B]text[/B][/B][/B]", "text"));
}

// ===========================================================================
// 7. Stray close between two valid disjoint pairs
// ===========================================================================

TEST(TestGUITextLayoutParseText, StrayCloseBetweenTwoValidPairs)
{
  // After first pair closes, depth is 0; stray [/B] ignored; second pair valid
  EXPECT_TRUE(FiltersTo("[B]first[/B] [/B] [B]second[/B]", "first  second"));
}

// ===========================================================================
// 8. Multiple disjoint matched pairs
// ===========================================================================

TEST(TestGUITextLayoutParseText, TwoDisjointBoldPairs)
{
  EXPECT_TRUE(FiltersTo("[B]first[/B] mid [B]second[/B]", "first mid second"));
}

TEST(TestGUITextLayoutParseText, BoldAndItalicDisjoint)
{
  EXPECT_TRUE(FiltersTo("[B]bold[/B] then [I]italic[/I]", "bold then italic"));
}

TEST(TestGUITextLayoutParseText, AllSixStyleTagsInSequence)
{
  // Note: [UPPERCASE] transforms 'u' -> 'U', [LOWERCASE] 'L' -> 'l',
  // [CAPITALIZE] capitalizes first letter of each word.
  EXPECT_TRUE(FiltersTo(
      "[B]b[/B][I]i[/I][UPPERCASE]u[/UPPERCASE]"
      "[LOWERCASE]L[/LOWERCASE][CAPITALIZE]c[/CAPITALIZE][LIGHT]lt[/LIGHT]",
      "biUlClt"));
}

// ===========================================================================
// 9. Cross-type nesting
// ===========================================================================

TEST(TestGUITextLayoutParseText, BoldWrappingItalic)
{
  EXPECT_TRUE(FiltersTo("[B][I]bold+italic[/I][/B]", "bold+italic"));
}

TEST(TestGUITextLayoutParseText, UppercaseWrappingLight)
{
  EXPECT_TRUE(FiltersTo("[UPPERCASE][LIGHT]text[/LIGHT][/UPPERCASE]", "TEXT"));
}

TEST(TestGUITextLayoutParseText, AllStylesNested)
{
  // Multiple case-transform tags active simultaneously.
  // ParseText applies UPPERCASE, then LOWERCASE, then CAPITALIZE,
  // resulting in "Content".
  EXPECT_TRUE(FiltersTo(
      "[B][I][UPPERCASE][LOWERCASE][CAPITALIZE][LIGHT]"
      "content"
      "[/LIGHT][/CAPITALIZE][/LOWERCASE][/UPPERCASE][/I][/B]",
      "Content"));
}

TEST(TestGUITextLayoutParseText, InterleavedBoldItalic_NoUnderflow)
{
  // Technically malformed nesting: [B][I]text[/B][/I]
  // Parser must not crash or underflow any depth counter.
  EXPECT_TRUE(PlainTextHas("[B][I]text[/B][/I]", "text"));
}

TEST(TestGUITextLayoutParseText, NestedBold_MissingOuterClose)
{
  // [B]text1[B]text2[/B]
  //
  // The inner pair closes correctly, leaving the outer [B] unmatched.
  // Content must still be preserved and extracted correctly.
  EXPECT_TRUE(FiltersTo("[B]text1[B]text2[/B]", "text1text2"));
}

TEST(TestGUITextLayoutParseText, CrossedBoldItalicNesting)
{
  // [B]A[I]B[/B]C[/I]
  //
  // Technically malformed nesting. The parser tracks bold and italic
  // independently via depth counters and does not enforce stack ordering.
  // Content must still be preserved and extracted correctly.
  EXPECT_TRUE(FiltersTo("[B]A[I]B[/B]C[/I]", "ABC"));
}

// ===========================================================================
// 10. CR tag
// ===========================================================================

TEST(TestGUITextLayoutParseText, CRTagConsumed)
{
  // [CR] is converted to a newline character; the literal "[CR]" must not appear
  EXPECT_TRUE(PlainTextLacks("line one[CR]line two", "[CR]"));
}

TEST(TestGUITextLayoutParseText, CRTagContentPreserved)
{
  EXPECT_TRUE(PlainTextHas("line one[CR]line two", "line one"));
  EXPECT_TRUE(PlainTextHas("line one[CR]line two", "line two"));
}

TEST(TestGUITextLayoutParseText, MultipleCRTags)
{
  EXPECT_TRUE(PlainTextLacks("a[CR]b[CR]c[CR]d", "[CR]"));
}

TEST(TestGUITextLayoutParseText, BoldSpanningCRTag)
{
  EXPECT_TRUE(PlainTextLacks("[B]line one[CR]line two[/B]", "[B]"));
  EXPECT_TRUE(PlainTextLacks("[B]line one[CR]line two[/B]", "[/B]"));
  EXPECT_TRUE(PlainTextLacks("[B]line one[CR]line two[/B]", "[CR]"));
}

// ===========================================================================
// 11. TABS tag
// ===========================================================================

TEST(TestGUITextLayoutParseText, MatchedTabsTagConsumed)
{
  EXPECT_TRUE(PlainTextLacks("[TABS]1[/TABS]label", "[TABS]"));
  EXPECT_TRUE(PlainTextLacks("[TABS]1[/TABS]label", "[/TABS]"));
  EXPECT_TRUE(PlainTextHas("[TABS]1[/TABS]label", "label"));
}

// Unmatched opening [TABS] should be treated as a formatting tag
// and not leak into the displayed text.
TEST(TestGUITextLayoutParseText, UnmatchedOpenTabsTagConsumed)
{
  // Current behaviour leaks the literal tag into the output.
  // Expected behaviour is to consume the tag while emitting no tabs.
  EXPECT_TRUE(PlainTextLacks("[TABS]2 label", "[TABS]"));
}

// ===========================================================================
// 12. Empty tag pairs
// ===========================================================================

TEST(TestGUITextLayoutParseText, EmptyBoldPair)
{
  EXPECT_TRUE(PlainTextLacks("[B][/B]", "[B]"));
  EXPECT_TRUE(PlainTextLacks("[B][/B]", "[/B]"));
}

TEST(TestGUITextLayoutParseText, EmptyNestedBoldPairs)
{
  EXPECT_TRUE(PlainTextLacks("[B][B][/B][/B]", "[B]"));
  EXPECT_TRUE(PlainTextLacks("[B][B][/B][/B]", "[/B]"));
}

// ===========================================================================
// 13. Pathological / edge-case inputs – must not crash
// ===========================================================================

TEST(TestGUITextLayoutParseText, SingleOpenBracket)
{
  EXPECT_NO_FATAL_FAILURE(GetPlainText("["));
}

TEST(TestGUITextLayoutParseText, UnclosedBracketBeforeTag)
{
  EXPECT_TRUE(PlainTextHas("[ [B]text[/B]", "text"));
}

TEST(TestGUITextLayoutParseText, BracketPairWithSpaces_NotATag)
{
  // "[ B ]" has spaces – not a recognised tag, passes through
  EXPECT_TRUE(PlainTextHas("[ B ]text[ /B ]", "text"));
}

TEST(TestGUITextLayoutParseText, LowercaseBoldTag_NotRecognised)
{
  // "[b]" is not a recognised tag (parser is case-sensitive)
  EXPECT_TRUE(PlainTextHas("[b]text[/b]", "text"));
}

TEST(TestGUITextLayoutParseText, VeryLongPlainText)
{
  const std::string text(10000, 'A');
  EXPECT_TRUE(FiltersTo(text, text));
}

TEST(TestGUITextLayoutParseText, VeryLongNestedBold)
{
  const std::string inner(5000, 'X');
  EXPECT_TRUE(FiltersTo("[B]" + inner + "[/B]", inner));
}

TEST(TestGUITextLayoutParseText, DeeplyNested100LevelsBold)
{
  std::string open, close;
  for (int i = 0; i < 100; ++i)
  {
    open += "[B]";
    close = "[/B]" + close;
  }
  EXPECT_TRUE(FiltersTo(open + "deep" + close, "deep"));
}

TEST(TestGUITextLayoutParseText, DeeplyNested100Levels_OneMissingClose)
{
  // 100 opens, 99 closes – outermost [B] has no matching [/B] via lookahead,
  // so it is not activated. Parser must not crash; content must be extracted.
  std::string open, close;
  for (int i = 0; i < 100; ++i)
    open += "[B]";
  for (int i = 0; i < 99; ++i)
    close += "[/B]";
  EXPECT_TRUE(PlainTextHas(open + "deep" + close, "deep"));
}

TEST(TestGUITextLayoutParseText, WhitespaceOnlyContent)
{
  EXPECT_NO_FATAL_FAILURE(GetPlainText("   \t  "));
}

// ===========================================================================
// 14. Depth integrity: text following a fully-closed nested pair
// ===========================================================================

TEST(TestGUITextLayoutParseText, DepthZeroAfterMatchedPair_PlainTextAfter)
{
  // After [B][B]styled[/B][/B] depth is 0; subsequent text is unaffected.
  EXPECT_TRUE(FiltersTo("[B][B]styled[/B][/B] plain", "styled plain"));
}

TEST(TestGUITextLayoutParseText, TwoCompleteNestedPairsInSequence)
{
  EXPECT_TRUE(FiltersTo(
      "[B][B]first[/B][/B] [I][I]second[/I][/I]",
      "first second"));
}

// ===========================================================================
// 15. Case-transform tags: verify content transformation (not just stripping)
//    These tests guard that [UPPERCASE]/[LOWERCASE]/[CAPITALIZE] actually
//    transform the text, not merely consume the tag tokens.
// ===========================================================================

TEST(TestGUITextLayoutParseText, UppercaseTransformsContent)
{
  EXPECT_TRUE(FiltersTo("[UPPERCASE]hello world[/UPPERCASE]", "HELLO WORLD"));
}

TEST(TestGUITextLayoutParseText, LowercaseTransformsContent)
{
  EXPECT_TRUE(FiltersTo("[LOWERCASE]HELLO WORLD[/LOWERCASE]", "hello world"));
}

TEST(TestGUITextLayoutParseText, CapitalizeTransformsContent)
{
  EXPECT_TRUE(FiltersTo("[CAPITALIZE]hello world[/CAPITALIZE]", "Hello World"));
}

TEST(TestGUITextLayoutParseText, UppercaseNestedDoesNotDoubleTransform)
{
  // [UPPERCASE][UPPERCASE]text[/UPPERCASE][/UPPERCASE] – text is uppercased once
  EXPECT_TRUE(FiltersTo("[UPPERCASE][UPPERCASE]hello[/UPPERCASE][/UPPERCASE]", "HELLO"));
}

TEST(TestGUITextLayoutParseText, UppercaseOuterPreservesInnerContent)
{
  // [UPPERCASE] around a [LIGHT] span – the content must still be uppercased
  EXPECT_TRUE(FiltersTo("[UPPERCASE][LIGHT]hello[/LIGHT][/UPPERCASE]", "HELLO"));
}
