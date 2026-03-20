'use client';

import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card';
import { Ruler } from 'lucide-react';

const noiseLevels = [
  { level: '110-120', name: 'Jet Engine / Rock Concert', description: 'Painful and dangerous. Immediate hearing damage risk.' },
  { level: '90-109', name: 'Lawnmower / Heavy Traffic', description: 'Very loud. Prolonged exposure can cause damage.' },
  { level: '75-89', name: 'Vacuum Cleaner / Busy Restaurant', description: 'Loud. Annoying and makes conversation difficult.' },
  { level: '60-74', name: 'Normal Conversation', description: 'Moderate. Comfortable for listening.' },
  { level: '40-59', name: 'Quiet Library', description: 'Faint. Very peaceful.' },
  { level: '30-39', name: 'Whisper', description: 'Very faint. Barely audible.' },
];

export function NoiseLevelReference() {
  return (
    <Card>
      <CardHeader>
        <CardTitle className="flex items-center gap-2">
          <Ruler className="h-5 w-5" />
          Common Noise Levels
        </CardTitle>
        <CardDescription>A reference for what different dB(A) levels sound like.</CardDescription>
      </CardHeader>
      <CardContent>
        <ul className="space-y-4">
          {noiseLevels.map((item, index) => (
            <li key={index} className="flex items-start gap-4">
              <div className="flex h-10 w-16 items-center justify-center rounded-lg bg-muted font-bold text-sm shrink-0">
                {item.level}
              </div>
              <div className="flex-1">
                <p className="font-semibold">{item.name}</p>
                <p className="text-sm text-muted-foreground">{item.description}</p>
              </div>
            </li>
          ))}
        </ul>
      </CardContent>
    </Card>
  );
}
