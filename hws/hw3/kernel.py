from math import pi, exp

def gaus(x, y, sig):
    return 1.0 / (2 * pi * sig ** 2) * exp(-1.0 * (x ** 2 + y ** 2) / (2 * sig ** 2))

# main
sig = 1
beg = -1
end = 1
for x in xrange(beg, end + 1):
    for y in xrange(beg, end + 1):
        print(gaus(x, y, sig)),
    print