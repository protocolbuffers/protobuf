# -*- coding: utf-8 -*-
# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Tests whether the native UPB module loads in subinterpreters."""


from textwrap import dedent
import unittest


class SubinterpreterTest(unittest.TestCase):

    def test_upb_module_can_load_in_main_interpreter(self):
        # Assert that it works importing UPB in the main interpreter
        from google._upb import _message

    def test_upb_module_can_load_in_subinterpreters(self):
        try:
            import _interpreters as interpreters
        except ImportError:
            unittest.skip("Subinterpreters are not available")
            return

        wrapped = dedent(
            """
                try:
                    from google._upb import _message
                    assert _message._IS_UPB == 1

                except Exception as e:
                    print(f"Caught: {e}")
                    raise
            """
        )

        interpreter = interpreters.create(reqrefs=True)
        result = interpreters.run_string(interpreter, wrapped)

        self.assertEqual(result, None)


if __name__ == "__main__":
    unittest.main()
