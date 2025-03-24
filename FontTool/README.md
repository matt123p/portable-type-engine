# Portable Type Engine Font Tool User Guide

This tool converts font files (such as TTF files) into C arrays that can be used with the Portable Type Engine. It extracts character glyphs, kerning information, and other metrics from the font file and generates C code that you can include in your projects.

## What Does It Do?

- **Generates Image Data:** Converts the glyphs into image data (packed into an array).
- **Processes Kerning:** Extracts kerning information between pairs of characters, scaling it to the correct pixel values.
- **Produces C Code:** Outputs a C file with arrays for glyphs, image data, and kerning data, along with a function to retrieve the font structure.

## Installation

### Prerequisites
- **Python 3.x:** Ensure Python is installed. You can download it from [python.org](https://www.python.org/downloads/).
- **Required Python Packages:**  
  This tool depends on the following Python packages:
  - [Pillow](https://pillow.readthedocs.io/)
  - [fontTools](https://github.com/fonttools/fonttools)

### Installation Steps

1. **Install Python 3:**  
   Follow the installation instructions on [python.org](https://www.python.org/downloads/).

2. **Install Required Packages:**  

Open a terminal or command prompt and run:
```sh
pip install Pillow fonttools
```

## How to Run the Tool

1.  **Open a Terminal:**  
    Open a command prompt or terminal in the folder that contains fontsampler.py.
2.  **Run the Tool:**  
    Use the following command format:

```sh
python fontsampler.py <font-file-path> [output file-name] [--charset <charset options>]
```

-   \<font-file-path\>: Path to the font file you want to convert (e.g., C:\path\to\yourfont.ttf).
-   [output file-name]: (Optional) Name for the output C file; default is output_font.c.
-   [--charset <charset options>]: (Optional) Define which characters to process using shorthand options.

### Examples

**Basic Usage:**

```sh
python fontsampler.py C:\path\to\yourfont.ttf
```

This command loads the specified font file and creates output_font.c in the same directory.

**Specifying an Output File:**

```sh
python fontsampler.py C:\path\to\yourfont.ttf myfont.c
```

**Using Charset Options:** 

The --charset option accepts options to choose which ranges to convert:

-   n: digits (0-9)
-   a: lowercase letters (a-z)
-   A: uppercase letters (A-Z)
-   Number from 1 to 9 (1, 2, etc.): uses ISO8859-n printable range.  See the [Wikipedia]([https://](https://en.wikipedia.org/wiki/ISO/IEC_8859)) article for more information about the ISO8859 character sets.
-   .: punctuation characters

For example, to process numbers, lowercase, and uppercase letters:

```sh
python fontsampler.py C:\path\to\yourfont.ttf myfont.c --charset "aAn"
```

