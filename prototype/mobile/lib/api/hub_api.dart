import 'dart:convert';

import 'package:http/http.dart' as http;

class HubException implements Exception {
  HubException(this.message);
  final String message;
  @override
  String toString() => message;
}

class HubApi {
  HubApi(this.baseUrl);
  String baseUrl;

  Uri _u(String path, [Map<String, String>? q]) =>
      Uri.parse('$baseUrl$path').replace(queryParameters: q);

  Future<dynamic> _get(String path, [Map<String, String>? q]) async {
    try {
      final r = await http.get(_u(path, q)).timeout(const Duration(seconds: 4));
      if (r.statusCode >= 400) {
        throw HubException('GET $path failed (${r.statusCode})');
      }
      return jsonDecode(r.body);
    } on HubException {
      rethrow;
    } catch (e) {
      throw HubException('Cannot reach hub at $baseUrl. Is app.py running with HOST=0.0.0.0?');
    }
  }

  Future<dynamic> _post(String path, [Map<String, dynamic>? body]) async {
    try {
      final r = await http
          .post(_u(path),
              headers: {'Content-Type': 'application/json'},
              body: jsonEncode(body ?? {}))
          .timeout(const Duration(seconds: 6));
      final j = r.body.isEmpty ? {} : jsonDecode(r.body);
      if (r.statusCode >= 400) {
        throw HubException(j is Map && j['error'] != null ? j['error'].toString() : 'POST $path failed');
      }
      return j;
    } on HubException {
      rethrow;
    } catch (e) {
      throw HubException('Cannot reach hub at $baseUrl');
    }
  }

  Future<Map<String, dynamic>> state() async =>
      Map<String, dynamic>.from(await _get('/api/state'));

  Future<Map<String, dynamic>> readings({String node = 'drain_01', int minutes = 5}) async =>
      Map<String, dynamic>.from(await _get('/api/readings', {'node': node, 'minutes': '$minutes'}));

  Future<Map<String, dynamic>> jobs({String show = 'active'}) async =>
      Map<String, dynamic>.from(await _get('/api/jobs', {'show': show}));

  Future<Map<String, dynamic>> config() async =>
      Map<String, dynamic>.from(await _get('/api/config'));

  Future<Map<String, dynamic>> geojson(String name) async =>
      Map<String, dynamic>.from(await _get('/data/$name'));

  Future<void> jobAction(int id, String action, [Map<String, dynamic>? body]) async {
    await _post('/api/jobs/$id/$action', body);
  }

  Future<Map<String, dynamic>> report({required String text, String type = 'flooding'}) async {
    return Map<String, dynamic>.from(await _post('/api/reports', {'text': text, 'report_type': type}));
  }

  Future<void> saveConfig(Map<String, String> body) async {
    await _post('/api/config', body);
  }
}
