#!/usr/bin/python
# Karl Palsson, 2011
# parse the ntc curve data...

import csv
import os
import sys

class curve(object):
    """
    Parses a ntc curves file, to help provide the nearest temperature for a resistance
    """

    def __init__(self, curvefile):
        """create the curve decoder, reading in the curve file"""
        this_dir, this_filename = os.path.split(__file__)
        curvefile_with_path = os.path.join(this_dir, curvefile)
        raw = csv.reader(open(curvefile_with_path, 'rb'), delimiter=',')
        self.curve = {}
        for row in raw:
            # save full precision resistance, and whole decimal degrees
            self.curve[int(row[1])] = int(float(row[0]))

    def find_temp(self, resistance):
        """
        Find the nearest specified temperature for this resistance.
        No rounding, interpolation, and no performance cares
        resistance can be a string or float, no problems.
        """
        best_diff = sys.maxint
        best_key = None
        for key in self.curve.keys():
            this_diff = abs(key - float(resistance))
            if this_diff < best_diff:
                best_diff = this_diff
                best_key = key

        if best_key:
            return self.curve[best_key]
        else:
            raise Exception("No best key?!")
            
