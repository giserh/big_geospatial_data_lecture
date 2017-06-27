#!/usr/bin/python3
from urllib import request;
from pyspark import SparkContext, SparkConf
import tempfile;
import sys
from plyfile import PlyData, PlyElement
import numpy as np;
import time;


extent = [545605, 545680, 5800690, 5800765];



dataset = ["http://www.martinwerner.de/ply/ffffff3f_ffffff02.ply",
"http://www.martinwerner.de/ply/ffffff3f_ffffff03.ply",
"http://www.martinwerner.de/ply/ffffff3f_ffffff04.ply",
"http://www.martinwerner.de/ply/ffffff3f_ffffff05.ply",
"http://www.martinwerner.de/ply/ffffff3f_ffffff06.ply",
"http://www.martinwerner.de/ply/ffffff40_ffffff02.ply",
"http://www.martinwerner.de/ply/ffffff40_ffffff03.ply",
"http://www.martinwerner.de/ply/ffffff40_ffffff04.ply",
"http://www.martinwerner.de/ply/ffffff40_ffffff05.ply",
"http://www.martinwerner.de/ply/ffffff40_ffffff06.ply",
"http://www.martinwerner.de/ply/ffffff41_ffffff02.ply",
"http://www.martinwerner.de/ply/ffffff41_ffffff03.ply",
"http://www.martinwerner.de/ply/ffffff41_ffffff04.ply",
"http://www.martinwerner.de/ply/ffffff41_ffffff05.ply",
"http://www.martinwerner.de/ply/ffffff41_ffffff06.ply",
"http://www.martinwerner.de/ply/ffffff42_ffffff02.ply",
"http://www.martinwerner.de/ply/ffffff42_ffffff03.ply",
"http://www.martinwerner.de/ply/ffffff42_ffffff04.ply",
"http://www.martinwerner.de/ply/ffffff42_ffffff05.ply",
"http://www.martinwerner.de/ply/ffffff42_ffffff06.ply",
"http://www.martinwerner.de/ply/ffffff43_ffffff03.ply",
"http://www.martinwerner.de/ply/ffffff43_ffffff04.ply",
"http://www.martinwerner.de/ply/ffffff43_ffffff05.ply",
"http://www.martinwerner.de/ply/ffffff43_ffffff06.ply"]

# uncomment for a small dataset
#dataset = ["http://www.martinwerner.de/ply/ffffff3f_ffffff02.ply",
#           "http://www.martinwerner.de/ply/ffffff3f_ffffff03.ply",
#           "http://www.martinwerner.de/ply/ffffff3f_ffffff04.ply"
#] 


# Read the URL from STDIN (this will come from hadoop)
def scanPly(url):
    tf = tempfile.NamedTemporaryFile()
    temp_file_name = tf.name
    tf.close()
    for i in range(1,10): # up to ten retries (server might be unfriendly as all requests come in at the same time)
        try:
            request.urlretrieve (url, temp_file_name)
        except :
            print("Failed for %s - retrying in 5s" % (url))
            time.sleep(5)
        else :
            #stops the inner loop if there is no error
            break
        
    # Load the temp file back into a numpy array data
    plydata = PlyData.read(temp_file_name)
    data = np.column_stack([plydata["vertex"]["x"],plydata["vertex"]["y"],plydata["vertex"]["z"]])

    # Now, let us only emit trees
    tree_conditional = ((data[:,2] > 110) & (data[:,2] < 112));
    trees = data[tree_conditional,:];
    return trees;
    # @TODO: Remove temp file

    



def key(p):
  # 1024x1024
  x = (p[0] - extent[0])/(extent[1]-extent[0])*1024;
  y = (p[1] - extent[2])/(extent[3]-extent[2])*1024;

  return ("%04d-%04d" % (int(x), int(y)));

def dekey(k):
  x,y = k.split("-");
  return int(x), int(y)

if __name__ == '__main__':
# Connect the the cluster under the name trees
    conf = SparkConf().setAppName("trees")
    sc = SparkContext(conf=conf)
 
    print(dataset);
    print("Distributed Loading");
    rdds = sc.parallelize(dataset).flatMap(lambda x: scanPly(x));
    print(rdds.count());
    # Load the file
    print("MapReduce");
    counts = rdds.map(lambda s: (key(s),1)).reduceByKey(lambda a, b: a + b);
    result = counts.collect();
    # Result crunching (non-parallel)
    print ("Result Denormalization")
    dekeyed = [dekey(x[0])+(x[1],) for x in result]
    # Now, we have x,y,<count>
    m = np.zeros([1024,1024]);
    for i in dekeyed:
      m[i[0],i[1]] = i[2];

    np.save("out.npy",m);
 
    
