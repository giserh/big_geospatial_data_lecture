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

# Operating the Environment
Even for local development, you need powerful tools that are as near as possible to the cloud deployment. Otherwise,
you might loose a lot of money in debugging weird problems while you already have your cloud machines booked.
Therefore, I will give hints on local development as well as on how to get this up and running as fast
as possible on AWS.

## Local development

For local development, you will need a Linux machine with docker working (e.g., docker run, docker build) and
need to download (and possibly install to /usr/local/bin) docker-compose. The docker-compose file in this
repository specifies, how a (mini) cluster can be run on a virtual local network (based on Docker).

All you need to do (cross your fingers, that everything is well-configured) is
```
docker-compose up
```
This will just create a virtual network and connect two containers to it (master and worker).
This is how it looks at my computer.
```
> docker-compose up
Creating network "06sparkdemo_default" with the default driver
Creating 06sparkdemo_master_1 ... 
Creating 06sparkdemo_master_1 ... done
Creating 06sparkdemo_worker_1 ... 
Creating 06sparkdemo_worker_1 ... done
Attaching to 06sparkdemo_master_1, 06sparkdemo_worker_1
master_1  | 17/06/26 13:56:51 INFO master.Master: Started daemon with process name: 1@master
master_1  | 17/06/26 13:56:51 INFO util.SignalUtils: Registered signal handler for TERM
master_1  | 17/06/26 13:56:51 INFO util.SignalUtils: Registered signal handler for HUP
master_1  | 17/06/26 13:56:51 INFO util.SignalUtils: Registered signal handler for INT
worker_1  | 17/06/26 13:56:51 INFO worker.Worker: Started daemon with process name: 1@4592dc7be79e
master_1  | 17/06/26 13:56:51 WARN util.NativeCodeLoader: Unable to load native-hadoop library for your platform... using builtin-java classes where applicable
worker_1  | 17/06/26 13:56:51 INFO util.SignalUtils: Registered signal handler for TERM
worker_1  | 17/06/26 13:56:51 INFO util.SignalUtils: Registered signal handler for HUP
worker_1  | 17/06/26 13:56:51 INFO util.SignalUtils: Registered signal handler for INT
master_1  | 17/06/26 13:56:51 INFO spark.SecurityManager: Changing view acls to: root
master_1  | 17/06/26 13:56:51 INFO spark.SecurityManager: Changing modify acls to: root
master_1  | 17/06/26 13:56:51 INFO spark.SecurityManager: Changing view acls groups to: 
master_1  | 17/06/26 13:56:51 INFO spark.SecurityManager: Changing modify acls groups to: 
master_1  | 17/06/26 13:56:51 INFO spark.SecurityManager: SecurityManager: authentication disabled; ui acls disabled; users  with view permissions: Set(root); groups with view permissions: Set(); users  with modify permissions: Set(root); groups with modify permissions: Set()
master_1  | 17/06/26 13:56:51 INFO util.Utils: Successfully started service 'sparkMaster' on port 7077.
master_1  | 17/06/26 13:56:52 INFO master.Master: Starting Spark master at spark://master:7077
master_1  | 17/06/26 13:56:52 INFO master.Master: Running Spark version 2.0.0
worker_1  | 17/06/26 13:56:52 WARN util.NativeCodeLoader: Unable to load native-hadoop library for your platform... using builtin-java classes where applicable
master_1  | 17/06/26 13:56:52 INFO util.log: Logging initialized @1456ms
worker_1  | 17/06/26 13:56:52 INFO spark.SecurityManager: Changing view acls to: root
worker_1  | 17/06/26 13:56:52 INFO spark.SecurityManager: Changing modify acls to: root
worker_1  | 17/06/26 13:56:52 INFO spark.SecurityManager: Changing view acls groups to: 
worker_1  | 17/06/26 13:56:52 INFO spark.SecurityManager: Changing modify acls groups to: 
worker_1  | 17/06/26 13:56:52 INFO spark.SecurityManager: SecurityManager: authentication disabled; ui acls disabled; users  with view permissions: Set(root); groups with view permissions: Set(); users  with modify permissions: Set(root); groups with modify permissions: Set()
master_1  | 17/06/26 13:56:52 INFO server.Server: jetty-9.2.z-SNAPSHOT
master_1  | 17/06/26 13:56:52 INFO handler.ContextHandler: Started o.s.j.s.ServletContextHandler@d46b055{/app,null,AVAILABLE}
master_1  | 17/06/26 13:56:52 INFO handler.ContextHandler: Started o.s.j.s.ServletContextHandler@61f36e07{/app/json,null,AVAILABLE}
master_1  | 17/06/26 13:56:52 INFO handler.ContextHandler: Started o.s.j.s.ServletContextHandler@333181b8{/,null,AVAILABLE}
master_1  | 17/06/26 13:56:52 INFO handler.ContextHandler: Started o.s.j.s.ServletContextHandler@46e8405b{/json,null,AVAILABLE}
master_1  | 17/06/26 13:56:52 INFO handler.ContextHandler: Started o.s.j.s.ServletContextHandler@7e1494cb{/static,null,AVAILABLE}
master_1  | 17/06/26 13:56:52 INFO handler.ContextHandler: Started o.s.j.s.ServletContextHandler@4ed4e58{/app/kill,null,AVAILABLE}
master_1  | 17/06/26 13:56:52 INFO handler.ContextHandler: Started o.s.j.s.ServletContextHandler@3bee333c{/driver/kill,null,AVAILABLE}
master_1  | 17/06/26 13:56:52 INFO server.ServerConnector: Started ServerConnector@27f8dba8{HTTP/1.1}{0.0.0.0:8080}
master_1  | 17/06/26 13:56:52 INFO server.Server: Started @1583ms
master_1  | 17/06/26 13:56:52 INFO util.Utils: Successfully started service 'MasterUI' on port 8080.
master_1  | 17/06/26 13:56:52 INFO ui.MasterWebUI: Bound MasterWebUI to 0.0.0.0, and started at http://172.21.0.2:8080
master_1  | 17/06/26 13:56:52 INFO server.Server: jetty-9.2.z-SNAPSHOT
master_1  | 17/06/26 13:56:52 INFO handler.ContextHandler: Started o.s.j.s.ServletContextHandler@40ee17ca{/,null,AVAILABLE}
master_1  | 17/06/26 13:56:52 INFO server.ServerConnector: Started ServerConnector@30604958{HTTP/1.1}{master:6066}
master_1  | 17/06/26 13:56:52 INFO server.Server: Started @1600ms
master_1  | 17/06/26 13:56:52 INFO util.Utils: Successfully started service on port 6066.
master_1  | 17/06/26 13:56:52 INFO rest.StandaloneRestServer: Started REST server for submitting applications on port 6066
worker_1  | 17/06/26 13:56:52 INFO util.Utils: Successfully started service 'sparkWorker' on port 8881.
master_1  | 17/06/26 13:56:52 INFO handler.ContextHandler: Started o.s.j.s.ServletContextHandler@7fe41543{/metrics/master/json,null,AVAILABLE}
master_1  | 17/06/26 13:56:52 INFO handler.ContextHandler: Started o.s.j.s.ServletContextHandler@59ebf207{/metrics/applications/json,null,AVAILABLE}
master_1  | 17/06/26 13:56:52 INFO master.Master: I have been elected leader! New state: ALIVE
worker_1  | 17/06/26 13:56:52 INFO worker.Worker: Starting Spark worker 172.21.0.3:8881 with 2 cores, 1024.0 MB RAM
worker_1  | 17/06/26 13:56:52 INFO worker.Worker: Running Spark version 2.0.0
worker_1  | 17/06/26 13:56:52 INFO worker.Worker: Spark home: /usr/spark-2.0.0
worker_1  | 17/06/26 13:56:52 INFO util.log: Logging initialized @1619ms
worker_1  | 17/06/26 13:56:52 INFO server.Server: jetty-9.2.z-SNAPSHOT
worker_1  | 17/06/26 13:56:52 INFO handler.ContextHandler: Started o.s.j.s.ServletContextHandler@2227ee2e{/logPage,null,AVAILABLE}
worker_1  | 17/06/26 13:56:52 INFO handler.ContextHandler: Started o.s.j.s.ServletContextHandler@612810a4{/logPage/json,null,AVAILABLE}
worker_1  | 17/06/26 13:56:52 INFO handler.ContextHandler: Started o.s.j.s.ServletContextHandler@e7551e2{/,null,AVAILABLE}
worker_1  | 17/06/26 13:56:52 INFO handler.ContextHandler: Started o.s.j.s.ServletContextHandler@7f9caa0c{/json,null,AVAILABLE}
worker_1  | 17/06/26 13:56:52 INFO handler.ContextHandler: Started o.s.j.s.ServletContextHandler@62d76d4f{/static,null,AVAILABLE}
worker_1  | 17/06/26 13:56:52 INFO handler.ContextHandler: Started o.s.j.s.ServletContextHandler@43dbfc81{/log,null,AVAILABLE}
worker_1  | 17/06/26 13:56:52 INFO server.ServerConnector: Started ServerConnector@50a848ed{HTTP/1.1}{0.0.0.0:8081}
worker_1  | 17/06/26 13:56:52 INFO server.Server: Started @1710ms
worker_1  | 17/06/26 13:56:52 INFO util.Utils: Successfully started service 'WorkerUI' on port 8081.
worker_1  | 17/06/26 13:56:52 INFO ui.WorkerWebUI: Bound WorkerWebUI to 0.0.0.0, and started at http://172.21.0.3:8081
worker_1  | 17/06/26 13:56:52 INFO worker.Worker: Connecting to master master:7077...
worker_1  | 17/06/26 13:56:52 INFO handler.ContextHandler: Started o.s.j.s.ServletContextHandler@6085d9e2{/metrics/json,null,AVAILABLE}
worker_1  | 17/06/26 13:56:52 INFO client.TransportClientFactory: Successfully created connection to master/172.21.0.2:7077 after 19 ms (0 ms spent in bootstraps)
master_1  | 17/06/26 13:56:52 INFO master.Master: Registering worker 172.21.0.3:8881 with 2 cores, 1024.0 MB RAM
worker_1  | 17/06/26 13:56:52 INFO worker.Worker: Successfully registered with master spark://master:7077
```
As you see, everything is up and running (however in a virtual network using IPs such as 172.21.0.3). Still,
some of these are exported (look into docker-compose.yml). Hence, go to
<http://localhost:8080> brings up a Spark UI showing the status of your system.

