#************************************************************************
#
#        PathFinder: finding a series of labeled nodes within a
#                two-layer directed, cyclic graph.
#               Copyright (2013) Sandia Corporation
#
# Copyright (2013) Sandia Corporation. Under the terms of Contract 
# DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government 
# retains certain rights in this software.
#
# This file is part of PathFinder.
# 
# PathFinder is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as 
# published by the Free Software Foundation, either version 3 of the 
# License, or (at your option) any later version.
#
# PathFinder is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with PathFinder.  If not, see <http://www.gnu.org/licenses/>.
#
# Questions? Contact J. Brian Rigdon (jbrigdo@sandia.gov)

 
import networkx, random, copy, os
#import api

random.seed()

def gen_super_structure(max):
    num_nodes = max
    max_edges = min(10, num_nodes-2)
    nodes = [x * 100000 for x in xrange(num_nodes)]
    g = networkx.DiGraph()
    g.add_nodes_from(nodes)
    added = []
    not_added = set(nodes)
    first = random.choice(list(not_added))
    added.append(first)
    not_added.remove(first)
    while added:
        n = added.pop()
        max_edges = min(max_edges, len(not_added))
        num_edges = random.randint(0,max_edges)
        outgoing = random.sample(not_added, num_edges)
        for e in outgoing:
            g.add_edge(n,e)
            added.append(e)
        not_added.difference_update(outgoing)
    num_random_edges = random.randint(min(5,len(nodes)-1),len(nodes))
    for i in xrange(num_random_edges):
        x = random.choice(list(nodes))
        y = random.choice(list(nodes))
        g.add_edge(x,y)
    return g

def new_gen_super_structure(max):
    num_nodes = max
    max_edges = min(num_nodes/2, num_nodes-2)
    nodes = [x * 100000 for x in xrange(num_nodes)]
    g = networkx.DiGraph()
    g.add_nodes_from(nodes)
    added = []
    not_added = set(nodes)
    first = random.choice(list(not_added))
    added.append(first)
    not_added.remove(first)
    while added:
        n = added.pop()
        max_edges = min(max_edges, len(not_added))
        num_edges = random.randint(0,max_edges)
        outgoing = random.sample(not_added, num_edges)
        for e in outgoing:
            g.add_edge(n,e)
            added.append(e)
        not_added.difference_update(outgoing)
    num_random_edges = random.randint(min(5,len(nodes)-1),len(nodes))
    for i in xrange(num_random_edges):
        x = random.choice(list(nodes))
        y = random.choice(list(nodes))
        g.add_edge(x,y)
    return g


    
def gen_sub_structure(top_node, super):
    g = networkx.DiGraph()
    label = top_node
    stack = [label]
    g.add_node(label)
    forward_edges = []
    min_nodes = len(super.successors(top_node))
    while stack:
        n = stack.pop()
        nodes_with_few_successors = 0
        for node in g.nodes():
            if len(g.successors(node)) < 2 and node not in forward_edges:
                nodes_with_few_successors += 1
        if min_nodes > nodes_with_few_successors:
            need_more_nodes = True
        else:
            need_more_nodes = False
            
        if random.random() > 0.2 or need_more_nodes:
            new_node = label + g.number_of_nodes()
            g.add_node(new_node)
            g.add_edge(n, new_node)
            stack.append(new_node)        
        if random.random() > 0.5: # Add another edge
            if random.random() > 0.5: # New edge is a back edge
                target = random.choice(g.nodes())
                g.add_edge(n, target)                
            else: # New edge is potentially a forward edge
                forward_edges.append(n)
    for e in forward_edges:
        target = random.choice(g.nodes())
        g.add_edge(e, target)
    return g
    
def add_calls(super, subs):
    edges = super.edges()
    new_edges = set()
    picked = set()
        
    for e in edges:
        source = e[0]
        target = e[1]
        n = random.choice(subs[source].nodes())
        while len(subs[source].successors(n)) > 1 or n in picked:
            n = random.choice(subs[source].nodes())
        picked.add(n)
        new_edges.add((n, target))
    return new_edges
    
def add_labels(sub_nodes, num_labels, total_labels):
    labels = []
    label_assignments = {}
    for i in xrange(num_labels):
        labels.append("label_" + str(i))
    if num_labels >= len(sub_nodes):
        num_labels = len(sub_nodes)
    nodes = copy.copy(sub_nodes)
    for l in labels:
        choice = random.choice(list(nodes))
        label_assignments[choice] = l
    remaining_labels = total_labels - num_labels
    for i in xrange(remaining_labels):
        choice = random.choice(list(nodes))
        label_assignments[choice] = random.choice(labels)
    
    return label_assignments
    
def build_graphs(num_nodes, num_labels, total_labels):
    g = new_gen_super_structure(num_nodes)
    subs = {}
    for n in g.nodes():
        subs[n] = gen_sub_structure(n, g)
    calls = add_calls(g, subs)
    basic_blocks = set()
    for n in subs:
        basic_blocks.update(subs[n].nodes())
    labels = add_labels(basic_blocks, num_labels, total_labels)
    return {"super":g, "subs":subs, "calls":calls, "labels":labels}     
    
