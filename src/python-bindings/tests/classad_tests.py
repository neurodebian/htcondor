#!/usr/bin/python

import re
import classad
import datetime
import unittest

class TestClassad(unittest.TestCase):

    def test_classad_constructor(self):
        ad = classad.ClassAd('[foo = "1"; bar = 2]')
        self.assertEquals(ad['foo'], "1")
        self.assertEquals(ad['bar'], 2)
        self.assertRaises(KeyError, ad.__getitem__, 'baz')

    def test_load_classad_from_file(self):
        ad = classad.parse(open("tests/test.ad"))
        self.assertEqual(ad["foo"], "bar")
        self.assertEqual(ad["baz"], classad.Value.Undefined)
        self.assertRaises(KeyError, ad.__getitem__, "bar")

    def test_old_classad(self):
        ad = classad.parseOld(open("tests/test.old.ad"))
        contents = open("tests/test.old.ad").read()
        keys = []
        for line in contents.splitlines():
            info = line.split(" = ")
            if len(info) != 2:
                continue
            self.assertTrue(info[0] in ad)
            self.assertEqual(ad.lookup(info[0]).__repr__(), info[1])
            keys.append(info[0])
        for key in ad:
            self.assertTrue(key in keys)

    def test_exprtree(self):
        ad = classad.ClassAd()
        ad["foo"] = classad.ExprTree("2+2")
        expr = ad["foo"]
        self.assertEqual(expr.__repr__(), "2 + 2")
        self.assertEqual(expr.eval(), 4)

    def test_exprtree_func(self):
        ad = classad.ClassAd()
        ad["foo"] = classad.ExprTree('regexps("foo (bar)", "foo bar", "\\\\1")')
        self.assertEqual(ad.eval("foo"), "bar")

    def test_ad_assignment(self):
        ad = classad.ClassAd()
        ad["foo"] = 2.1
        self.assertEqual(ad["foo"], 2.1)
        ad["foo"] = 2
        self.assertEqual(ad["foo"], 2)
        ad["foo"] = "bar"
        self.assertEqual(ad["foo"], "bar")
        self.assertRaises(TypeError, ad.__setitem__, {})

    def test_ad_refs(self):
        ad = classad.ClassAd()
        ad["foo"] = classad.ExprTree("bar + baz")
        ad["bar"] = 2.1
        ad["baz"] = 4
        self.assertEqual(ad["foo"].__repr__(), "bar + baz")
        self.assertEqual(ad.eval("foo"), 6.1)

    def test_ad_special_values(self):
        ad = classad.ClassAd()
        ad["foo"] = classad.ExprTree('regexp(12, 34)')
        ad["bar"] = classad.Value.Undefined
        self.assertEqual(ad["foo"].eval(), classad.Value.Error)
        self.assertNotEqual(ad["foo"].eval(), ad["bar"])
        self.assertEqual(classad.Value.Undefined, ad["bar"])

    def test_ad_iterator(self):
        ad = classad.ClassAd()
        ad["foo"] = 1
        ad["bar"] = 2
        self.assertEqual(len(ad), 2)
        self.assertEqual(len(list(ad)), 2)
        self.assertEqual(list(ad)[1], "foo")
        self.assertEqual(list(ad)[0], "bar")
        self.assertEqual(list(ad.items())[1][1], 1)
        self.assertEqual(list(ad.items())[0][1], 2)
        self.assertEqual(list(ad.values())[1], 1)
        self.assertEqual(list(ad.values())[0], 2)

    def test_ad_lookup(self):
        ad = classad.ClassAd()
        ad["foo"] = classad.Value.Error
        self.assertTrue(isinstance(ad.lookup("foo"), classad.ExprTree))
        self.assertEquals(ad.lookup("foo").eval(), classad.Value.Error)

    def test_get(self):
        ad = classad.ClassAd()
        self.assertEquals(ad.get("foo"), None)
        self.assertEquals(ad.get("foo", "bar"), "bar")
        ad["foo"] = "baz"
        self.assertEquals(ad.get("foo"), "baz")
        self.assertEquals(ad.get("foo", "bar"), "baz")

    def test_setdefault(self):
        ad = classad.ClassAd()
        self.assertEquals(ad.setdefault("foo", "bar"), "bar")
        self.assertEquals(ad.get("foo"), "bar")
        ad["bar"] = "baz"
        self.assertEquals(ad.setdefault("bar", "foo"), "baz")

    def test_update(self):
        ad = classad.ClassAd()
        ad.update({"1": 2})
        self.assertTrue("1" in ad)
        self.assertEquals(ad["1"], 2)
        ad.update([("1",3)])
        self.assertEquals(ad["1"], 3)
        other = classad.ClassAd({"3": "5"})
        ad.update(other)
        del other
        self.assertTrue("3" in ad)
        self.assertEquals(ad["3"], "5")

    def test_invalid_ref(self):
        expr = classad.ExprTree("foo")
        self.assertEquals(classad.Value.Undefined, expr.eval())

    def test_temp_scope(self):
        expr = classad.ExprTree("foo")
        self.assertEquals("bar", expr.eval({"foo": "bar"}))
        ad = classad.ClassAd({"foo": "baz", "test": classad.ExprTree("foo")})
        expr = ad["test"]
        self.assertEquals("baz", expr.eval())
        self.assertEquals("bar", expr.eval({"foo": "bar"}))
        self.assertEquals("bar", expr.eval({"foo": "bar"}))
        self.assertEquals("baz", expr.eval())

    def test_abstime(self):
        expr = classad.ExprTree('absTime("2013-09-12T07:50:23")')
        dt = expr.eval()
        self.assertTrue(isinstance(dt, datetime.datetime))
        self.assertEquals(dt.year, 2013)
        self.assertEquals(dt.month, 9)
        self.assertEquals(dt.day, 12)
        self.assertEquals(dt.hour, 7)
        self.assertEquals(dt.minute, 50)
        self.assertEquals(dt.second, 23)

        ad = classad.ClassAd({"foo": dt})
        dt2 = ad["foo"]
        self.assertTrue(isinstance(dt2, datetime.datetime))
        self.assertEquals(dt, dt2)

        ad = classad.ClassAd({"foo": datetime.datetime.now()});
        td = (datetime.datetime.now()-ad["foo"])
        self.assertEquals(td.days, 0)
        self.assertTrue(td.seconds < 300)

    def test_reltime(self):
        expr = classad.ExprTree('relTime(5)')
        self.assertEquals(expr.eval(), 5)

    def test_quote(self):
        self.assertEquals(classad.quote("foo"), '"foo"')
        self.assertEquals(classad.quote('"foo'), '"\\"foo"')
        for i in ["foo", '"foo', '"\\"foo']:
            self.assertEquals(i, classad.unquote(classad.quote(i)))

if __name__ == '__main__':
    unittest.main()