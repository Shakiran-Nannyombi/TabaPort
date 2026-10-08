import 'dart:async';

import 'package:flutter/material.dart';

import 'api/hub_api.dart';
import 'hub_config.dart';
import 'screens/intro_screen.dart';
import 'screens/jobs_screen.dart';
import 'screens/map_screen.dart';
import 'screens/node_screen.dart';
import 'screens/settings_screen.dart';
import 'theme.dart';

void main() {
  WidgetsFlutterBinding.ensureInitialized();
  runApp(const TabaPortApp());
}

class TabaPortApp extends StatelessWidget {
  const TabaPortApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'TabaPort',
      debugShowCheckedModeBanner: false,
      theme: tabaportTheme(),
      themeMode: ThemeMode.light,
      home: const _Opening(),
    );
  }
}

class _Opening extends StatefulWidget {
  const _Opening();

  @override
  State<_Opening> createState() => _OpeningState();
}

class _OpeningState extends State<_Opening> with WidgetsBindingObserver {
  bool _showIntro = true;
  bool _backgrounded = false;

  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addObserver(this);
  }

  @override
  void dispose() {
    WidgetsBinding.instance.removeObserver(this);
    super.dispose();
  }

  @override
  void didChangeAppLifecycleState(AppLifecycleState state) {
    if (state == AppLifecycleState.paused || state == AppLifecycleState.hidden) {
      _backgrounded = true;
    }
    if (state == AppLifecycleState.resumed && _backgrounded && mounted) {
      _backgrounded = false;
      setState(() => _showIntro = true);
    }
  }

  @override
  Widget build(BuildContext context) {
    if (!_showIntro) return const Shell();
    return IntroScreen(onDone: () {
      if (mounted) setState(() => _showIntro = false);
    });
  }
}

class Shell extends StatefulWidget {
  const Shell({super.key});

  @override
  State<Shell> createState() => _ShellState();
}

class _ShellState extends State<Shell> {
  late HubApi _api;
  String _url = HubConfig.local;
  Map<String, dynamic>? _state;
  String? _err;
  int _tab = 0;
  Timer? _poll;

  @override
  void initState() {
    super.initState();
    _api = HubApi(_url);
    HubConfig.load().then((u) {
      setState(() {
        _url = u;
        _api = HubApi(u);
      });
      _tick();
    });
    _poll = Timer.periodic(const Duration(seconds: 2), (_) => _tick());
  }

  @override
  void dispose() {
    _poll?.cancel();
    super.dispose();
  }

  Future<void> _setUrl(String url) async {
    await HubConfig.save(url);
    setState(() {
      _url = url.endsWith('/') ? url.substring(0, url.length - 1) : url.trim();
      _api = HubApi(_url);
    });
    await _tick();
  }

  Future<void> _tick() async {
    try {
      final s = await _api.state();
      if (mounted) setState(() { _state = s; _err = null; });
    } catch (e) {
      if (mounted) setState(() => _err = e.toString());
    }
  }

  @override
  Widget build(BuildContext context) {
    final serial = Map<String, dynamic>.from(_state?['serial'] as Map? ?? {});
    final nodes = Map<String, dynamic>.from(_state?['nodes'] as Map? ?? {});
    final n = Map<String, dynamic>.from(nodes['drain_01'] as Map? ?? {});
    final segs = (_state?['segments'] as List? ?? []);
    final drain = segs.cast<dynamic>().firstWhere(
          (s) => s['node_id'] == 'drain_01',
          orElse: () => {'state': 'unknown'},
        ) as Map;
    final st = drain['state'] as String? ?? 'unknown';
    final open = (_state?['jobs'] as Map? ?? {})['open'] ?? 0;

    final pages = [
      MapScreen(api: _api, state: _state, onChanged: _tick),
      JobsScreen(key: ValueKey(_url), api: _api),
      NodeScreen(api: _api, state: _state),
      SettingsScreen(api: _api, hubUrl: _url, onHubUrl: _setUrl),
    ];

    return Scaffold(
      appBar: AppBar(
        leading: const Padding(
          padding: EdgeInsets.all(8),
          child: Image(image: AssetImage('assets/logo.png')),
        ),
        title: const Text('TabaPort'),
        actions: [
          Padding(
            padding: const EdgeInsets.only(right: 12),
            child: Center(
              child: Container(
                padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 4),
                decoration: BoxDecoration(color: stateColor(st), borderRadius: BorderRadius.circular(20)),
                child: Text(
                  n['level_cm'] != null ? 'drain_01 · $st · ${n['level_cm']} cm' : 'drain_01 · $st',
                  style: const TextStyle(color: Colors.black, fontWeight: FontWeight.w800, fontSize: 12),
                ),
              ),
            ),
          ),
          Icon(Icons.circle, size: 10, color: serial['connected'] == true ? kGreen : kRed),
          const SizedBox(width: 12),
        ],
      ),
      body: Column(
        children: [
          if (_err != null)
            Material(
              color: const Color(0xFFFFE8E6),
              child: Padding(
                padding: const EdgeInsets.all(8),
                child: Text(_err!, style: const TextStyle(fontSize: 12)),
              ),
            ),
          Expanded(child: pages[_tab]),
        ],
      ),
      bottomNavigationBar: NavigationBar(
        selectedIndex: _tab,
        onDestinationSelected: (i) => setState(() => _tab = i),
        destinations: [
          const NavigationDestination(icon: Icon(Icons.map_outlined), label: 'Map'),
          NavigationDestination(
            icon: Badge(isLabelVisible: open is num && open > 0, label: Text('$open'), child: const Icon(Icons.assignment_outlined)),
            label: 'Jobs',
          ),
          const NavigationDestination(icon: Icon(Icons.water_drop_outlined), label: 'Node'),
          const NavigationDestination(icon: Icon(Icons.settings_outlined), label: 'Settings'),
        ],
      ),
    );
  }
}
