import scipy.misc
import numpy as np;
m = np.load("out.npy");
scipy.misc.imsave('outfile.jpg', m)


