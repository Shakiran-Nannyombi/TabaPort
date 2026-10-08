import 'package:flutter/material.dart';

import '../theme.dart';

/// Opening mark: the logo drops in, then the name and slogan.
class IntroScreen extends StatefulWidget {
  const IntroScreen({super.key, required this.onDone});
  final VoidCallback onDone;

  @override
  State<IntroScreen> createState() => _IntroScreenState();
}

class _IntroScreenState extends State<IntroScreen> with SingleTickerProviderStateMixin {
  late final AnimationController _controller;
  bool _done = false;

  void _finish() {
    if (_done) return;
    _done = true;
    widget.onDone();
  }

  @override
  void initState() {
    super.initState();
    _controller = AnimationController(vsync: this, duration: const Duration(milliseconds: 2800));
    _controller.addStatusListener((status) {
      if (status == AnimationStatus.completed && mounted) _finish();
    });
    _controller.forward();
  }

  @override
  void dispose() {
    _controller.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final logo = CurvedAnimation(parent: _controller, curve: const Interval(0.0, 0.42, curve: Curves.easeOutBack));
    final name = CurvedAnimation(parent: _controller, curve: const Interval(0.32, 0.58, curve: Curves.easeOut));
    final slogan = CurvedAnimation(parent: _controller, curve: const Interval(0.50, 0.78, curve: Curves.easeOut));
    final bars = CurvedAnimation(parent: _controller, curve: const Interval(0.62, 0.88, curve: Curves.easeOut));

    return Scaffold(
      backgroundColor: Colors.white,
      body: GestureDetector(
        behavior: HitTestBehavior.opaque,
        onTap: _finish,
        child: SafeArea(
          child: Center(
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                AnimatedBuilder(
                  animation: logo,
                  builder: (context, child) {
                    return Opacity(
                      opacity: logo.value.clamp(0.0, 1.0),
                      child: Transform.translate(
                        offset: Offset(0, (1 - logo.value) * -36),
                        child: Transform.scale(scale: 0.72 + (0.28 * logo.value), child: child),
                      ),
                    );
                  },
                  child: Image.asset('assets/logo.png', width: 168, height: 168),
                ),
                const SizedBox(height: 8),
                FadeTransition(
                  opacity: name,
                  child: const Text(
                    'TabaPort',
                    style: TextStyle(fontSize: 36, fontWeight: FontWeight.w800, color: Color(0xFF1A2330), letterSpacing: -0.5),
                  ),
                ),
                const SizedBox(height: 10),
                FadeTransition(
                  opacity: slogan,
                  child: const Padding(
                    padding: EdgeInsets.symmetric(horizontal: 36),
                    child: Text(
                      'See the water before it closes the road.',
                      textAlign: TextAlign.center,
                      style: TextStyle(fontSize: 16, height: 1.35, color: Color(0xFF5C6770)),
                    ),
                  ),
                ),
                const SizedBox(height: 22),
                AnimatedBuilder(
                  animation: bars,
                  builder: (context, _) {
                    return Row(
                      mainAxisAlignment: MainAxisAlignment.center,
                      children: [
                        _bar(kGreen, bars.value),
                        const SizedBox(width: 8),
                        _bar(kYellow, bars.value),
                        const SizedBox(width: 8),
                        _bar(kRed, bars.value),
                      ],
                    );
                  },
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }

  Widget _bar(Color color, double t) {
    return Container(
      width: 36 * t,
      height: 8,
      decoration: BoxDecoration(color: color, borderRadius: BorderRadius.circular(4)),
    );
  }
}
