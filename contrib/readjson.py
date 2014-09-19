#!/usr/bin/env python
#
# Copyright (c) 2014 by California Institute of Technology
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# 3. Neither the name of the California Institute of Technology nor
#    the names of its contributors may be used to endorse or promote
#    products derived from this software without specific prior
#    written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL CALTECH OR THE
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
"""Read JSON output from gr1c and build a NetworkX DiGraph

Reads from file with name given as command-line argument, or if no
name given, then read from stdin.  E.g., try

  $ ./gr1c -t json examples/counter.spc | ./readjson.py


SCL; 19 Sep 2014
"""

import sys
import json
import networkx as nx
import matplotlib.pyplot as plt


if __name__ == "__main__":
    if len(sys.argv) < 2:
        json_aut = json.load(sys.stdin)
    else:
        json_aut = json.load(open(sys.argv[1]))

    assert json_aut["version"] == 1

    G = nx.DiGraph()
    for node_ID in json_aut["nodes"].iterkeys():
        node_label = dict([(k, json_aut["nodes"][node_ID][k]) \
                           for k in ("state", "initial", "mode", "rgrad")])
        G.add_node(node_ID, node_label)
    for node_id in json_aut["nodes"].iterkeys():
        for to_node in json_aut["nodes"][node_id]["trans"]:
            G.add_edge(node_id, to_node)

    for (node_ID, node_d) in G.nodes_iter(data=True):
        print(node_ID, node_d)

    nx.draw(G)
    plt.show()
