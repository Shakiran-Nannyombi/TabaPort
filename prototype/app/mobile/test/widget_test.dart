import 'package:flutter_test/flutter_test.dart';
import 'package:tabaport_mobile/main.dart';

void main() {
  testWidgets('app title is TabaPort', (tester) async {
    await tester.pumpWidget(const TabaPortApp());
    expect(find.text('TabaPort'), findsWidgets);
  });
}
