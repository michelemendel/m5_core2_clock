# Quick Start: Fixing Hebrew Rectangle Display

## Current Status
✅ Hebrew characters are being output correctly  
❌ Displaying as rectangles because no Hebrew font is loaded

## What You Need
A Hebrew font file in LovyanGFX format (.h header file)

## Fastest Solution

### Option 1: Use Online Font Converter (Easiest)

1. **Go to LovyanGFX Font Generator:**
   - Visit: https://github.com/lovyan03/LovyanGFX/tree/master/tools
   - Or search for "LovyanGFX bitmapFontGenerator"

2. **Download/Clone the tool:**
   ```bash
   git clone https://github.com/lovyan03/LovyanGFX.git
   cd LovyanGFX/tools/bitmapFontGenerator
   ```

3. **Run the generator:**
   - Select a Hebrew TTF font (e.g., Arial Hebrew, David, Tahoma)
   - Font size: 12-14 pixels
   - Unicode range: `U+05D0-U+05FF` (Hebrew block)
   - Also include: `U+05F3` (׳), `U+05F4` (״), `U+0020` (space)
   - Generate as C header file

4. **Copy the generated file:**
   - Save as `hebrew_font.h` in your project folder

5. **Enable in display.cpp:**
   ```cpp
   // Change these lines (around line 5-7):
   #define HEBREW_FONT_AVAILABLE 1
   #include "hebrew_font.h"
   ```

6. **Update setHebrewFont() function:**
   ```cpp
   void Display::setHebrewFont() {
     M5.Display.setFont(&hebrew_font_12);  // Use your font variable name
   }
   ```

### Option 2: Use Python Script (If Available)

Some LovyanGFX tools include a Python script:
```bash
python bitmapFontGenerator.py --font "Arial Hebrew.ttf" --size 12 --unicode-range "U+05D0-U+05FF" --output hebrew_font.h
```

## Verification

After adding the font:
- Compile and upload
- Hebrew date should show as: **ט׳ בחשון תשפ״ו** (not rectangles)
- If still rectangles, check font variable name matches in `setHebrewFont()`

## Character Requirements

The font must include:
- Hebrew letters: א-ת (U+05D0-U+05EA)
- Geresh: ׳ (U+05F3)  
- Gershayim: ״ (U+05F4)
- Space: (U+0020)

## Memory Note

Hebrew fonts are typically 50-200KB. The M5Cardputer should have enough flash, but monitor compilation for memory warnings.

## Troubleshooting

**Still seeing rectangles?**
- Verify `HEBREW_FONT_AVAILABLE` is defined (not commented)
- Check font variable name in generated header matches `setHebrewFont()`
- Ensure all Unicode ranges are included
- Try a different font size (10-16px)

**Compilation errors?**
- Check LovyanGFX version compatibility
- Verify font header file syntax
- Ensure all includes are present

## Alternative: Keep Transliteration

If font generation is too complex, we can switch back to transliteration (English text like "9 b'Cheshvan 5786") which doesn't require a font.

