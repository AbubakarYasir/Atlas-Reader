import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

class OpenCommandCenterIntent extends Intent {
  const OpenCommandCenterIntent();
}

/// Installs the global Ctrl+K shortcut without taking focus away from children.
class CommandCenterShortcuts extends StatelessWidget {
  const CommandCenterShortcuts({
    super.key,
    required this.onOpen,
    required this.child,
  });

  final VoidCallback onOpen;
  final Widget child;

  @override
  Widget build(BuildContext context) {
    return Shortcuts(
      shortcuts: const {
        SingleActivator(LogicalKeyboardKey.keyK, control: true):
            OpenCommandCenterIntent(),
      },
      child: Actions(
        actions: {
          OpenCommandCenterIntent: CallbackAction<OpenCommandCenterIntent>(
            onInvoke: (_) {
              onOpen();
              return null;
            },
          ),
        },
        child: Focus(autofocus: true, child: child),
      ),
    );
  }
}
