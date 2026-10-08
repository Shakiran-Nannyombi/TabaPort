import 'package:flutter/material.dart';
import 'package:flutter_map/flutter_map.dart';
import 'package:latlong2/latlong.dart';

import '../api/hub_api.dart';
import '../theme.dart';

class MapScreen extends StatefulWidget {
  const MapScreen({super.key, required this.api, required this.state});
  final HubApi api;
  final Map<String, dynamic>? state;

  @override
  State<MapScreen> createState() => _MapScreenState();
}

class _MapScreenState extends State<MapScreen> {
  Map<String, dynamic>? _segs;
  final _report = TextEditingController();
  final _map = MapController();
  String _note = '';
  bool _mapReady = false;

  static final _kampala = LatLng(0.330, 32.575);

  @override
  void initState() {
    super.initState();
    widget.api.geojson('road_segments.geojson').then((g) {
      if (!mounted) return;
      setState(() => _segs = g);
      _fit();
    }).catchError((_) {});
  }

  @override
  void dispose() {
    _report.dispose();
    super.dispose();
  }

  void _fit() {
    if (!_mapReady || _segs == null) return;
    final pts = <LatLng>[];
    for (final f in (_segs?['features'] as List?) ?? []) {
      for (final c in f['geometry']['coordinates'] as List) {
        pts.add(LatLng((c[1] as num).toDouble(), (c[0] as num).toDouble()));
      }
    }
    if (pts.length < 2) return;
    _map.fitCamera(CameraFit.bounds(bounds: LatLngBounds.fromPoints(pts), padding: const EdgeInsets.all(28)));
  }

  Map<String, String> get _states {
    final out = <String, String>{};
    for (final s in (widget.state?['segments'] as List? ?? [])) {
      out[s['segment_id'] as String] = s['state'] as String? ?? 'unknown';
    }
    return out;
  }

  List<Marker> get _markers {
    final features = (_segs?['features'] as List?) ?? [];
    final nodes = Map<String, dynamic>.from(widget.state?['nodes'] as Map? ?? {});
    final markers = <Marker>[];
    for (final f in features) {
      final props = Map<String, dynamic>.from(f['properties'] as Map);
      final nodeId = props['node_id'];
      if (nodeId == null) continue;
      final coords = f['geometry']['coordinates'] as List;
      final mid = coords[coords.length ~/ 2] as List;
      final n = Map<String, dynamic>.from(nodes[nodeId] as Map? ?? {});
      final st = n['state'] as String? ?? _states[props['segment_id'] as String] ?? 'unknown';
      final level = n['level_cm'];
      final label = level == null ? '$nodeId · $st' : '$nodeId · $st · ${(level as num).toStringAsFixed(1)} cm';
      markers.add(Marker(
        point: LatLng((mid[1] as num).toDouble(), (mid[0] as num).toDouble()),
        width: 22,
        height: 22,
        child: Tooltip(
          message: label,
          child: Container(
            decoration: BoxDecoration(
              color: stateColor(st),
              shape: BoxShape.circle,
              border: Border.all(color: const Color(0xFF1A2330), width: 2),
            ),
          ),
        ),
      ));
    }
    return markers;
  }

  List<Polyline> get _lines {
    final features = (_segs?['features'] as List?) ?? [];
    final lines = <Polyline>[];
    for (final f in features) {
      final props = Map<String, dynamic>.from(f['properties'] as Map);
      final id = props['segment_id'] as String;
      final coords = (f['geometry']['coordinates'] as List)
          .map((c) => LatLng((c[1] as num).toDouble(), (c[0] as num).toDouble()))
          .toList();
      final st = _states[id] ?? 'unknown';
      lines.add(Polyline(
        points: coords,
        color: stateColor(st),
        strokeWidth: st == 'red' ? 8 : 6,
        borderColor: const Color(0xFF1A2330),
        borderStrokeWidth: 2,
        pattern: st == 'unknown' ? const StrokePattern.dotted() : const StrokePattern.solid(),
      ));
    }
    return lines;
  }

  @override
  Widget build(BuildContext context) {
    return Column(
      children: [
        Expanded(
          child: FlutterMap(
            mapController: _map,
            options: MapOptions(
              initialCenter: _kampala,
              initialZoom: 12.2,
              onMapReady: () {
                _mapReady = true;
                _fit();
              },
            ),
            children: [
              TileLayer(
                urlTemplate: 'https://tile.openstreetmap.org/{z}/{x}/{y}.png',
                userAgentPackageName: 'ug.tabaport.tabaport_mobile',
              ),
              PolylineLayer(polylines: _lines),
              MarkerLayer(markers: _markers),
              const RichAttributionWidget(
                attributions: [
                  TextSourceAttribution('OpenStreetMap contributors'),
                ],
              ),
            ],
          ),
        ),
        Container(
          color: Colors.white,
          padding: const EdgeInsets.all(12),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.stretch,
            children: [
              Wrap(
                spacing: 10,
                children: [
                  _chip('green', kGreen),
                  _chip('yellow', kYellow),
                  _chip('red', kRed),
                  _chip('unknown', kUnknown),
                ],
              ),
              const SizedBox(height: 8),
              Row(
                children: [
                  Expanded(
                    child: TextField(
                      controller: _report,
                      decoration: const InputDecoration(
                        hintText: 'Field report e.g. Kalerwe mkt flooded',
                        isDense: true,
                      ),
                    ),
                  ),
                  const SizedBox(width: 8),
                  FilledButton(
                    onPressed: () async {
                      final t = _report.text.trim();
                      if (t.isEmpty) return;
                      try {
                        await widget.api.report(text: t);
                        _report.clear();
                        setState(() => _note = 'logged');
                      } catch (e) {
                        setState(() => _note = e.toString());
                      }
                    },
                    child: const Text('Log'),
                  ),
                ],
              ),
              if (_note.isNotEmpty)
                Padding(
                  padding: const EdgeInsets.only(top: 6),
                  child: Text(_note, style: const TextStyle(color: Colors.black54, fontSize: 12)),
                ),
            ],
          ),
        ),
      ],
    );
  }

  Widget _chip(String label, Color c) => Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          Container(width: 18, height: 4, color: c),
          const SizedBox(width: 4),
          Text(label, style: const TextStyle(fontSize: 12, color: Colors.black54)),
        ],
      );
}
