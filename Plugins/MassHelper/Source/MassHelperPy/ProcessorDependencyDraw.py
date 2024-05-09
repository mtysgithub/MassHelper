import json
import os
import sys

import matplotlib.pyplot as plt
import networkx as nx
import numpy as np
import scipy as sp
import random

def hierarchy_pos(G, root=None, width=1., vert_gap=0.2, vert_loc=0, xcenter=0.5):
    '''
    From Joel's answer at https://stackoverflow.com/a/29597209/2966723.
    Licensed under Creative Commons Attribution-Share Alike

    If the graph is a tree this will return the positions to plot this in a
    hierarchical layout.

    G: the graph (must be a tree)

    root: the root node of current branch
    - if the tree is directed and this is not given,
      the root will be found and used
    - if the tree is directed and this is given, then
      the positions will be just for the descendants of this node.
    - if the tree is undirected and not given,
      then a random choice will be used.

    width: horizontal space allocated for this branch - avoids overlap with other branches

    vert_gap: gap between levels of hierarchy

    vert_loc: vertical location of root

    xcenter: horizontal location of root
    '''
    if not nx.is_tree(G):
        raise TypeError('cannot use hierarchy_pos on a graph that is not a tree')

    if root is None:
        if isinstance(G, nx.DiGraph):
            root = next(iter(nx.topological_sort(G)))  # allows back compatibility with nx version 1.11
        else:
            root = random.choice(list(G.nodes))

    def _hierarchy_pos(G, root, width=1., vert_gap=0.2, vert_loc=0, xcenter=0.5, pos=None, parent=None):
        '''
        see hierarchy_pos docstring for most arguments

        pos: a dict saying where all nodes go if they have been assigned
        parent: parent of this branch. - only affects it if non-directed

        '''

        if pos is None:
            pos = {root: (xcenter, vert_loc)}
        else:
            pos[root] = (xcenter, vert_loc)
        children = list(G.neighbors(root))
        if not isinstance(G, nx.DiGraph) and parent is not None:
            children.remove(parent)
        if len(children) != 0:
            dx = width / len(children)
            nextx = xcenter - width / 2 - dx / 2
            for child in children:
                nextx += dx
                pos = _hierarchy_pos(G, child, width=dx, vert_gap=vert_gap,
                                     vert_loc=vert_loc - vert_gap, xcenter=nextx,
                                     pos=pos, parent=root)
        return pos

    return _hierarchy_pos(G, root, width, vert_gap, vert_loc, xcenter)

# 运行程序
def main():
    # 检查是否有足够的参数传入
    if len(sys.argv) > 1:
        json_file = sys.argv[1]
        DrawG(os.path.abspath(json_file))
    else:
        directory = "./data"
        json_files = [os.path.abspath(os.path.join(directory, file)) for file in os.listdir(directory) if
                      file.endswith(".json")]

        for json_file in json_files:
            DrawG(json_file)


def DrawG(json_file):
    # 读取JSON数据
    with open(json_file, 'r') as file:
        data = json.load(file)

    # 修改排序函数，现在它考虑了颜色、层级和字典序
    # def node_sort_key(item):
    #     node, color = item
    #     # 将颜色"B"视为更高优先级
    #     color_priority = 0 if color == "B" else 1
    #     parts = node["NodeName"].split('.')
    #     return (color_priority, len(parts), parts)
    #
    # # 构建节点列表，包含节点名称和颜色
    # nodes_with_color = [(node, node["Color"]) for node in data["Nodes"]]
    # # 按照新的排序规则排序
    # sorted_nodes = sorted(nodes_with_color, key=node_sort_key)

    # 创建有向图
    G = nx.DiGraph()

    actual_nodes_colors = []
    actual_edge_colors = []
    node_name_map = {}
    node_in_degree_map = {}
    node_nickname_labels = {}

    for node in data["Nodes"]:
        node_name_map[node['NodeName']] = node

    # 按照排序后的顺序添加节点（只需节点名称）
    for node in data["Nodes"]:
        if node['Color'] == 'B':
            G.add_node(node["NodeName"], color='blue')
            actual_edge_colors.append('blue')
            for sub_node in node["SubNodeIndices"]:
                if node_name_map[sub_node]['Color'] == 'B':
                    G.add_node(sub_node, color='blue')
                    actual_edge_colors.append('blue')

                    sub_node_nickname = sub_node.split(".")[-1]
                    node_nickname_labels[sub_node] = sub_node_nickname

                else:
                    G.add_node(sub_node, color='red')
                    actual_edge_colors.append('red')
                G.add_edge(node["NodeName"], sub_node)
                if sub_node in node_in_degree_map:
                    node_in_degree_map[sub_node] += 1
                else:
                    node_in_degree_map[sub_node] = 1
        else:
            G.add_node(node["NodeName"], color='red')
            node_nickname_labels[node["NodeName"]] = node["NodeName"]

    #添加超节点
    G.add_node("super", color='black')
    for node in data["Nodes"]:
        if not (node['NodeName'] in node_in_degree_map) or (node_in_degree_map[node['NodeName']] == 0):
            G.add_edge("super", node['NodeName'], color='black')

    # 添加时序依赖
    # for node in data["Nodes"]:
    #     for after_node in node["ExecuteAfterNodes"]:
    #         G.add_edge(node["NodeName"], after_node)
    #     for before_node in node["ExecuteBeforeNodes"]:
    #         G.add_edge(before_node, node["NodeName"])

    # 设置图形大小
    plt.figure(figsize=(350, 350))

    # 染色
    for node_key in G.nodes():
        actual_nodes_colors.append(G.nodes[node_key]['color'])

    if not nx.is_tree(G):
    # if True:
        # pos = nx.kamada_kawai_layout(G)  # 使用kamada_kawai_layout布局
        pos = nx.spring_layout(G, k=0.55, iterations=50)
        nx.draw(G, pos, with_labels=True, labels=node_nickname_labels, node_color=actual_nodes_colors, edge_color=actual_edge_colors, node_size=50000,
                font_weight='bold', arrows=True,
                arrowstyle='->', arrowsize=10, font_size=20, alpha=0.7)
    else:
        pos = hierarchy_pos(G, "super")
        nx.draw(G, pos=pos, with_labels=True, labels=node_nickname_labels)

    # 显示图形
    # plt.show()

    file_name_no_extension = os.path.splitext(os.path.basename(json_file))[0]
    plt.savefig("./data/{0}.jpg".format(file_name_no_extension))


if __name__ == "__main__":
    main()
