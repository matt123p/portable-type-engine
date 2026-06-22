import importlib.util
import contextlib
import io
import sys
import tempfile
import types
import unittest
from pathlib import Path
from types import SimpleNamespace as NS


# fontsampler only needs TTFont when run as a command. Stub that import so these
# unit tests can exercise the table logic without requiring the optional tools.
if importlib.util.find_spec("fontTools") is None:
    fonttools = types.ModuleType("fontTools")
    ttlib = types.ModuleType("fontTools.ttLib")
    ttlib.TTFont = object
    fonttools.ttLib = ttlib
    sys.modules["fontTools"] = fonttools
    sys.modules["fontTools.ttLib"] = ttlib

import fontsampler


def value(amount=0):
    return NS(XAdvance=amount)


class FakeFont(dict):
    def __init__(self, cmap, **tables):
        super().__init__(tables)
        self.cmap = cmap

    def getBestCmap(self):
        return self.cmap


class FontSamplerTests(unittest.TestCase):
    def test_continuation_rle_encoding(self):
        encode = fontsampler.FontSampler.encode_pixels

        self.assertEqual(encode([]), [])
        self.assertEqual(encode([False] * 3 + [True] * 2), [0x32])
        self.assertEqual(encode([False] * 15 + [True]), [0xf0, 0x10])
        self.assertEqual(encode([False] * 35), [0xff, 0x50])

    def test_continuation_rle_round_trip(self):
        pixels = ([False] * 35 + [True] * 15 + [False, True] * 20
                  + [True] * 31)
        encoded = fontsampler.FontSampler.encode_pixels(pixels)
        decoded = []
        byte = 0
        high_nibble = True
        run_of_on = False
        switch_colour = False

        while len(decoded) < len(pixels):
            if switch_colour:
                run_of_on = not run_of_on
            run = ((encoded[byte] >> 4) if high_nibble
                   else (encoded[byte] & 0xf))
            if not high_nibble:
                byte += 1
            high_nibble = not high_nibble
            switch_colour = run < 15
            decoded.extend([run_of_on] * run)

        self.assertEqual(decoded, pixels)

    def sampler_with_glyphs(self):
        sampler = fontsampler.FontSampler([])
        for code in (97, 66, 65):
            sampler.m_glyphs[code] = fontsampler.Glyph(
                chr(code), code, 1, 1, 0, 0, code, 1)
        return sampler

    def test_identical_kerning_rows_are_shared(self):
        sampler = self.sampler_with_glyphs()
        sampler.m_kerns = [
            fontsampler.Kern(65, 66, -10),
            fontsampler.Kern(97, 66, -10),
        ]
        sampled_font = NS(size=1000)
        tt_font = FakeFont({}, head=NS(unitsPerEm=1000))

        glyph_rows, rows, entries = sampler.buildKernRows(sampled_font, tt_font)

        self.assertEqual(glyph_rows, [0, 0xffff, 0])
        self.assertEqual(rows, [(0, 1)])
        self.assertEqual(entries, [(1, -10)])

    def test_zero_scaled_kerning_is_omitted(self):
        sampler = self.sampler_with_glyphs()
        sampler.m_kerns = [fontsampler.Kern(65, 66, 1)]
        sampled_font = NS(size=128)
        tt_font = FakeFont({}, head=NS(unitsPerEm=1000))

        glyph_rows, rows, entries = sampler.buildKernRows(sampled_font, tt_font)

        self.assertEqual(glyph_rows, [0xffff, 0xffff, 0xffff])
        self.assertEqual(rows, [])
        self.assertEqual(entries, [])

    def test_glyph_output_is_sorted(self):
        sampler = self.sampler_with_glyphs()
        sampled_font = NS(
            size=128,
            getname=lambda: ("Test Font", "Regular"),
            getmetrics=lambda: (100, 20),
        )
        tt_font = FakeFont({65: "A", 66: "B", 97: "a"},
                           head=NS(unitsPerEm=1000))

        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "font.c"
            sampler.convertFont(sampled_font, tt_font, output)
            glyph_table = output.read_text().split("_glyphs[", 1)[1].split("};", 1)[0]
            glyph_lines = [line.strip() for line in glyph_table.splitlines()
                           if line.startswith(" { ")]

        self.assertEqual([int(line.split(",", 1)[0][2:]) for line in glyph_lines],
                         [65, 66, 97])

    def test_generated_accessor_does_not_include_source_size(self):
        sampler = self.sampler_with_glyphs()
        sampled_font = NS(
            size=128,
            getname=lambda: ("Test Font", "Regular"),
            getmetrics=lambda: (100, 20),
        )
        tt_font = FakeFont({65: "A", 66: "B", 97: "a"},
                           head=NS(unitsPerEm=1000))

        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "font.c"
            sampler.convertFont(sampled_font, tt_font, output)
            generated = output.read_text()

        self.assertIn("pte_base_font *get_Test_Font()", generated)
        self.assertNotIn("get_Test_Font128", generated)

    def test_generation_command_is_embedded(self):
        sampler = self.sampler_with_glyphs()
        sampled_font = NS(
            size=128,
            getname=lambda: ("Test Font", "Regular"),
            getmetrics=lambda: (100, 20),
        )
        tt_font = FakeFont({65: "A", 66: "B", 97: "a"},
                           head=NS(unitsPerEm=1000))

        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "font.c"
            sampler.convertFont(sampled_font, tt_font, output,
                                "python fontsampler.py --font font.ttf --output font.c")
            generated = output.read_text()

        self.assertIn("// Recreate with:\n// python fontsampler.py --font font.ttf --output font.c",
                      generated)

    def test_range_and_symbols_options_can_be_mixed(self):
        args = fontsampler.create_argument_parser().parse_args([
            "--font", "font.ttf", "--output", "font.c",
            "--range", "0x41-0x42,", "67-67",
            "--range", "0x44-0x44", "--symbols", "€A",
        ])

        self.assertEqual(args.ranges, [(65, 66), (67, 67), (68, 68)])
        sampler = fontsampler.FontSampler(
            unicode_ranges=args.ranges, symbols=args.symbols)
        selected = []
        sampler.convertGlyph = lambda font, character: selected.append(character)
        sampler.calcAllKerns = lambda tt_font: None
        sampled_font = NS(
            size=128,
            getname=lambda: ("Test Font", "Regular"),
            getmetrics=lambda: (100, 20),
        )

        with tempfile.TemporaryDirectory() as directory:
            sampler.convertFont(sampled_font, {}, Path(directory) / "font.c")

        self.assertEqual([ord(character) for character in selected],
                         [65, 66, 67, 68, 8364])

    def test_symbols_only_does_not_include_default_ranges(self):
        sampler = fontsampler.FontSampler([], "€✓")
        selected = []
        sampler.convertGlyph = lambda font, character: selected.append(character)
        sampler.calcAllKerns = lambda tt_font: None
        sampled_font = NS(
            size=128,
            getname=lambda: ("Test Font", "Regular"),
            getmetrics=lambda: (100, 20),
        )

        with tempfile.TemporaryDirectory() as directory:
            sampler.convertFont(sampled_font, {}, Path(directory) / "font.c")

        self.assertEqual([ord(character) for character in selected], [8364, 10003])

    def test_invalid_range_is_rejected(self):
        with contextlib.redirect_stderr(io.StringIO()):
            with self.assertRaises(SystemExit):
                fontsampler.create_argument_parser().parse_args(
                    ["--font", "font.ttf", "--range", "0x100-0x20"])

    def test_charset_option_has_been_removed(self):
        with contextlib.redirect_stderr(io.StringIO()):
            with self.assertRaises(SystemExit):
                fontsampler.create_argument_parser().parse_args(
                    ["--font", "font.ttf", "--charset", "aAn"])

    def test_font_and_output_options(self):
        parser = fontsampler.create_argument_parser()

        args = parser.parse_args([
            "--font", "font.ttf", "--output", "generated.c"])

        self.assertEqual(args.font_file, "font.ttf")
        self.assertEqual(args.output_file, "generated.c")

    def test_output_is_required(self):
        with contextlib.redirect_stderr(io.StringIO()):
            with self.assertRaises(SystemExit):
                fontsampler.create_argument_parser().parse_args(
                    ["--font", "font.ttf"])

    def test_positional_font_is_rejected(self):
        with contextlib.redirect_stderr(io.StringIO()):
            with self.assertRaises(SystemExit):
                fontsampler.create_argument_parser().parse_args(["font.ttf"])

    def test_legacy_and_gpos_kerns_are_merged_and_sorted(self):
        sampler = self.sampler_with_glyphs()
        legacy = NS(kernTables=[NS(kernTable={("A", "B"): -10})])

        class2 = NS(
            Format=2,
            Coverage=NS(glyphs=["A"]),
            ClassDef1=NS(classDefs={"A": 1}),
            ClassDef2=NS(classDefs={"B": 1}),
            Class1Record=[
                NS(Class2Record=[NS(Value1=value()), NS(Value1=value())]),
                NS(Class2Record=[NS(Value1=value()), NS(Value1=value(-20))]),
            ],
        )
        pair_record = NS(SecondGlyph="A", Value1=value(-5))
        class1 = NS(Format=1, Coverage=NS(glyphs=["a"]),
                    PairSet=[NS(PairValueRecord=[pair_record])])
        extension = NS(ExtensionLookupType=2, ExtSubTable=class1)
        lookups = [NS(LookupType=2, SubTable=[class2]),
                   NS(LookupType=9, SubTable=[extension])]
        feature = NS(FeatureTag="kern", Feature=NS(LookupListIndex=[0, 1]))
        gpos = NS(table=NS(
            LookupList=NS(Lookup=lookups),
            FeatureList=NS(FeatureRecord=[feature]),
        ))
        tt_font = FakeFont({65: "A", 66: "B", 97: "a"},
                           kern=legacy, GPOS=gpos)

        sampler.calcAllKerns(tt_font)

        self.assertEqual([(kern.first, kern.second, kern.amount)
                          for kern in sampler.m_kerns],
                         [(65, 66, -20), (97, 65, -5)])


if __name__ == "__main__":
    unittest.main()
