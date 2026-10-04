"""TabaPort GIS offline toolkit (runs on the dispatcher laptop, no internet).
1. Gazetteer matching for boda USSD/SMS reports (IEEE #5267: 'regex tokenizer + gazetteer').
2. Stage elevation and head difference from sensor readings (Eric's H_up, H_down, dH).
3. Flood-aware A* routing over pre-computed GeoJSON road segments.
Dependencies: Python 3 standard library + networkx."""
import json, math, re, difflib
import networkx as nx

# ---------- 1. Gazetteer --------------------------------------------------
GAZETTEER = [
    {"place_id": "TP-01", "name": "Clock Tower", "aliases": ["clocktower", "clock tower junction", "queensway"], "lat": 0.3095, "lon": 32.5785, "segment_ids": ["clock_tower_01", "clock_tower_02"]},
    {"place_id": "TP-02", "name": "Bwaise", "aliases": ["bwayise", "bwaise market", "bwaise roundabout"], "lat": 0.3500, "lon": 32.5611, "segment_ids": ["bwaise_01"]},
    {"place_id": "TP-03", "name": "Kalerwe", "aliases": ["kalerwe market", "kalerwe mkt", "kaleerwe"], "lat": 0.3525, "lon": 32.5720, "segment_ids": ["kalerwe_01"]},
    {"place_id": "TP-04", "name": "Mulago roundabout", "aliases": ["mulago", "mulago round about"], "lat": 0.3395, "lon": 32.5745, "segment_ids": ["mulago_rbt_01"]},
    {"place_id": "TP-11", "name": "Nalukolongo", "aliases": ["nalukolongo channel", "nalukolongo bridge"], "lat": 0.2960, "lon": 32.5490, "segment_ids": ["nalukolongo_01"]},
    {"place_id": "TP-10", "name": "Nateete", "aliases": ["natete", "family loaf"], "lat": 0.2983, "lon": 32.5350, "segment_ids": ["nateete_01"]},
]
UNCERTAINTY_M = 100  # IEEE #5267: text reports carry a 100 m radius until confirmed

def _norm(t):
    return re.sub(r"[^a-z ]", " ", t.lower()).split()

def match_report(text, cutoff=0.82):
    """Return best gazetteer place for free text, or None. Uses 1 to 3 word n-grams."""
    words = _norm(text)
    grams = {" ".join(words[i:i + n]) for n in (1, 2, 3) for i in range(len(words) - n + 1)}
    best, best_score = None, 0.0
    for p in GAZETTEER:
        for label in [p["name"].lower()] + p["aliases"]:
            for g in grams:
                s = difflib.SequenceMatcher(None, g, label).ratio()
                if s > best_score:
                    best, best_score = p, s
    if best_score < cutoff:
        return None
    return {"place_id": best["place_id"], "name": best["name"], "score": round(best_score, 2),
            "lat": best["lat"], "lon": best["lon"], "radius_m": UNCERTAINTY_M,
            "candidate_segments": best["segment_ids"]}

# ---------- 2. Stage elevation and head difference -------------------------
def stage_elevation(sensor_face_elev_m, measured_distance_cm):
    """Water surface elevation (m above datum) = surveyed sensor face elevation - measured air gap."""
    return round(sensor_face_elev_m - measured_distance_cm / 100.0, 3)

def head_difference(wse_upstream_m, wse_downstream_m):
    """Eric's dH: a growing positive value at a crossing suggests blockage or restriction."""
    return round(wse_upstream_m - wse_downstream_m, 3)

# ---------- 3. Flood-aware routing ----------------------------------------
STATE_FACTOR = {"green": 1.0, "unknown": 2.0, "yellow": 4.0, "red": math.inf}

def haversine_m(a, b):
    R = 6371000.0
    la1, lo1, la2, lo2 = map(math.radians, (a[1], a[0], b[1], b[0]))
    h = math.sin((la2 - la1) / 2) ** 2 + math.cos(la1) * math.cos(la2) * math.sin((lo2 - lo1) / 2) ** 2
    return 2 * R * math.asin(math.sqrt(h))

def build_graph(segments_geojson, states):
    """segments_geojson: FeatureCollection of LineStrings with properties.segment_id.
    states: {segment_id: 'green'|'yellow'|'red'|'unknown'} from SQLite road_segment_state."""
    G = nx.Graph()
    for f in segments_geojson["features"]:
        sid = f["properties"]["segment_id"]
        coords = f["geometry"]["coordinates"]
        u, v = tuple(coords[0]), tuple(coords[-1])
        length = sum(haversine_m(coords[i], coords[i + 1]) for i in range(len(coords) - 1))
        factor = STATE_FACTOR[states.get(sid, "unknown")]
        if math.isinf(factor):
            continue  # red segment: infinite penalty, removed from the graph
        G.add_edge(u, v, segment_id=sid, length_m=length, cost=length * factor)
    return G

def safe_route(G, start, goal):
    path = nx.astar_path(G, start, goal, heuristic=haversine_m, weight="cost")
    segs = [G.edges[path[i], path[i + 1]]["segment_id"] for i in range(len(path) - 1)]
    dist = sum(G.edges[path[i], path[i + 1]]["length_m"] for i in range(len(path) - 1))
    return segs, round(dist)

if __name__ == "__main__":
    print("GAZETTEER TESTS")
    for msg in ["Flooding at kalerwe mkt road closed", "BWAYISE water too much",
                "clocktower submerged", "natete near family loaf blocked drain", "all fine in Entebbe"]:
        print(f"  '{msg}' -> {match_report(msg)}")
    print("\nSTAGE AND HEAD DIFFERENCE TEST")
    up = stage_elevation(1181.450, 62.0)
    down = stage_elevation(1181.120, 71.5)
    print(f"  WSE up={up} m, WSE down={down} m, dH={head_difference(up, down)} m")
    print("\nROUTING TEST (demo network, not surveyed geometry)")
    A, B, C, D = (32.570, 0.340), (32.575, 0.345), (32.572, 0.350), (32.566, 0.347)
    demo = {"type": "FeatureCollection", "features": [
        {"type": "Feature", "properties": {"segment_id": "demo_direct"}, "geometry": {"type": "LineString", "coordinates": [A, C]}},
        {"type": "Feature", "properties": {"segment_id": "demo_east_1"}, "geometry": {"type": "LineString", "coordinates": [A, B]}},
        {"type": "Feature", "properties": {"segment_id": "demo_east_2"}, "geometry": {"type": "LineString", "coordinates": [B, C]}},
        {"type": "Feature", "properties": {"segment_id": "demo_west_1"}, "geometry": {"type": "LineString", "coordinates": [A, D]}},
        {"type": "Feature", "properties": {"segment_id": "demo_west_2"}, "geometry": {"type": "LineString", "coordinates": [D, C]}}]}
    for states in [{}, {"demo_direct": "green"}, {"demo_direct": "red", "demo_east_1": "yellow",
                   "demo_east_2": "green", "demo_west_1": "green", "demo_west_2": "green"}]:
        G = build_graph(demo, states)
        print(f"  states={states or 'all unknown'} -> {safe_route(G, A, C)}")
