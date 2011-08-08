
import upb
import unittest

class TestFieldDef(unittest.TestCase):
  def test_construction(self):
    fielddef1 = upb.FieldDef()
    self.assertTrue(fielddef1 is not None)
    fielddef2 = upb.FieldDef(number=1, name="field1", label=2, type=3)
    self.assertTrue(fielddef2 is not None)
    self.assertTrue(id(fielddef1) != id(fielddef2))

if __name__ == '__main__':
  unittest.main()
