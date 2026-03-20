'use client';

import { WifiOff } from 'lucide-react';
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card';

export default function OfflinePage() {
  return (
    <main className="flex min-h-screen flex-col items-center justify-center p-4 sm:p-6 lg:p-8 bg-muted/40">
      <div className="w-full max-w-md mx-auto text-center">
        <Card>
            <CardHeader>
                <div className="mx-auto flex h-12 w-12 items-center justify-center rounded-full bg-muted">
                    <WifiOff className="h-6 w-6 text-muted-foreground" />
                </div>
            </CardHeader>
            <CardContent className="space-y-2">
                <CardTitle className="text-2xl">You are Offline</CardTitle>
                <CardDescription>
                A connection to the server could not be established. Please check your network connection and try again.
                </CardDescription>
            </CardContent>
        </Card>
      </div>
    </main>
  );
}
