#
# This R sample shall get you started with mapreduce in R
# Note that we follow some naming such that you can actually
# run mapreduce algorithms developed in this environment
# over Hadoop using, for example, rhadoop.
#
#
# Look at the commented lines at the end of this file for a real-world
# MapReduce over Hadoop, which first distributes data to the distributed file sysmte (DFS)
# and then mapreduces using the very same functions.
#
# you might need to run install.packages("plyr") before using this, as the PLYR package is
# used for grouping

library(plyr);




# Step 1: Read lines
lines = readLines(file("input.txt","r"));

# first, remove all non-alphabetic chars (gsub), then split at space
words = strsplit(gsub("[^A-Za-z ]","",lines),' ');
words = as.list(unlist(words))

mapfunc <- function(k)
{
   list(key = k,value=1);
}



# Step 1: Map all words to mappers
mapresult = lapply(words, mapfunc) 
# Step 2: Sort using key
map_result_sorted = mapresult[order(sapply(mapresult,function(x){x$key}))]
# Step 2b: Make a table (using ldply, very slow)
mapresult_df  = data.frame(key = unlist(lapply(map_result_sorted, "[","key")),
	       		  value = unlist(lapply(map_result_sorted, "[","value")))

# Step 3: Apply the reducer over groups
reduce_result = ddply(mapresult_df, .(key), function(x) sum(x$value))
print(head(reduce_result));



# This is, how it works on Hadoop
# compare https://www.r-bloggers.com/from-functional-programming-to-mapreduce-in-r/

#word_count_hadoop <- function(lines) { 
#  words <- to.dfs(do.call(c, strsplit(clean(lines),'\s')))
#  fs.ptr <- mapreduce(input=words,
#    map=function(k,v) keyval(v,1),
#    reduce=function(k,v) keyval(k, sum(v)))
#  raw <- from.dfs(fs.ptr)
#  out <- raw$val
#  names(out) <- raw$key
#  out[order(names(out))]
#} 
