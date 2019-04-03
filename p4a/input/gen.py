import numpy as np

n = 500
u = ['write', 'listen', 'read', 'talk']

for i in range(n):
    s = 'read' + str(i) + '.in'
    f = open(s, 'w')
    kk = u[np.random.randint(0,4)] + ' ' + u[np.random.randint(0,4)] + '\n'
    for  j in range(i):
        kk += u[np.random.randint(0,4)] + ' ' + u[np.random.randint(0,4)] + '\n'
    f.write(kk)
    f.close()
