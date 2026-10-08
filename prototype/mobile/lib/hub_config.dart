import 'dart:io' show Platform;

import 'package:flutter/foundation.dart';
import 'package:shared_preferences/shared_preferences.dart';

/// Where the phone finds the laptop hub.
///
/// Generation 1: ESP32 still talks USB-serial to the laptop. The phone never
/// opens the serial port. It calls the hub REST API over Wi-Fi on the same LAN.
///
/// Later (field node): the ESP32 will also join that Wi-Fi / MQTT. The phone
/// keeps talking to the hub; the hub keeps being the source of truth.
class HubConfig {
  HubConfig._();
  static const _key = 'hub_base_url';

  /// Laptop on this machine (Flutter Linux / Chrome).
  static const local = 'http://127.0.0.1:5000';

  /// Android emulator's alias for the host machine.
  static const androidEmulator = 'http://10.0.2.2:5000';

  /// This laptop on the phone hotspot. A physical phone cannot use 127.0.0.1.
  static const lan = 'http://192.168.43.31:5000';

  static bool get _onPhone => !kIsWeb && Platform.isAndroid;

  static bool _stale(String? saved) {
    if (saved == null || saved.isEmpty) return true;
    return saved.contains('127.0.0.1') ||
        saved.contains('localhost') ||
        saved.contains('10.0.2.2') ||
        saved.contains('10.10.') ||
        saved.contains('trycloudflare.com');
  }

  static Future<String> load() async {
    final p = await SharedPreferences.getInstance();
    final saved = p.getString(_key);
    if (_onPhone && _stale(saved)) return lan;
    return saved ?? (_onPhone ? lan : local);
  }

  static Future<void> save(String url) async {
    final p = await SharedPreferences.getInstance();
    var u = url.trim();
    if (u.endsWith('/')) u = u.substring(0, u.length - 1);
    await p.setString(_key, u);
  }
}
