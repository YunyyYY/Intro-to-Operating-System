# MapReduce
#### [MapReduce: Simplified Data Processing on Large Clusters](<https://static.googleusercontent.com/media/research.google.com/en//archive/mapreduce-osdi04.pdf>)

MapReduce is a paradigm for large-scale parallel data processing introduced by Google. This project is a simplified version of MapReduce for just a single machine. The core problem is to design an efficient  concurrent data structure.

#### Ordering

For each partition, keys (and the value list associated with said keys) are sorted in ascending key order; When a particular reducer thread (and its associated partition) are working, the `Reduce()` function is called on each key in order for that partition.

#### Mechanism

The mechanism is composed of three parts: map, sort, reduce.

For each partition, the data structure is an array of `<key, value>` pairs. During map stage, each insertion operation is $O(1)​$.

In the sort step, [`qsort()`](<https://www.tutorialspoint.com/c_standard_library/c_function_qsort.htm>) is used to sort  the array of  each partition in parallel (in num-partition threads). Complexity is $O(n\log n)$.

In reduce stage, data access is sequential. Each pair is visited in sequence and a temporary aided pointer is used to remember the previous access. Each iteration is $O(1)$.

#### Data Structure

```tex
array of partitions
+------------+
|	     |  ------> |<k1, v1>|<k2, v2>| ... |<kn, vn>|  array of <key, value> pairs
+------------+
|	     |
+------------+
|	     |			For qsort, a comparison is defined for the <key, value> pairs
     ...		in each partition:
|	     |			1. compare keys;
+------------+			2. if keys are the same, compare values.
|	     |
+------------+
|	     |
+------------+
```

