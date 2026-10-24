"""Asset-free checks for macOS platform selection and compiler input validation."""
import importlib.util
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch


TOOLS = Path(__file__).resolve().parents[1]


def load(name):
    spec = importlib.util.spec_from_file_location(name, TOOLS / (name + '.py'))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class MacPlatformTests(unittest.TestCase):
    def test_platform_release_names(self):
        module = load('get_platform')
        for system, arch, expected in [
            ('Darwin', 'arm64', ('macos', 'arm64', '')),
            ('Darwin', 'aarch64', ('macos', 'arm64', '')),
            ('Darwin', 'x86_64', ('macos', 'x86_64', '')),
            ('Linux', 'x86_64', ('linux', 'x86_64', '')),
            ('Windows', 'AMD64', ('windows', 'x86_64', '.exe')),
        ]:
            with self.subTest(system=system, arch=arch), \
                    patch.object(module.platform, 'system', return_value=system), \
                    patch.object(module.platform, 'machine', return_value=arch):
                result = module.get_platform()
                self.assertEqual((result.system, result.machine, result.exe), expected)


class MacCompilerTests(unittest.TestCase):
    def setUp(self):
        self.wrapper = load('macos_compiler')
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.source = self.root / 'test.cpp'
        self.source.write_text('extern "C" int f(void) { return 1; }\n')

    def args(self):
        return ['-inline', 'noauto', '-ipa', 'file', '-interworking',
                '-i', str(self.root), '-lang=c++', '-c', str(self.source), '-o', 'out.o']

    def test_ascii_inputs_preserve_every_argument(self):
        argv = self.args()
        original = list(argv)
        self.wrapper.validate(argv)
        self.assertEqual(argv, original)

    def test_non_ascii_source_is_rejected(self):
        self.source.write_bytes(b'// \xc3\xa9\n')
        with self.assertRaisesRegex(ValueError, 'non-ASCII'):
            self.wrapper.validate(self.args())

    def test_nested_non_ascii_header_is_rejected(self):
        nested = self.root / 'nested'
        nested.mkdir()
        (nested / 'test.h').write_bytes(b'// \xc3\xa9\n')
        with self.assertRaisesRegex(ValueError, 'non-ASCII'):
            self.wrapper.validate(self.args())

    def test_response_files_are_rejected(self):
        with self.assertRaisesRegex(ValueError, 'response files'):
            self.wrapper.validate(self.args() + ['@flags.rsp'])

    def test_ambiguous_include_flags_are_rejected(self):
        with self.assertRaisesRegex(ValueError, 'separate -i DIR'):
            self.wrapper.validate(self.args() + ['-iinclude'])

    def test_missing_source_is_rejected(self):
        with self.assertRaisesRegex(ValueError, 'exactly one source'):
            self.wrapper.validate(['-lang=c'])


if __name__ == '__main__':
    unittest.main()
