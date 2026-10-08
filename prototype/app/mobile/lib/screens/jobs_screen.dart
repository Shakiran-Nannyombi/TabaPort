import 'dart:async';

import 'package:flutter/material.dart';

import '../api/hub_api.dart';
import '../theme.dart';

class JobsScreen extends StatefulWidget {
  const JobsScreen({super.key, required this.api});
  final HubApi api;

  @override
  State<JobsScreen> createState() => _JobsScreenState();
}

class _JobsScreenState extends State<JobsScreen> {
  String _show = 'active';
  Map<String, dynamic>? _data;
  String? _err;
  Timer? _poll;

  @override
  void initState() {
    super.initState();
    _load();
    _poll = Timer.periodic(const Duration(seconds: 2), (_) => _load());
  }

  @override
  void dispose() {
    _poll?.cancel();
    super.dispose();
  }

  Future<void> _load() async {
    try {
      final d = await widget.api.jobs(show: _show);
      if (mounted) setState(() { _data = d; _err = null; });
    } catch (e) {
      if (mounted) setState(() => _err = e.toString());
    }
  }

  Future<void> _act(int id, String action, [Map<String, dynamic>? body]) async {
    try {
      await widget.api.jobAction(id, action, body);
      await _load();
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text('$e')));
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    final jobs = (_data?['jobs'] as List?) ?? [];
    final crews = ((_data?['crews'] as List?) ?? []).cast<String>();
    final escalate = (_data?['escalate_after_s'] as num?)?.toDouble() ?? 180;

    return Column(
      children: [
        Padding(
          padding: const EdgeInsets.fromLTRB(12, 8, 12, 0),
          child: SegmentedButton<String>(
            segments: const [
              ButtonSegment(value: 'active', label: Text('Active')),
              ButtonSegment(value: 'closed', label: Text('Closed')),
              ButtonSegment(value: 'all', label: Text('All')),
            ],
            selected: {_show},
            onSelectionChanged: (s) {
              setState(() => _show = s.first);
              _load();
            },
          ),
        ),
        if (_err != null)
          Padding(padding: const EdgeInsets.all(16), child: Text(_err!, style: const TextStyle(color: kRed))),
        Expanded(
          child: RefreshIndicator(
            onRefresh: _load,
            child: jobs.isEmpty
                ? ListView(children: const [
                    SizedBox(height: 80),
                    Center(child: Text('No jobs. The drain is behaving.', style: TextStyle(color: Colors.black54))),
                  ])
                : ListView.builder(
                    padding: const EdgeInsets.all(12),
                    itemCount: jobs.length,
                    itemBuilder: (_, i) => _card(Map<String, dynamic>.from(jobs[i] as Map), crews, escalate),
                  ),
          ),
        ),
      ],
    );
  }

  Widget _card(Map<String, dynamic> j, List<String> crews, double escalate) {
    final id = j['job_id'] as int;
    final status = j['status'] as String? ?? 'open';
    final age = (j['age_s'] as num?)?.toDouble() ?? 0;
    final flash = status == 'open' && age >= escalate;
    final st = j['segment_state'] as String? ?? 'unknown';

    return Card(
      color: flash ? const Color(0xFFFFE8E6) : null,
      child: Padding(
        padding: const EdgeInsets.all(12),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Text(status.replaceAll('_', ' ').toUpperCase(),
                    style: TextStyle(fontWeight: FontWeight.w800, color: _statusColor(status))),
                const Spacer(),
                Container(
                  padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 2),
                  decoration: BoxDecoration(color: stateColor(st), borderRadius: BorderRadius.circular(20)),
                  child: Text(st, style: const TextStyle(color: Colors.black, fontWeight: FontWeight.bold, fontSize: 12)),
                ),
              ],
            ),
            const SizedBox(height: 4),
            Text(j['segment_name'] as String? ?? j['segment_id'] as String? ?? '',
                style: const TextStyle(fontSize: 16, fontWeight: FontWeight.w700)),
            Text('→ ${j['action'] ?? ''}', style: const TextStyle(color: Colors.black87)),
            Text('#$id · ${age.round()} s ago · ${j['assigned_crew'] ?? 'no crew yet'}',
                style: const TextStyle(fontSize: 12, color: Colors.black54)),
            if (['open', 'acked', 'en_route', 'on_site'].contains(status)) ...[
              const SizedBox(height: 8),
              Wrap(
                spacing: 8,
                runSpacing: 8,
                children: [
                  DropdownButton<String>(
                    hint: const Text('assign crew'),
                    value: crews.contains(j['assigned_crew']) ? j['assigned_crew'] as String : null,
                    items: crews.map((c) => DropdownMenuItem(value: c, child: Text(c, style: const TextStyle(fontSize: 13)))).toList(),
                    onChanged: (c) { if (c != null) _act(id, 'assign', {'crew': c}); },
                  ),
                  if (status == 'open') FilledButton(onPressed: () => _act(id, 'ack'), child: const Text('Ack')),
                  if (status == 'acked') OutlinedButton(onPressed: () => _act(id, 'en_route'), child: const Text('En route')),
                  if (status == 'en_route') OutlinedButton(onPressed: () => _act(id, 'on_site'), child: const Text('On site')),
                  FilledButton.tonal(onPressed: () => _act(id, 'close'), child: const Text('Close')),
                  TextButton(
                    onPressed: () => _act(id, 'false_alarm', {'reason': 'dispatcher judgement'}),
                    child: const Text('False alarm'),
                  ),
                ],
              ),
            ],
          ],
        ),
      ),
    );
  }

  Color _statusColor(String s) {
    switch (s) {
      case 'open':
        return kRed;
      case 'acked':
        return kYellow;
      case 'closed':
        return kGreen;
      default:
        return kBlue;
    }
  }
}
