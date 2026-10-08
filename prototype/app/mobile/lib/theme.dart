import 'package:flutter/material.dart';

const kGreen = Color(0xFF2ECC71);
const kYellow = Color(0xFFF1C40F);
const kRed = Color(0xFFE74C3C);
const kUnknown = Color(0xFF7F8C8D);
const kBlue = Color(0xFF3498DB);

Color stateColor(String? s) {
  switch (s) {
    case 'green':
      return kGreen;
    case 'yellow':
      return kYellow;
    case 'red':
      return kRed;
    default:
      return kUnknown;
  }
}

ThemeData tabaportTheme() {
  return ThemeData(
    brightness: Brightness.light,
    scaffoldBackgroundColor: Colors.white,
    colorScheme: const ColorScheme.light(
      surface: Colors.white,
      primary: kBlue,
      secondary: kGreen,
      error: kRed,
    ),
    appBarTheme: const AppBarTheme(
      backgroundColor: Colors.white,
      foregroundColor: Colors.black,
      elevation: 0,
      surfaceTintColor: Colors.white,
    ),
    cardTheme: CardThemeData(
      color: Colors.white,
      elevation: 0,
      shape: RoundedRectangleBorder(
        borderRadius: BorderRadius.circular(12),
        side: const BorderSide(color: Color(0xFFE6E8EC)),
      ),
    ),
    navigationBarTheme: const NavigationBarThemeData(
      backgroundColor: Colors.white,
      surfaceTintColor: Colors.white,
    ),
    useMaterial3: true,
  );
}
