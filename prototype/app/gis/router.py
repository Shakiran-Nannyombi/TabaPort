"""Offline GIS helpers for the hub. Adapted from Trevor's TabaPort_GIS_Code/tabaport_gis_offline.py.

1. Gazetteer matching for free-text reports (100 m uncertainty until confirmed).
2. Stage elevation and head difference (Eric's H_up, H_down, dH).
3. Flood-aware A* routing over the GeoJSON road segments:
   cost = length x factor, green 1, unknown 2, yellow 4, red removed from the graph.
"""
import difflib
import json
import math
import os
import re

import networkx as nx

HERE = os.path.dirname(os.path.abspath(__file__))
DATA = os.path.join(os.path.dirname(HERE), "data")

STATE_FACTOR = {"green": 1.0, "unknown": 2.0, "yellow": 4.0, "red": math.inf}


def load_json(name):
    with open(os.path.join(DATA, name), encoding="utf-8") as f:
        return json.load(f)


# ---------- 1. Gazetteer --------------------------------------------------
def _norm(t):
    return re.sub(r"[^a-z ]", " ", t.lower()).split()


def match_report(text, cutoff=0.82, gazetteer=None):
    """Return best gazetteer place for free text, or None. Uses 1 to 3 word n-grams."""
    gz = gazetteer or load_json("gazetteer.json")
    words = _norm(text)
    grams = {" ".join(words[i:i + n]) for n in (1, 2, 3) for i in range(len(words) - n + 1)}
    best, best_score = None, 0.0
    for p in gz["places"]:
        for label in [p["name"].lower()] + [a.lower() for a in p["aliases"]]:
            for g in grams:
                s = difflib.SequenceMatcher(None, g, label).ratio()
                if s > best_score:
                    best, best_score = p, s
    if best is None or best_score < cutoff:
        return None
    return {"place_id": best["place_id"], "name": best["name"], "score": round(best_score, 2),
            "lat": best["lat"], "lon": best["lon"], "radius_m": gz.get("uncertainty_m", 100),
            "candidate_segments": best["segment_ids"]}


# ---------- 2. Stage elevation and head difference -------------------------
def stage_elevation(sensor_face_elev_m, measured_distance_cm):
    """Water surface elevation (m above datum) = surveyed sensor face elevation - measured air gap."""
    return round(sensor_face_elev_m - measured_distance_cm / 100.0, 3)


def head_difference(wse_upstream_m, wse_downstream_m):
    """Eric's dH: a growing positive value at a crossing suggests blockage or restriction."""
    return round(wse_upstream_m - wse_downstream_m, 3)


# ---------- 3. Flood-aware routing ----------------------------------------
def haversine_m(a, b):
    R = 6371000.0
    la1, lo1, la2, lo2 = map(math.radians, (a[1], a[0], b[1], b[0]))
    h = math.sin((la2 - la1) / 2) ** 2 + math.cos(la1) * math.cos(la2) * math.sin((lo2 - lo1) / 2) ** 2
    return 2 * R * math.asin(math.sqrt(h))


def seg_length_m(coords):
    return sum(haversine_m(coords[i], coords[i + 1]) for i in range(len(coords) - 1))


def build_graph(segments_geojson, states):
    """states: {segment_id: 'green'|'yellow'|'red'|'unknown'} from road_segment_state."""
    G = nx.Graph()
    for f in segments_geojson["features"]:
        sid = f["properties"]["segment_id"]
        coords = f["geometry"]["coordinates"]
        u, v = tuple(coords[0]), tuple(coords[-1])
        length = seg_length_m(coords)
        factor = STATE_FACTOR[states.get(sid, "unknown")]
        if math.isinf(factor):
            continue  # red segment removed
        G.add_edge(u, v, segment_id=sid, length_m=length, cost=length * factor, coords=coords)
    return G


def safe_route(G, start, goal):
    path = nx.astar_path(G, start, goal, heuristic=haversine_m, weight="cost")
    segs = [G.edges[path[i], path[i + 1]]["segment_id"] for i in range(len(path) - 1)]
    dist = sum(G.edges[path[i], path[i + 1]]["length_m"] for i in range(len(path) - 1))
    return segs, round(dist), path


def segment_endpoints(segments_geojson, segment_id):
    for f in segments_geojson["features"]:
        if f["properties"]["segment_id"] == segment_id:
            c = f["geometry"]["coordinates"]
            return tuple(c[0]), tuple(c[-1])
    return None


def route_between_segments(segments_geojson, states, from_segment, to_segment):
    """Route from the start of one segment to the end of another, avoiding red. Returns dict or error."""
    a = segment_endpoints(segments_geojson, from_segment)
    b = segment_endpoints(segments_geojson, to_segment)
    if not a or not b:
        return {"error": "unknown segment"}
    # the target segment itself must be passable
    states = dict(states)
    for sid in (from_segment, to_segment):
        if states.get(sid) == "red":
            return {"error": f"{sid} is red; crew cannot enter it. Route to the nearest green segment instead."}
    G = build_graph(segments_geojson, states)
    try:
        segs, dist, path = safe_route(G, a[0], b[-1])
    except (nx.NetworkXNoPath, nx.NodeNotFound):
        return {"error": "no passable route; all options are red"}
    return {"segments": segs, "distance_m": dist, "path": [list(p) for p in path]}


if __name__ == "__main__":
    for msg in ["Flooding at kalerwe mkt road closed", "BWAYISE water too much", "clocktower submerged", "all fine in Entebbe"]:
        print(f"  '{msg}' -> {match_report(msg)}")
    segs = load_json("road_segments.geojson")
    print(route_between_segments(segs, {}, "campus_drain_01", "bwaise_01"))
    print(route_between_segments(segs, {"kubiri_01": "red", "mulago_rbt_01": "green", "mulago_kubiri_01": "green"}, "campus_drain_01", "bwaise_01"))
