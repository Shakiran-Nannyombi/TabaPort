import 'package:flutter/material.dart';

import '../api/hub_api.dart';
import '../hub_config.dart';

class SettingsScreen extends StatefulWidget {
  const SettingsScreen({
    super.key,
    required this.api,
    required this.hubUrl,
    required this.onHubUrl,
  });
  final HubApi api;
  final String hubUrl;
  final Future<void> Function(String url) onHubUrl;

  @override
  State<SettingsScreen> createState() => _SettingsScreenState();
}

class _SettingsScreenState extends State<SettingsScreen> {
  late final TextEditingController _url;
  final _depth = TextEditingController(text: '60');
  final _sensor = TextEditingController(text: '50');
  String _msg = '';

  @override
  void initState() {
    super.initState();
    _url = TextEditingController(text: widget.hubUrl);
    _loadCalibration();
  }

  Future<void> _loadCalibration() async {
    try {
      final raw = await widget.api.config();
      if (!mounted) return;
      setState(() {
        _depth.text = raw['h_road_cm.drain_01']?.toString() ?? _depth.text;
        _sensor.text = raw['h_sensor_cm.drain_01']?.toString() ?? _sensor.text;
      });
    } catch (_) {}
  }

  @override
  void didUpdateWidget(SettingsScreen oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (oldWidget.hubUrl != widget.hubUrl && _url.text != widget.hubUrl) {
      _url.text = widget.hubUrl;
    }
  }

  @override
  void dispose() {
    _url.dispose();
    _depth.dispose();
    _sensor.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return ListView(
      padding: const EdgeInsets.all(16),
      children: [
        const Text('Hub address', style: TextStyle(fontWeight: FontWeight.w700)),
        const SizedBox(height: 6),
        const Text(
          'The phone uses https://tabaport.onrender.com. '
          'Change it here only if you need the laptop or the emulator.',
          style: TextStyle(color: Colors.black54, fontSize: 13),
        ),
        const SizedBox(height: 8),
        TextField(
          controller: _url,
          decoration: const InputDecoration(
            labelText: 'https://tabaport.onrender.com',
            border: OutlineInputBorder(),
          ),
        ),
        const SizedBox(height: 8),
        Wrap(spacing: 8, children: [
          OutlinedButton(
            onPressed: () => _url.text = HubConfig.renderHub,
            child: const Text('Render hub'),
          ),
          OutlinedButton(
            onPressed: () => _url.text = HubConfig.local,
            child: const Text('This computer'),
          ),
          OutlinedButton(
            onPressed: () => _url.text = HubConfig.androidEmulator,
            child: const Text('Android emulator'),
          ),
          FilledButton(
            onPressed: () async {
              await widget.onHubUrl(_url.text);
              setState(() => _msg = 'hub URL saved');
            },
            child: const Text('Save URL'),
          ),
        ]),
        const Divider(height: 32),
        const Text('Calibration (Joel 50 % / 80 %)', style: TextStyle(fontWeight: FontWeight.w700)),
        const SizedBox(height: 8),
        TextField(
          controller: _depth,
          keyboardType: TextInputType.number,
          decoration: const InputDecoration(labelText: 'Drain depth H_road (cm)', border: OutlineInputBorder()),
        ),
        const SizedBox(height: 8),
        TextField(
          controller: _sensor,
          keyboardType: TextInputType.number,
          decoration: const InputDecoration(labelText: 'Probe height H_sensor (cm)', border: OutlineInputBorder()),
        ),
        const SizedBox(height: 8),
        FilledButton(
          onPressed: () async {
            final depth = double.tryParse(_depth.text) ?? 60;
            final sensor = double.tryParse(_sensor.text) ?? 50;
            final yellow = (depth * 0.5).round();
            final red = (depth * 0.8).round().clamp(0, (depth - 10).round());
            try {
              await widget.api.saveConfig({
                'h_road_cm.drain_01': '$depth',
                'h_sensor_cm.drain_01': '$sensor',
                'yellow_cm.drain_01': '$yellow',
                'red_cm.drain_01': '$red',
              });
              setState(() => _msg = 'saved yellow $yellow cm, red $red cm — hub pushes CFG to the node');
            } catch (e) {
              setState(() => _msg = e.toString());
            }
          },
          child: const Text('Compute and save on hub'),
        ),
        if (_msg.isNotEmpty)
          Padding(
            padding: const EdgeInsets.only(top: 12),
            child: Text(_msg, style: const TextStyle(color: Colors.black87)),
          ),
      ],
    );
  }
}
