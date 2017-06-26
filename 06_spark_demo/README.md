# Spark Demo

This is a simple example to show, how to use spark on Amazon AWS. The files in this git repository
should be enough to get you started.

The architecture of the big data stack is held quite simple, however, some tools are actually needed. These
are

# Docker
Instead of provisioning machines one by one by hand, we are going to cluster all AWS instances into a swarm
and deploy a docker image on all of them using a docker-compose.yml. This means, that files and python libraries
inside our docker image are available on workers as well as the master.

Especially, we add the source file sp.py and the needed python libraries for loading PLY files. The Dockerfile
has all installation information on these aspects.

Use
```
docker build . -t mwernerds/sparkdemo
docker login
docker push mwernerds/sparkdemo
```
to make it available online (replacing of course my account and the project name)

# Spark
Spark is a modern replacement / extension to the Hadoop way of managing big data. It supports much more functionality
and is based on Resilient Distributed Datasets (RDDs), which provide an API for distributed operations on them.

The file sp.py contains all functionality in a single file (not a mapper and a reducer, but a single file).
This file can be run with
```
spark-submit sp.py
```
if everything is well-organized

# Dataset
The dataset consists of several PLY files made available on my web page (only during this video, it is 1GB of data).
I chose a very generic setting not using any integrated mechanisms such as HDFS or Amazon S3. Note that you are
free to do so, but any way of reading a remote file will be sufficient for this implementation.


# A short walkthrough of sp.py

The file starts with a set of libraries needed and a specification of the extent of the dataset
for calculating keys.
```
from urllib import request;
from pyspark import SparkContext, SparkConf
import tempfile;
import sys
from plyfile import PlyData, PlyElement
import numpy as np;


extent = [545605, 545680, 5800690, 5800765];
```
This extent is minx, maxx, miny, maxy and being used by the functions key and dekey.

The file proceeds with the dataset given as a set of URLs:
```
dataset = ["http://www.martinwerner.de/ply/ffffff3f_ffffff02.ply",
"http://www.martinwerner.de/ply/ffffff3f_ffffff03.ply",
[...]
```
Then, we learn to load a PLY file for this project:
```
def scanPly(url):
    tf = tempfile.NamedTemporaryFile()
    temp_file_name = tf.name
    tf.close()

    request.urlretrieve (url, temp_file_name)
    # Load the temp file back into a numpy array data
    plydata = PlyData.read(temp_file_name)
    data = np.column_stack([plydata["vertex"]["x"],plydata["vertex"]["y"],plydata["vertex"]["z"]])

    plydata = PlyData.read(temp_file_name)
    data = np.column_stack([plydata["vertex"]["x"],plydata["vertex"]["y"],plydata["vertex"]["z"]])

    # Now, let us only emit trees
    tree_conditional = ((data[:,2] > 110) & (data[:,2] < 112));
    trees = data[tree_conditional,:];
    return trees;
    # @TODO: Remove temp file
```
This function needs a few words: first, we create a temporary file and close it. Then we download the
URL given to this function into this file just to load this file using PlyData.read. Then, we create
a matrix of the vertex data (x,y,z) using NumPy. From this matrix, we select only those rows that
have a suitable height (tree_conditional is a vector of true and false and being used to select the rows).
Those trees are then returned. Note that they are just a numpy matrix local to the machine we are currently
working on. We could remove the temporary file, but as we are going to delete the cloud machines, we
are free not to do it.


```
def key(p):
  # 1024x1024
  x = (p[0] - extent[0])/(extent[1]-extent[0])*1024;
  y = (p[1] - extent[2])/(extent[3]-extent[2])*1024;

  return ("%04d-%04d" % (int(x), int(y)));

def dekey(k):
  x,y = k.split("-");
  return int(x), int(y)
```

Two simple helper functions are needed to do mapreduce. We want to have an image file in the end, so
each coordinate is mapped to a pixel in an 1024x1024 image. Each pixel is given a string name such as
0149-1021. This means that the point will fall in pixel 149,1021 in the end. The function dekey just
inverts this operation by splitting on - and parsing numbers. This is our mapreduce keying scheme. Put differently:
every pixel gets a name and we are able to map a name to a pixel for plotting the results.


Now, we are left with the main program:
```
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
```
Step by step, this program first connects to the Spark cluster (note that you might need to change the first
lines, if you are in a different environment) and gives itself the name trees. For distributed loading,
we apply a simple trick: First, we distribute the array of source files over the cluster, then we map
to each line of the (now) distirbuted dataset the function scanPly. Here is some magic inside: Docker makes
all worker nodes able to use our scanPly function as the spark docker image (and, hence, workers as well as masters) have the library installed.

After loading the files over the Internet, we create our result set by MapReduce. Therefore, we take every point (element in the distirbuted dataset), map it to a pair (key(x),1) just as the famous WordCount example just to reduce (by Key) summing up the second part of every key-value pair. These operations are distributed as they are applied over
a distributed dataset. Now, we are ready to grab the results using collect() into a regular, local python object, which
is then postprocessed by first decoding keys into coordinates and then setting matrix entries from coordinates.
Finally, the matrix is written as a numpy file. Note that we are not visualizing here as this would add even more dependencies. It is now a manual task to get this file out of the cluster...

# Operations

For the cluster, all we need is sp.py on the master as well as docker-compose.yml on the master. Note that
if you want to use the docker-compose.yml locally, you must remove the constraints in the file.