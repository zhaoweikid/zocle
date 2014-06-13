# coding: utf-8
import os, sys
os.putenv('DYLD_LIBRARY_PATH', os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

def test():
    libpath = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    os.environ['LD_LIBRARY_PATH'] = libpath
    
    tests = []
    for root,dirs,files in os.walk('.'):
        for fn in files:
            if fn.endswith('_test'):
                fpath = os.path.join(root, fn)
                tests.append(fpath)

    print tests
    
    maxlen = 0
    result = [''] * len(tests)
    for i in range(0, len(tests)):
        t = tests[i]
        ret = os.system(t) 
        result[i] = ret
        #x = sum(divmod(len(t), 8))
        x = len(t)
        if x > maxlen:
            maxlen = x
    x = maxlen / 8 + 2
    print '====== result ======'
    for i in range(0, len(tests)):
        tb = '\t' * (x - len(tests[i]) / 8)  
        if result[i] == 0:
            print '%s%s\33[32msuccess\33[0m' % (tests[i], tb)
        else:
            print '%s%s\33[31mfailed\33[0m\t%d' % (tests[i], tb, result[i])


if __name__ == '__main__':
    test()

