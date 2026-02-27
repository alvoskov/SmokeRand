import lfsr_engine as lfsr
import sympy, unittest

class TestLfsrs(unittest.TestCase):
    def __init__(self, *args, **kwargs):
        super(TestLfsrs, self).__init__(*args, **kwargs)
        self.gen16 = lfsr.XorGenMaker(16)
        self.gen32 = lfsr.XorGenMaker(32)
        self.gen64 = lfsr.XorGenMaker(64)

    def test_xorrot32(self):
        T = self.gen32.make_xorrot_matrix(1, 9, 27)
        self.assertTrue(lfsr.is_full_period(T, False))

    def test_xorrot64(self):
        T = self.gen64.make_xorrot_matrix(5, 13, 47)
        self.assertTrue(lfsr.is_full_period(T, False))

    def test_xorrot64w16(self):
        T = self.gen16.make_xorrot4w_matrix(3, 7, 13)
        self.assertTrue(lfsr.is_full_period(T, False))

    def test_xorrot128w64(self):
        T = self.gen64.make_xorrot2w_matrix(3, 17, 52)
        self.assertTrue(lfsr.is_full_period(T, False))

    def test_xorrot256(self):
        T = self.gen64.make_xorrot4w_matrix(3, 8, 37)
        self.assertTrue(lfsr.is_full_period(T, False))

if __name__ == '__main__':
    unittest.main()
