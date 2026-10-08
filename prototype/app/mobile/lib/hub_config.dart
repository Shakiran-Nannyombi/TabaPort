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

  /// This laptop on the lab Wi-Fi. A physical phone cannot use 127.0.0.1.
  static const lan = 'http://10.10.152.131:5000';

  /// Public address for the hub while it is running on this laptop.
  /// The quick tunnel URL changes if that process is restarted.
  static const publicHub = 'https://cited-nylon-pittsburgh-throws.trycloudflare.com';

  static bool get _onPhone => !kIsWeb && Platform.isAndroid;

  static bool _stale(String? saved) {
    if (saved == null || saved.isEmpty) return true;
    return saved.contains('127.0.0.1') ||
        saved.contains('localhost') ||
        saved.contains('10.0.2.2') ||
        saved.contains('10.10.');
  }

  static Future<String> load() async {
    final p = await SharedPreferences.getInstance();
    final saved = p.getString(_key);
    if (_onPhone && (_stale(saved) || (saved != null && saved.contains('trycloudflare.com') && saved != publicHub))) {
      return publicHub;
    }
    return saved ?? (_onPhone ? publicHub : local);
  }

  static Future<void> save(String url) async {
    final p = await SharedPreferences.getInstance();
    var u = url.trim();
    if (u.endsWith('/')) u = u.substring(0, u.length - 1);
    await p.setString(_key, u);
  }
}
