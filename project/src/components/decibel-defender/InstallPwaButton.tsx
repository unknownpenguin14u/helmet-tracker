"use client";

import { useState, useEffect } from 'react';
import { Button } from '@/components/ui/button';
import { Download, CheckCircle } from 'lucide-react';
import { Label } from '@/components/ui/label';

interface BeforeInstallPromptEvent extends Event {
  readonly platforms: Array<string>;
  readonly userChoice: Promise<{
    outcome: 'accepted' | 'dismissed',
    platform: string
  }>;
  prompt(): Promise<void>;
}

export function InstallPwaButton() {
  const [installPrompt, setInstallPrompt] = useState<BeforeInstallPromptEvent | null>(null);
  const [isStandalone, setIsStandalone] = useState(false);

  useEffect(() => {
    // Check if the app is already running in standalone mode.
    if (window.matchMedia('(display-mode: standalone)').matches) {
        setIsStandalone(true);
    }

    const handleBeforeInstallPrompt = (e: Event) => {
      e.preventDefault();
      setInstallPrompt(e as BeforeInstallPromptEvent);
    };

    window.addEventListener('beforeinstallprompt', handleBeforeInstallPrompt);

    return () => {
      window.removeEventListener('beforeinstallprompt', handleBeforeInstallPrompt);
    };
  }, []);

  const handleInstallClick = async () => {
    if (!installPrompt) {
      return;
    }
    await installPrompt.prompt();
    const { outcome } = await installPrompt.userChoice;
    if (outcome === 'accepted') {
      console.log('User accepted the install prompt');
      setIsStandalone(true); // Assume it's now installed
    } else {
      console.log('User dismissed the install prompt');
    }
    setInstallPrompt(null);
  };

  if (isStandalone) {
    return (
        <div className="space-y-3 bg-green-500/10 border-green-500/20 p-4 rounded-lg border">
            <Label className="flex items-center gap-2 font-semibold text-green-300">
                <CheckCircle className="h-5 w-5" /> App Installed
            </Label>
            <p className="text-sm text-green-400/80">
                The dashboard is running as an installed app. You can now access it offline directly from your desktop or home screen.
            </p>
        </div>
    );
  }

  if (!installPrompt) {
    return null;
  }

  return (
    <div className="space-y-3 bg-muted/50 p-4 rounded-lg border">
        <Label className="flex items-center gap-2 font-semibold">
            <Download className="h-5 w-5" /> Install for Offline Access
        </Label>
        <p className="text-sm text-muted-foreground">
            For the best experience and to use the dashboard even when your internet is down, install it as an app on your device.
        </p>
        <Button onClick={handleInstallClick} className="w-full">
            Install App
        </Button>
    </div>
  );
}
