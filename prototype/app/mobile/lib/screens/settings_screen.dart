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
  final Map<String, TextEditingController> _cfg = {};
  String _msg = '';
  String? _cfgError;

  @override
  void initState() {
    super.initState();
    _url = TextEditingController(text: widget.hubUrl);
    _loadConfig();
  }

  Future<void> _loadConfig() async {
    try {
      final raw = await widget.api.config();
      if (!mounted) return;
      setState(() {
        _cfgError = null;
        for (final e in raw.entries) {
          final key = e.key.toString();
          final value = e.value.toString();
          final existing = _cfg[key];
          if (existing == null) {
            _cfg[key] = TextEditingController(text: value);
          } else {
            existing.text = value;
          }
        }
        _depth.text = raw['h_road_cm.drain_01']?.toString() ?? _depth.text;
        _sensor.text = raw['h_sensor_cm.drain_01']?.toString() ?? _sensor.text;
      });
    } catch (e) {
      if (mounted) setState(() => _cfgError = e.toString());
    }
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
    for (final c in _cfg.values) {
      c.dispose();
    }
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
          'The phone uses the public hub address, so it does not need this laptop\'s Wi-Fi address. '
          'The hub process still runs on the laptop.',
          style: TextStyle(color: Colors.black54, fontSize: 13),
        ),
        const SizedBox(height: 8),
        TextField(
          controller: _url,
          decoration: const InputDecoration(
            labelText: 'https://….trycloudflare.com',
            border: OutlineInputBorder(),
          ),
        ),
        const SizedBox(height: 8),
        Wrap(spacing: 8, children: [
          OutlinedButton(
            onPressed: () => _url.text = HubConfig.publicHub,
            child: const Text('Public hub'),
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
              await _loadConfig();
            } catch (e) {
              setState(() => _msg = e.toString());
            }
          },
          child: const Text('Compute and save on hub'),
        ),
        const Divider(height: 32),
        const Text('All hub settings', style: TextStyle(fontWeight: FontWeight.w700)),
        const SizedBox(height: 6),
        const Text(
          'These are the same rows as the laptop Settings page. They load from the hub after you connect. '
          'Saving here writes them on the laptop, including crews, silence timeout, and the node-to-road link.',
          style: TextStyle(color: Colors.black54, fontSize: 13),
        ),
        const SizedBox(height: 8),
        if (_cfgError != null)
          Text(_cfgError!, style: const TextStyle(color: Colors.red))
        else if (_cfg.isEmpty)
          const Text('Connect to the hub to load settings.', style: TextStyle(color: Colors.black54))
        else ...[
          for (final key in _cfg.keys.toList()..sort())
            Padding(
              padding: const EdgeInsets.only(bottom: 8),
              child: TextField(
                controller: _cfg[key],
                decoration: InputDecoration(labelText: key, border: const OutlineInputBorder(), isDense: true),
              ),
            ),
          FilledButton(
            onPressed: () async {
              try {
                await widget.api.saveConfig({for (final e in _cfg.entries) e.key: e.value.text});
                setState(() => _msg = 'all settings saved on the hub');
                await _loadConfig();
              } catch (e) {
                setState(() => _msg = e.toString());
              }
            },
            child: const Text('Save all'),
          ),
        ],
        if (_msg.isNotEmpty)
          Padding(
            padding: const EdgeInsets.only(top: 12),
            child: Text(_msg, style: const TextStyle(color: Colors.black87)),
          ),
      ],
    );
  }
}
