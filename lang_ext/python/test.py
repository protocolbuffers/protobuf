
import upb
import unittest

class TestFieldDef(unittest.TestCase):
  def test_construction(self):
    fielddef1 = upb.FieldDef()
    self.assertTrue(fielddef1.number is None)
    self.assertTrue(fielddef1.name is None)
    self.assertTrue(fielddef1.type is None)
    self.assertEqual(fielddef1.label, upb.LABEL_OPTIONAL)

    fielddef2 = upb.FieldDef(number=5, name="field2",
                             label=upb.LABEL_REQUIRED, type=upb.TYPE_INT32,
                             type_name="MyType")

    self.assertTrue(id(fielddef1) != id(fielddef2))
    self.assertEqual(fielddef2.number, 5)
    self.assertEqual(fielddef2.name, "field2")
    self.assertEqual(fielddef2.label, upb.LABEL_REQUIRED)
    self.assertEqual(fielddef2.type, upb.TYPE_INT32)
    self.assertEqual(fielddef2.type_name, "MyType")

    fielddef2.number = 8
    self.assertEqual(fielddef2.number, 8)

    fielddef2.name = "xxx"
    self.assertEqual(fielddef2.name, "xxx")

    fielddef2.label = upb.LABEL_REPEATED
    self.assertEqual(fielddef2.label, upb.LABEL_REPEATED)

    fielddef2.type = upb.TYPE_FLOAT
    self.assertEqual(fielddef2.type, upb.TYPE_FLOAT)

  # TODO: test that assigning invalid values is properly prevented.

class TestMessageDef(unittest.TestCase):
  def test_construction(self):
    msgdef1 = upb.MessageDef()
    self.assertTrue(msgdef1.fqname is None)
    self.assertEqual(msgdef1.fields(), [])

    msgdef2 = upb.MessageDef(fqname="Message2", fields=[
      upb.FieldDef(number=1, name="field1", type=upb.TYPE_INT32)
    ])

if __name__ == '__main__':
  unittest.main()
