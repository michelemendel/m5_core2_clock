/*
 * Hebrew Font for LovyanGFX
 * 
 * TO GENERATE THIS FONT:
 * 1. Use the LovyanGFX font converter tool:
 *    - Online: https://github.com/lovyan03/LovyanGFX/tree/master/tools/bitmapFontGenerator
 *    - Or use the loboris font converter: https://community.m5stack.com/user/loboris/posts
 * 
 * 2. Select a Hebrew TTF font (e.g., Arial Hebrew, David, or any Hebrew font)
 * 
 * 3. Specify these Unicode ranges for Hebrew characters:
 *    - Hebrew letters: U+05D0-U+05EA (א-ת)
 *    - Hebrew numbers: U+05D0-U+05EA (same as letters, used for gematria)
 *    - Punctuation: U+05F3 (׳ geresh), U+05F4 (״ gershayim)
 *    - Space: U+0020
 *    - Additional: U+0020-U+002F (for " ב" prefix)
 * 
 * 4. Recommended font size: 12-16 pixels for date display
 * 
 * 5. Generate the font header file and name it hebrew_font.h
 * 
 * 6. The generated font should be in LovyanGFX format with a structure like:
 *    extern const lgfx::U8g2font hebrew_font_12;
 * 
 * PLACEHOLDER: This file will be replaced with the actual generated font.
 * For now, the code will fall back to transliteration if this font is not available.
 */

// Placeholder - replace with actual font once generated
// #include will be added in display.cpp when font is ready
// For now, define a guard to allow compilation
#ifndef HEBREW_FONT_AVAILABLE
#define HEBREW_FONT_AVAILABLE 0
#endif

#if HEBREW_FONT_AVAILABLE
// Uncomment when you have the actual font:
// extern const lgfx::U8g2font hebrew_font_12;
// Or if using different format:
// extern const lgfx::IFont* hebrew_font;
#endif

