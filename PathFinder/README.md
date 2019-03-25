# PathFinder Graph Search Mini-App

PathFinder searches for "signatures" within graphs. Graphs being searched are  directed and cyclic. Many, but not all nodes within the graph have labels. Any given node may have more than one label, and any label may be applied to more than one node. A signature is an orderd list of labels. PathFinder searches for paths between labels within the signature. PathFinder returns success if there is a path from a node with the first label in the signature that passes through nodes with each label in order, ultimately reaching a node with the last label in the signature. Labeled nodes need not be contiguous on any given path. 

At the current time, PathFinder does not do any pathway analysis (e.g. shortest path) for discovered signatures. PathFinder simply searches until a signature is satisfied or all pathways have been exhausted.
