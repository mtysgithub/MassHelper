import json
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
        JsonFilePath = sys.argv[1]

        # test
        # JsonFilePath = "./data/tmpProcessorDependencyDraw.json"

        # 读取JSON数据
        with open(JsonFilePath, 'r') as file:
            data = json.load(file)

        # 修改排序函数，现在它考虑了颜色、层级和字典序
        def node_sort_key(item):
            node_name, color = item
            # 将颜色"B"视为更高优先级
            color_priority = 0 if color == "B" else 1
            parts = node_name.split('.')
            return (color_priority, len(parts), parts)

        # 构建节点列表，包含节点名称和颜色
        nodes_with_color = [(node["NodeName"], node["Color"]) for node in data["Nodes"]]

        # 按照新的排序规则排序
        sorted_nodes = sorted(nodes_with_color, key=node_sort_key)

        # 创建有向图
        G = nx.DiGraph()

        # 按照排序后的顺序添加节点（只需节点名称）
        for node_name, _ in sorted_nodes:
            G.add_node(node_name, color=_)

        # 添加时序依赖
        # for node in data["Nodes"]:
        #     for after_node in node["ExecuteAfterNodes"]:
        #         G.add_edge(after_node, node["NodeName"])
        #     for before_node in node["ExecuteBeforeNodes"]:
        #         G.add_edge(node["NodeName"], before_node)

        for node in data["Nodes"]:
            for parent_node in node["OriginalDependencies"]:
                G.add_edge(parent_node, node["NodeName"])
            for sub_node in node["SubNodeIndices"]:
                G.add_edge(node["NodeName"], sub_node)

        # 绘制图形
        colors = [G.nodes[n]['color'] for n in G.nodes()]  # 根据节点属性获取颜色
        # 为了图形美观，这里对颜色进行转换：R -> red, B -> blue
        color_map = {'R': 'red', 'B': 'blue'}
        actual_colors = [color_map[color] for color in colors]

        # 设置图形大小
        plt.figure(figsize=(350, 350))

        # 创建并绘制图形
        # pos = nx.kamada_kawai_layout(G)  # 使用kamada_kawai_layout布局

        pos = nx.spring_layout(G, k=0.55, iterations=50)

        nx.draw(G, pos, with_labels=True, node_color=actual_colors, node_size=50000, font_weight='bold', arrows=True,
                arrowstyle='->', arrowsize=10, font_size=20, alpha=0.7)

        # pos = hierarchy_pos(G, 1)
        # nx.draw(G, pos=pos, with_labels=True)

        # 显示图形
        # plt.show()
        plt.savefig("ret.jpg")


if __name__ == "__main__":
    main()
