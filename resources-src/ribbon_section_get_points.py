#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import sys
from typing import Sequence, Tuple
import xml.etree.ElementTree as ET


def get_points(filename: str) -> Sequence[Tuple[float, float]]:
    tree = ET.parse(filename)
    root = tree.getroot()
    g = root.find("{http://www.w3.org/2000/svg}g")
    path = g.find("{http://www.w3.org/2000/svg}path")
    data = path.attrib["d"]

    # Tokenize data string
    tokens = []

    cur_tok = ""
    for char in data:
        if not cur_tok:
            cur_tok = char
            continue
        if char in (' ', '-', ',') or (char.isalpha() and not cur_tok[-1].isalpha()) or (
                not char.isalpha() and cur_tok[-1].isalpha()):
            tokens.append(cur_tok)
            cur_tok = char if char not in (' ', ',') else ""
            continue
        cur_tok += char
    else:
        if cur_tok:
            tokens.append(cur_tok)

    # Convert all numbers
    for i in range(len(tokens)):
        try:
            tokens[i] = float(tokens[i])
        except ValueError:
            pass

    # Calculate points
    points = []

    assert tokens.pop(0) == "m"
    initial_point = tokens.pop(0), tokens.pop(0)
    points.append(initial_point)

    while tokens and tokens[0] != "z":
        point = tokens.pop(0) + points[-1][0], tokens.pop(0) + points[-1][1]
        points.append(point)

    if tokens and tokens[0] == "z":
        points.append(points[0])

    # Center the shape around the origin
    xs, ys = zip(*points)
    xs, ys = list(xs), list(ys)

    x_range = min(xs), max(xs)
    y_range = min(ys), max(ys)

    offset = (-((x_range[1] - x_range[0]) / 2 + x_range[0]), -((y_range[1] - y_range[0]) / 2 + y_range[0]))

    translated_points = []
    for point in points:
        translated_points.append((point[0] + offset[0], point[1] + offset[1]))

    return translated_points


if __name__ == "__main__":
    scale = 0.0028
    offset = [0, 0.05]

    points = get_points(sys.argv[1])
    print("{")
    strings = (f"  {{ {point[0] * scale + offset[0]}f, {point[1] * scale + offset[0]}f, 0.0f }}" for point in points)
    print(",\n".join(strings))
    print("};")
