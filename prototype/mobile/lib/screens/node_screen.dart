import 'package:flutter/material.dart';

import '../api/hub_api.dart';
import '../theme.dart';

class NodeScreen extends StatelessWidget {
  const NodeScreen({super.key, required this.api, required this.state});
  final HubApi api;
  final Map<String, dynamic>? state;

  @override
  Widget build(BuildContext context) {
    final nodes = Map<String, dynamic>.from(state?['nodes'] as Map? ?? {});
    final n = Map<String, dynamic>.from(nodes['drain_01'] as Map? ?? {});
    final segs = (state?['segments'] as List? ?? []);
    final seg = segs.cast<dynamic>().firstWhere(
          (s) => s['node_id'] == 'drain_01',
          orElse: () => {'state': 'unknown'},
        ) as Map;
    final st = seg['state'] as String? ?? 'unknown';
    final serial = Map<String, dynamic>.from(state?['serial'] as Map? ?? {});
    final live = n.isNotEmpty && n['silent'] != true && n['fault'] != true && st != 'unknown';

    return ListView(
      padding: const EdgeInsets.all(16),
      children: [
        Center(
          child: Container(
            padding: const EdgeInsets.symmetric(horizontal: 28, vertical: 12),
            decoration: BoxDecoration(color: stateColor(st), borderRadius: BorderRadius.circular(40)),
            child: Text(st.toUpperCase(),
                style: const TextStyle(fontSize: 28, fontWeight: FontWeight.w800, color: Colors.black)),
          ),
        ),
        const SizedBox(height: 20),
        Text(live ? '${(n['level_cm'] as num).toStringAsFixed(1)} cm' : '–',
            textAlign: TextAlign.center,
            style: const TextStyle(fontSize: 44, fontWeight: FontWeight.w800)),
        const Text('water depth', textAlign: TextAlign.center, style: TextStyle(color: Colors.black54)),
        const SizedBox(height: 20),
        _kv('below road edge', live ? '${n['below_road_cm']} cm' : '–'),
        _kv('rise rate', n['rise_rate'] == null ? '–' : '${n['rise_rate']} cm/min'),
        _kv('raw echo', live ? '${n['raw_cm']} cm' : '–'),
        _kv('filtered spike', '${n['filtered'] == true}'),
        _kv('hub serial', serial['connected'] == true ? '${serial['url']}' : 'disconnected'),
        _kv('link mode', '${serial['mode'] ?? '–'}'),
        const SizedBox(height: 12),
        const Text(
          'The phone does not talk to the ESP32. USB stays on the laptop. '
          'This screen is the same live node the dispatcher sees, over Wi-Fi.',
          style: TextStyle(color: Colors.black54, fontSize: 13),
        ),
      ],
    );
  }

  Widget _kv(String k, String v) => Padding(
        padding: const EdgeInsets.symmetric(vertical: 6),
        child: Row(
          children: [
            Expanded(child: Text(k, style: const TextStyle(color: Colors.black54))),
            Text(v, style: const TextStyle(fontWeight: FontWeight.w700)),
          ],
        ),
      );
}