def build_graph(args, opts):
    """ Plugin: builds an artificial graph resembling a control flow graph.  n is the
                number of functions, labels is the number of unique labels.
        Syntax: build_graph --n=100 --labels=50 --total_labels=100
    """
    if not "n" in opts or not "labels" in opts:
        raise ShellSyntaxError("You must specify n and labels.")
    n = opts["n"]
    labels = opts["labels"]
    total_labels = opts["total_labels"]
    return [build_graphs(n, labels, total_labels)]

def write_graph(args, opts):
    """ Plugin: writes the graph created by build_graph to a file specified by name.
        Syntax: build_graph --n=100 --labels=50 | write_graph --name=myfile
    """
    if not "name" in opts:
        raise ShellSyntaxError("You must specify a file name.")
    name = opts["name"]
    graph = args[0]
    cg = graph["super"]
    cfgs = graph["subs"]
    calls = dict(graph["calls"])
    labels = graph["labels"]
    
    fpath = name  ; # making this a standalone script... os.path.join(api.scratch_dir, name)
    fd = file(fpath, 'wb')
    fd.write("Functions: " + str(cg.number_of_nodes()) + "\n")
    num_bbs = 0
    for bb in cfgs:
        num_bbs += len(cfgs[bb].nodes())
    fd.write("Basic blocks: " + str(num_bbs) + "\n\n")
    
    for n in cg.nodes():
        edges = cg.successors(n)
        s = len(edges)
        outstr = str(n) + " " + str(s) + " "
        for e in edges:
            outstr += str(e) + " "
        fd.write(outstr + "\n")
    fd.write("\n--------------------------------------------\n")
    for cfg in cfgs:
        fd.write(str(cfg) + "\n")
        if not cfgs[cfg]:
            fd.write("\n--------------------------------------------\n")
            continue
        for n in cfgs[cfg].nodes():
            edges = cfgs[cfg].successors(n)
            s = len(edges)
            if n in calls:
                s += 1
            outstr = str(n) + " " + str(s) + " "
            for e in edges:
                outstr += str(e) + " "
            if n in calls:
                outstr += str(calls[n]) + " "
            fd.write(outstr + "\n")
        fd.write("\n--------------------------------------------\n")
    fd.write("SYSTEM CALLS \n\n")
    for s in labels:
        fd.write(str(s) + " " + labels[s] + "\n")
    fd.close()
    return graph


def write_sample_signatures(args, opts):
    """ Plugin: writes the graph created by build_graph to a file specified by name.
        Syntax: build_graph ... | write_graph ... | write_sample_signatures --name=myfile --signatures=100 --longest=20
    """
    if not "signatures" in opts:
        raise ShellSyntaxError("You must specify how many signatures to generate.")
    sigs = opts["signatures"]
    if not "longest" in opts:
        raise ShellSyntaxError("You must specify longest signature possible.")
    longest = opts["longest"]
    if not "name" in opts:
        raise ShellSyntaxError("You must specify a file name.")
    name = opts["name"]
    graph = args[0]
    labels = graph["labels"].values()  ; # we only want the actual labels
    fpath = name  ; # making this a standalone script... os.path.join(api.scratch_dir, name)
    fd = file(fpath, 'wb')
    for i in xrange(sigs):
        signature = ""
        for j in xrange(random.randrange(2,longest+1)):
            signature += random.choice(labels) + " "
        fd.write(signature + "\n")
    fd.close()

# The exports variable is used when this module is loaded into an SNL product
# called "Oxide".
exports = [build_graph, write_graph, write_sample_signatures]

if __name__ == "__main__":
    print "How big should the graph be?:" 
    num_nodes = input("(number of nodes) > ")
    print  "How many labels should there be?:" 
    num_labels = input("(number of labels) > ")
    print  "How many nodes should have labels?:" 
    prompt = "(number bigger than " + str(num_labels) + ") > "
    total_labels = input(prompt)
    graph = build_graphs(num_nodes, num_labels, total_labels)
    print "\n\tGraph has been built\n"
    print "Output directory?:"
    dir = raw_input("(relative or absolute path) > ")
    print "Output filename?:"
    name = raw_input("(.adj_list will be added) > ")
    adj_file_name = dir + "/" + name + ".adj_list"
    args = [graph]
    opts = {}
    opts["name"] = adj_file_name
    write_graph(args, opts)
    print "\n\tAdjacency list file written as " + adj_file_name + ".\n"
    print "Generate random signatures?:"
    sig_yes_no = raw_input("(y/n) > ")
    if sig_yes_no == 'y' or sig_yes_no == 'Y' or sig_yes_no == "yes":
        print "How many signatures do you want?:"
        signatures = input(" > ")
        print "How long should the longest one be?"
        sig_max = input(" > ")
        sig_file_name = dir + "/" + name + ".sig"
        opts["name"] = sig_file_name
        opts["signatures"] = signatures
        opts["longest"] = sig_max
        write_sample_signatures(args,opts)
        print "\n\tSignature file written as " + sig_file_name + ".\n"
    print "...and done!\n\n"
    
    
    