For development, the current directory (I was just running from my local github repository) is mounted into
the container at /data. Let us just examine the container and run a fun application. Therefore, it is easiest
to enter the master container, because then, we don't need to configure anything:
```
martin@gauss9:~/github/big_geospatial_data_lecture/06_spark_demo$ docker ps
CONTAINER ID        IMAGE               COMMAND                  CREATED             STATUS              PORTS                                                                                            NAMES
4592dc7be79e        mwernerds/demo      "bin/spark-class o..."   2 minutes ago       Up 2 minutes        0.0.0.0:8081->8081/tcp                                                                           06sparkdemo_worker_1
530f33ecaf5d        mwernerds/demo      "bin/spark-class o..."   2 minutes ago       Up 2 minutes        0.0.0.0:4040->4040/tcp, 0.0.0.0:6066->6066/tcp, 0.0.0.0:7077->7077/tcp, 0.0.0.0:8080->8080/tcp   06sparkdemo_master_1

martin@gauss9:~/github/big_geospatial_data_lecture/06_spark_demo$ docker exec -it 06sparkdemo_master_1 bash
bash-4.3# cd /data
bash-4.3# ls
Dockerfile  README.md  docker-compose-aws.yml  docker-compose.yml	make_png.py  sp.py
bash-4.3# 
```
Great, we have sp.py directly here and can edit it on the host with our favourite editor. But, we can also
execute it (from within the container, expect it to be very slow (1GB over the Internet + MapReduce). You can
press CTRL-C at any time to stop execution. But, let us just run it:
```
bash-4.3# spark-submit sp.py
17/06/26 14:02:16 INFO spark.SparkContext: Running Spark version 2.0.0
17/06/26 14:02:16 WARN util.NativeCodeLoader: Unable to load native-hadoop library for your platform... using builtin-java classes where applicable
17/06/26 14:02:17 INFO spark.SecurityManager: Changing view acls to: root
17/06/26 14:02:17 INFO spark.SecurityManager: Changing modify acls to: root
17/06/26 14:02:17 INFO spark.SecurityManager: Changing view acls groups to: 
17/06/26 14:02:17 INFO spark.SecurityManager: Changing modify acls groups to: 
17/06/26 14:02:17 INFO spark.SecurityManager: SecurityManager: authentication disabled; ui acls disabled; users  with view permissions: Set(root); groups with view permissions: Set(); users  with modify permissions: Set(root); groups with modify permissions: Set()
17/06/26 14:02:17 INFO util.Utils: Successfully started service 'sparkDriver' on port 44833.
17/06/26 14:02:17 INFO spark.SparkEnv: Registering MapOutputTracker
17/06/26 14:02:17 INFO spark.SparkEnv: Registering BlockManagerMaster
17/06/26 14:02:17 INFO storage.DiskBlockManager: Created local directory at /tmp/blockmgr-65b996c8-be3a-49f6-ba3a-618ae9b7cef0
17/06/26 14:02:17 INFO memory.MemoryStore: MemoryStore started with capacity 366.3 MB
17/06/26 14:02:17 INFO spark.SparkEnv: Registering OutputCommitCoordinator
17/06/26 14:02:17 INFO util.log: Logging initialized @1827ms
17/06/26 14:02:17 INFO server.Server: jetty-9.2.z-SNAPSHOT
17/06/26 14:02:17 INFO handler.ContextHandler: Started o.s.j.s.ServletContextHandler@731f8e55{/jobs,null,AVAILABLE}
17/06/26 14:02:17 INFO handler.ContextHandler: Started o.s.j.s.ServletContextHandler@4f0467e8{/jobs/json,null,AVAILABLE}
17/06/26 14:02:17 INFO handler.ContextHandler: Started o.s.j.s.ServletContextHandler@7ff5f536{/jobs/job,null,AVAILABLE}
17/06/26 14:02:17 INFO handler.ContextHandler: Started o.s.j.s.ServletContextHandler@27ffcfd2{/jobs/job/json,null,AVAILABLE}
17/06/26 14:02:17 INFO handler.ContextHandler: Started o.s.j.s.ServletContextHandler@289d805e{/stages,null,AVAILABLE}
17/06/26 14:02:17 INFO handler.ContextHandler: Started o.s.j.s.ServletContextHandler@44ca9982{/stages/json,null,AVAILABLE}
17/06/26 14:02:17 INFO handler.ContextHandler: Started o.s.j.s.ServletContextHandler@14468868{/stages/stage,null,AVAILABLE}
17/06/26 14:02:17 INFO handler.ContextHandler: Started o.s.j.s.ServletContextHandler@4d1e446d{/stages/stage/json,null,AVAILABLE}
17/06/26 14:02:17 INFO handler.ContextHandler: Started o.s.j.s.ServletContextHandler@945a2e0{/stages/pool,null,AVAILABLE}
17/06/26 14:02:17 INFO handler.ContextHandler: Started o.s.j.s.ServletContextHandler@19533cc3{/stages/pool/json,null,AVAILABLE}
17/06/26 14:02:17 INFO handler.ContextHandler: Started o.s.j.s.ServletContextHandler@39f6e249{/storage,null,AVAILABLE}
17/06/26 14:02:17 INFO handler.ContextHandler: Started o.s.j.s.ServletContextHandler@674f436{/storage/json,null,AVAILABLE}
17/06/26 14:02:17 INFO handler.ContextHandler: Started o.s.j.s.ServletContextHandler@228214ba{/storage/rdd,null,AVAILABLE}
17/06/26 14:02:17 INFO handler.ContextHandler: Started o.s.j.s.ServletContextHandler@2237810d{/storage/rdd/json,null,AVAILABLE}
17/06/26 14:02:17 INFO handler.ContextHandler: Started o.s.j.s.ServletContextHandler@2c5dcba8{/environment,null,AVAILABLE}
17/06/26 14:02:17 INFO handler.ContextHandler: Started o.s.j.s.ServletContextHandler@da94f51{/environment/json,null,AVAILABLE}
17/06/26 14:02:17 INFO handler.ContextHandler: Started o.s.j.s.ServletContextHandler@3fbcbd9a{/executors,null,AVAILABLE}
17/06/26 14:02:17 INFO handler.ContextHandler: Started o.s.j.s.ServletContextHandler@211d7c89{/executors/json,null,AVAILABLE}
17/06/26 14:02:17 INFO handler.ContextHandler: Started o.s.j.s.ServletContextHandler@635ff389{/executors/threadDump,null,AVAILABLE}
17/06/26 14:02:17 INFO handler.ContextHandler: Started o.s.j.s.ServletContextHandler@244beb74{/executors/threadDump/json,null,AVAILABLE}
17/06/26 14:02:17 INFO handler.ContextHandler: Started o.s.j.s.ServletContextHandler@7e41c20a{/static,null,AVAILABLE}
17/06/26 14:02:17 INFO handler.ContextHandler: Started o.s.j.s.ServletContextHandler@3472107f{/,null,AVAILABLE}
17/06/26 14:02:17 INFO handler.ContextHandler: Started o.s.j.s.ServletContextHandler@282d6ec3{/api,null,AVAILABLE}
17/06/26 14:02:17 INFO handler.ContextHandler: Started o.s.j.s.ServletContextHandler@53c620f9{/stages/stage/kill,null,AVAILABLE}
17/06/26 14:02:17 INFO server.ServerConnector: Started ServerConnector@376f1a5d{HTTP/1.1}{0.0.0.0:4040}
17/06/26 14:02:17 INFO server.Server: Started @1926ms
17/06/26 14:02:17 INFO util.Utils: Successfully started service 'SparkUI' on port 4040.
17/06/26 14:02:17 INFO ui.SparkUI: Bound SparkUI to 0.0.0.0, and started at http://172.21.0.2:4040
17/06/26 14:02:17 INFO util.Utils: Copying /data/sp.py to /tmp/spark-f2d4de3e-aa81-4dad-adb5-9d1673ccab51/userFiles-e574a00a-5926-48ec-8e3e-114df1d92880/sp.py
17/06/26 14:02:17 INFO spark.SparkContext: Added file file:/data/sp.py at spark://172.21.0.2:44833/files/sp.py with timestamp 1498485737682
17/06/26 14:02:17 INFO client.StandaloneAppClient$ClientEndpoint: Connecting to master spark://master:7077...
17/06/26 14:02:17 INFO client.TransportClientFactory: Successfully created connection to master/172.21.0.2:7077 after 20 ms (0 ms spent in bootstraps)
17/06/26 14:02:17 INFO cluster.StandaloneSchedulerBackend: Connected to Spark cluster with app ID app-20170626140217-0000
17/06/26 14:02:17 INFO util.Utils: Successfully started service 'org.apache.spark.network.netty.NettyBlockTransferService' on port 36053.
17/06/26 14:02:17 INFO netty.NettyBlockTransferService: Server created on 172.21.0.2:36053
17/06/26 14:02:17 INFO storage.BlockManagerMaster: Registering BlockManager BlockManagerId(driver, 172.21.0.2, 36053)
17/06/26 14:02:17 INFO storage.BlockManagerMasterEndpoint: Registering block manager 172.21.0.2:36053 with 366.3 MB RAM, BlockManagerId(driver, 172.21.0.2, 36053)
17/06/26 14:02:17 INFO storage.BlockManagerMaster: Registered BlockManager BlockManagerId(driver, 172.21.0.2, 36053)
17/06/26 14:02:18 INFO client.StandaloneAppClient$ClientEndpoint: Executor added: app-20170626140217-0000/0 on worker-20170626135652-172.21.0.3-8881 (172.21.0.3:8881) with 2 cores
17/06/26 14:02:18 INFO cluster.StandaloneSchedulerBackend: Granted executor ID app-20170626140217-0000/0 on hostPort 172.21.0.3:8881 with 2 cores, 1024.0 MB RAM
17/06/26 14:02:18 INFO client.StandaloneAppClient$ClientEndpoint: Executor updated: app-20170626140217-0000/0 is now RUNNING
17/06/26 14:02:18 INFO handler.ContextHandler: Started o.s.j.s.ServletContextHandler@78161eba{/metrics/json,null,AVAILABLE}
17/06/26 14:02:18 INFO cluster.StandaloneSchedulerBackend: SchedulerBackend is ready for scheduling beginning after reached minRegisteredResourcesRatio: 0.0
['http://www.martinwerner.de/ply/ffffff3f_ffffff02.ply', 'http://www.martinwerner.de/ply/ffffff3f_ffffff03.ply', 'http://www.martinwerner.de/ply/ffffff3f_ffffff04.ply', 'http://www.martinwerner.de/ply/ffffff3f_ffffff05.ply', 'http://www.martinwerner.de/ply/ffffff3f_ffffff06.ply', 'http://www.martinwerner.de/ply/ffffff40_ffffff02.ply', 'http://www.martinwerner.de/ply/ffffff40_ffffff03.ply', 'http://www.martinwerner.de/ply/ffffff40_ffffff04.ply', 'http://www.martinwerner.de/ply/ffffff40_ffffff05.ply', 'http://www.martinwerner.de/ply/ffffff40_ffffff06.ply', 'http://www.martinwerner.de/ply/ffffff41_ffffff02.ply', 'http://www.martinwerner.de/ply/ffffff41_ffffff03.ply', 'http://www.martinwerner.de/ply/ffffff41_ffffff04.ply', 'http://www.martinwerner.de/ply/ffffff41_ffffff05.ply', 'http://www.martinwerner.de/ply/ffffff41_ffffff06.ply', 'http://www.martinwerner.de/ply/ffffff42_ffffff02.ply', 'http://www.martinwerner.de/ply/ffffff42_ffffff03.ply', 'http://www.martinwerner.de/ply/ffffff42_ffffff04.ply', 'http://www.martinwerner.de/ply/ffffff42_ffffff05.ply', 'http://www.martinwerner.de/ply/ffffff42_ffffff06.ply', 'http://www.martinwerner.de/ply/ffffff43_ffffff03.ply', 'http://www.martinwerner.de/ply/ffffff43_ffffff04.ply', 'http://www.martinwerner.de/ply/ffffff43_ffffff05.ply', 'http://www.martinwerner.de/ply/ffffff43_ffffff06.ply']
Distributed Loading
17/06/26 14:02:18 INFO spark.SparkContext: Starting job: count at /data/sp.py:89
17/06/26 14:02:19 INFO scheduler.DAGScheduler: Got job 0 (count at /data/sp.py:89) with 2 output partitions
17/06/26 14:02:19 INFO scheduler.DAGScheduler: Final stage: ResultStage 0 (count at /data/sp.py:89)
17/06/26 14:02:19 INFO scheduler.DAGScheduler: Parents of final stage: List()
17/06/26 14:02:19 INFO scheduler.DAGScheduler: Missing parents: List()
17/06/26 14:02:19 INFO scheduler.DAGScheduler: Submitting ResultStage 0 (PythonRDD[1] at count at /data/sp.py:89), which has no missing parents
17/06/26 14:02:19 INFO memory.MemoryStore: Block broadcast_0 stored as values in memory (estimated size 5.8 KB, free 366.3 MB)
17/06/26 14:02:19 INFO memory.MemoryStore: Block broadcast_0_piece0 stored as bytes in memory (estimated size 4.0 KB, free 366.3 MB)
17/06/26 14:02:19 INFO storage.BlockManagerInfo: Added broadcast_0_piece0 in memory on 172.21.0.2:36053 (size: 4.0 KB, free: 366.3 MB)
17/06/26 14:02:19 INFO spark.SparkContext: Created broadcast 0 from broadcast at DAGScheduler.scala:1012
17/06/26 14:02:19 INFO scheduler.DAGScheduler: Submitting 2 missing tasks from ResultStage 0 (PythonRDD[1] at count at /data/sp.py:89)
17/06/26 14:02:19 INFO scheduler.TaskSchedulerImpl: Adding task set 0.0 with 2 tasks
17/06/26 14:02:20 INFO cluster.CoarseGrainedSchedulerBackend$DriverEndpoint: Registered executor NettyRpcEndpointRef(null) (172.21.0.3:53650) with ID 0
17/06/26 14:02:20 INFO scheduler.TaskSetManager: Starting task 0.0 in stage 0.0 (TID 0, 172.21.0.3, partition 0, PROCESS_LOCAL, 6128 bytes)
17/06/26 14:02:20 INFO scheduler.TaskSetManager: Starting task 1.0 in stage 0.0 (TID 1, 172.21.0.3, partition 1, PROCESS_LOCAL, 6128 bytes)
17/06/26 14:02:20 INFO cluster.CoarseGrainedSchedulerBackend$DriverEndpoint: Launching task 0 on executor id: 0 hostname: 172.21.0.3.
17/06/26 14:02:20 INFO cluster.CoarseGrainedSchedulerBackend$DriverEndpoint: Launching task 1 on executor id: 0 hostname: 172.21.0.3.
17/06/26 14:02:20 INFO storage.BlockManagerMasterEndpoint: Registering block manager 172.21.0.3:37473 with 366.3 MB RAM, BlockManagerId(0, 172.21.0.3, 37473)
17/06/26 14:02:20 INFO storage.BlockManagerInfo: Added broadcast_0_piece0 in memory on 172.21.0.3:37473 (size: 4.0 KB, free: 366.3 MB)
```
Now the dataset is being downloaded from martinwerner.de. Be patient or take more machines than your laptop...

You will find a line
```
17/06/26 14:15:57 INFO scheduler.TaskSetManager: Finished task 1.0 in stage 0.0 (TID 1) in 157621 ms on 172.21.0.3 (1/2)

```
meaning that the first partiton of the dataset load was complete. However, it takes a long time on a single computer.
So, possibly, you might want to uncomment the small dataset in sp.py 

## Remote execution

For the cluster, all we need is sp.py on the master as well as docker-compose.yml on the master. Note that
if you want to use the docker-compose.yml locally, you must remove the constraints in the file.



## Remote Building

If you are on a slow Internet connection, build all involved Docker stuff remotely on a cheap AWS machine:
```
> docker-machine create --driver amazonec2 aws03
Running pre-create checks...
Creating machine...
(aws03) Launching instance...
Waiting for machine to be running, this may take a few minutes...
Detecting operating system of created instance...
Waiting for SSH to be available...
Detecting the provisioner...
Provisioning with ubuntu(systemd)...
Installing Docker...
Copying certs to the local machine directory...
Copying certs to the remote machine...
>docker-machine ssh aws03
```
Now you are remotely at Amazon (good Internet connection ;-)
```
sudo -s 
wget https://raw.githubusercontent.com/mwernerds/big_geospatial_data_lecture/master/06_spark_demo/Dockerfile
docker build . -t mwernerds/sparkdemo
docker push mwernerds/sparkdemo
```

## A full walkthrough in a video
The following video leads through the full process of development, cloud deployment and cloud execution.

[![Youtube Video](https://img.youtube.com/vi/TDmbSnNeKLk/0.jpg)](https://www.youtube.com/watch?v=TDmbSnNeKLk)
