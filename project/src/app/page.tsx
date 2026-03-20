
import { Dashboard } from '@/components/decibel-defender/Dashboard';

export default function Home() {
  return (
    <main className="flex min-h-screen flex-col items-center justify-center p-4 sm:p-6 lg:p-8 bg-muted/40">
      <div className="w-full max-w-screen-2xl mx-auto">
        <header className="mb-8 flex flex-wrap items-center justify-between gap-4">
          <div>
            <h1 className="text-3xl font-bold tracking-tight">
              AcousticCurtain Module
            </h1>
            <p className="text-muted-foreground">
              auto-deploying acoustic curtain.
            </p>
          </div>
        </header>
        <Dashboard />
      </div>
    </main>
  );
}
