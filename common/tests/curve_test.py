#! /usr/bin/env python
# Karl Palsson, 2011
# tests for the curve parser


from .. import curve
class curve_test():

    def setup(self):
        self.cc = curve.curve("ntc.10kz.curve.csv")
        
    def test_flexible_input_float(self):
        temp = self.cc.find_temp(10000.1234)
        assert temp == 25

    def test_flexible_input_float_str(self):
        temp = self.cc.find_temp("10000.1234")
        assert temp == 25

    def test_exact_10k(self):
        temp = self.cc.find_temp(10000)
        assert temp == 25

    def test_near_10k(self):
        temp = self.cc.find_temp(10005)
        assert temp == 25

    def test_under_10k(self):
        temp = self.cc.find_temp(9950)
        assert temp == 25

    def test_boundary_low(self):
        temp = self.cc.find_temp(355)
        assert temp == 123

    def test_boundary_high(self):
        temp = self.cc.find_temp(354)
        assert temp == 124

    def test_exact_294485(self):
        temp = self.cc.find_temp(294485)
        assert temp == -38
            
    def test_exact_121(self):
        temp = self.cc.find_temp(378)
        assert temp == 121

    def test_offscale_bottom(self):
        temp = self.cc.find_temp(-1000)
        assert temp == 125

    def test_offscale_top(self):
        temp = self.cc.find_temp(436050)
        assert temp == -40

        
