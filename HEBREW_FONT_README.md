# Hebrew Font Setup Instructions

This document explains how to add Hebrew font support to display Hebrew dates on the M5Cardputer.

## Current Status

The code is set up to support Hebrew fonts, but currently uses transliteration (English text) as a fallback. The Hebrew characters are already implemented in the code - you just need to generate and add the font file.

## Step 1: Generate Hebrew Font

You need to create a Hebrew font file compatible with LovyanGFX. Here are the options:

### Option A: Using LovyanGFX Font Generator (Recommended)

1. **Download/Clone LovyanGFX Font Generator:**
   ```bash
   git clone https://github.com/lovyan03/LovyanGFX.git
   cd LovyanGFX/tools/bitmapFontGenerator
   ```

2. **Or use the online converter** (if available):
   - Check: https://github.com/lovyan03/LovyanGFX/tree/master/tools

3. **Generate Font:**
   - Select a Hebrew TTF font (e.g., Arial Hebrew, David, Tahoma, or any Hebrew font)
   - Specify font size: **12-14 pixels** (recommended for date display)
   - Select Unicode ranges:
     - Hebrew letters: `U+05D0-U+05EA` (א-ת)
     - Geresh: `U+05F3` (׳)
     - Gershayim: `U+05F4` (״)
     - Space and "ב": `U+0020-U+002F`

4. **Output:**
   - Generate a C header file named `hebrew_font.h`
   - The font should export a variable like: `extern const lgfx::U8g2font hebrew_font_12;`

### Option B: Using loboris Font Converter

1. **Access the tool:**
   - Visit: https://community.m5stack.com/user/loboris/posts
   - Or search for "loboris font converter M5Stack"

2. **Follow similar steps** as Option A to generate the font

## Step 2: Add Font to Project

1. **Place the generated font file:**
   - Copy `hebrew_font.h` to your project directory (same folder as `display.cpp`)

2. **Enable the font:**
   - Edit `display.cpp`
   - Find this section near the top:
     ```cpp
     // Hebrew font support - uncomment when font is generated
     // #define HEBREW_FONT_AVAILABLE 1
     // #include "hebrew_font.h"
     ```
   
   - Uncomment and update:
     ```cpp
     #define HEBREW_FONT_AVAILABLE 1
     #include "hebrew_font.h"
     ```

3. **Update font loading (if needed):**
   - Edit `display.cpp`, find `Display::setHebrewFont()`
   - Update to match your font variable name:
     ```cpp
     void Display::setHebrewFont() {
       M5.Display.setFont(&hebrew_font_12);  // Adjust name to match your font
     }
     ```

## Step 3: Verify Font Format

The font should be compatible with LovyanGFX. Common formats:

- `lgfx::U8g2font` - Standard format
- `lgfx::IFont*` - Interface pointer format
- Font array/table format

Check your generated font file's format and adjust `setHebrewFont()` accordingly.

## Step 4: Compile and Test

1. **Compile the project:**
   ```bash
   # Using Arduino IDE or PlatformIO
   ```

2. **Check memory usage:**
   - Hebrew fonts can be 50-200KB+
   - Ensure your M5Cardputer has enough flash memory
   - Monitor compilation output for memory warnings

3. **Test the display:**
   - The Hebrew date should now show as: "ט׳ בחשון תשפ״ו" instead of transliteration
   - If you see squares, the font isn't loaded correctly

## Troubleshooting

### Font shows as squares:
- Verify `HEBREW_FONT_AVAILABLE` is defined
- Check font variable name matches in `setHebrewFont()`
- Ensure Unicode ranges include all Hebrew characters
- Verify font file is in the correct directory

### Memory errors:
- Try a smaller font size (10-12px instead of 14-16px)
- Generate font with only required characters (remove unused ranges)
- Check available flash memory on device

### Compilation errors:
- Verify font header file syntax
- Check LovyanGFX version compatibility
- Ensure all required includes are present

## Character Set Required

For Hebrew dates, you need:

**Hebrew Letters (22 characters):**
א ב ג ד ה ו ז ח ט י כ ל מ נ ס ע פ צ ק ר ש ת

**Hebrew Numbers (same as letters, used for gematria):**
Same as above

**Punctuation:**
- ׳ (U+05F3) - Geresh (single quote after day)
- ״ (U+05F4) - Gershayim (double quote after year)
- ב (U+05D1) - "in" prefix for months

**Special combinations:**
- טו (15) - Special day
- טז (16) - Special day

## Resources

- LovyanGFX Documentation: https://github.com/lovyan03/LovyanGFX
- Font Converter: Check LovyanGFX tools directory
- M5Stack Community: https://community.m5stack.com/
- Hebrew Unicode Block: U+0590-U+05FF

## Alternative: Keep Transliteration

If font generation is too complex or memory is limited, the current transliteration format ("9 b'Cheshvan 5786") works well and is readable.

