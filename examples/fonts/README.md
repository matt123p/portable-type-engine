# Generated example fonts

These fonts are sampled at 128 pixels and can be resized at runtime by PTE.
Each generated C file records its complete recreation command in its header.

| File | Characters | Getter |
| --- | --- | --- |
| `roboto_regular.c` | ISO 8859-1 graphic characters | `get_Roboto_Regular()` |
| `roboto_bold.c` | ISO 8859-1 graphic characters | `get_Roboto_Bold()` |
| `roboto_italic.c` | ISO 8859-1 graphic characters | `get_Roboto_Italic()` |
| `roboto_bold_italic.c` | ISO 8859-1 graphic characters | `get_Roboto_Bold_Italic()` |
| `material_icons.c` | All 2,226 mapped glyphs | `get_Material_Icons()` |

The Roboto range is `U+0020-U+007E` plus `U+00A0-U+00FF`; ISO 8859-1 control
codes are omitted. Material Icons uses the font's complete Unicode cmap,
including its private-use icon code points.

Roboto is licensed under the SIL Open Font License in
`src/font-tool/OFL-Roboto.txt`. Material Icons is licensed under Apache 2.0 in
`src/font-tool/LICENSE-Material-Icons.txt`.
