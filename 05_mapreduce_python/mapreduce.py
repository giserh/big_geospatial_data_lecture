import sys;
from itertools import groupby;

#
# Pyton supports MapReduce paradigm (and more elaborate functional programming)
# by the following functions:
#
# map (function,list) applies the given function over the collection
# filter(function, collection) filters using the given function
# reduce (function, collection) applies reduce successively over the list (slightly different from MapReduce reduce)

# This makes a pure python wordcount mapreduce very easy:


# Now, words is a list of all the words from input.txt. Let us map it

def mapfunc(w):
    # Let us remove all puncuation and spaces.  
    cleanword = ''.join([i for i in w if i.isalpha()])
    return [cleanword,1];

def reducefunc(key, values):
    counts = [x[1] for x in values];
    return [key,sum(counts)];

# MapReduce performs the following:
# Step 1: Read the input and split it for the mapper, here we split already into words
with open("input.txt") as f:
    words=[word for line in f for word in line.split()]
# Map each of those, here we use a mapper that removes all non-alpha characters and emits pairs.
map_result = map (mapfunc, words)
# Sort the mapped results by using the first (e.g. the word)
map_result_sorted = sorted (map_result, key = lambda x: x[0])
# Now apply the reducer for each group of equal keys
reduce_result = [];
for k, g in groupby(map_result_sorted, key = lambda x: x[0]):
    reduce_result.append(reducefunc(k, list(g)))
# print out the results:
print reduce_result
