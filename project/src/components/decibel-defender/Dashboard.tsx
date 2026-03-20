
"use client";

import { useState, useEffect, useCallback } from 'react';
import { Moon, Bot, ShieldAlert, ChevronsLeft, ChevronsRight, Bluetooth, BluetoothOff, Sun, Settings, Volume2, Power } from 'lucide-react';

import { Card, CardContent, CardHeader, CardTitle, CardDescription } from '@/components/ui/card';
import { Button } from '@/components/ui/button';
import { Slider } from '@/components/ui/slider';
import { Label } from '@/components/ui/label';
import { Separator } from '@/components/ui/separator';
import { cn } from '@/lib/utils';
import { Switch } from '@/components/ui/switch';
import { useToast } from '@/hooks/use-toast';
import { NoiseLevelReference } from './NoiseLevelReference';
import { Accordion, AccordionContent, AccordionItem, AccordionTrigger } from '@/components/ui/accordion';
import { Progress } from '@/components/ui/progress';

const MOVEMENT_COOLDOWN_MS = 500; // 0.5-second cooldown between movements

export function Dashboard() {
  const [curtainOpenness, setCurtainOpenness] = useState(100); // 0 = closed, 100 = fully open
  const [closeThreshold, setCloseThreshold] = useState(75);
  const [isAutoMode, setIsAutoMode] = useState(true);
  const [isNoiseAutoCloseEnabled, setIsNoiseAutoCloseEnabled] = useState(true);
  const [motorSpeed, setMotorSpeed] = useState(400);
  const [motorAccel, setMotorAccel] = useState(200);
  const [noiseLevel, setNoiseLevel] = useState<number | null>(null);

  const { toast } = useToast();
  const [isClient, setIsClient] = useState(false);
  const [theme, setTheme] = useState('dark');

  const [lastMovementTimestamp, setLastMovementTimestamp] = useState(0);
  
  const [btCharacteristic, setBtCharacteristic] = useState<BluetoothRemoteGATTCharacteristic | null>(null);
  const [btSpeedCharacteristic, setBtSpeedCharacteristic] = useState<BluetoothRemoteGATTCharacteristic | null>(null);
  const [btAccelCharacteristic, setBtAccelCharacteristic] = useState<BluetoothRemoteGATTCharacteristic | null>(null);
  const [btNoiseCharacteristic, setBtNoiseCharacteristic] = useState<BluetoothRemoteGATTCharacteristic | null>(null);


  const isHardwareConnected = !!btCharacteristic;

  const handleNoiseValueChanged = useCallback((event: Event) => {
    const target = event.target as BluetoothRemoteGATTCharacteristic;
    const value = target.value;

    if (value && value.byteLength >= 4) {
      const dbValue = value.getFloat32(0, true); 
      if (!isNaN(dbValue) && isFinite(dbValue)) {
        setNoiseLevel(Math.round(dbValue * 10) / 10);
      }
    }
  }, []);

  const clearBluetoothState = useCallback(() => {
    setBtCharacteristic(null);
    setBtSpeedCharacteristic(null);
    setBtAccelCharacteristic(null);
    if (btNoiseCharacteristic) {
      btNoiseCharacteristic.removeEventListener('characteristicvaluechanged', handleNoiseValueChanged);
    }
    setBtNoiseCharacteristic(null);
    setNoiseLevel(null);
  }, [btNoiseCharacteristic, handleNoiseValueChanged]);

  const handleBluetoothDisconnect = useCallback(() => {
      if (btCharacteristic?.service.device.gatt?.connected) {
          btCharacteristic.service.device.gatt.disconnect();
      }
      clearBluetoothState();
      toast({ title: 'Disconnected from Bluetooth' });
  }, [btCharacteristic, clearBluetoothState, toast]);

  const handleBluetoothConnect = useCallback(async () => {
    if (!navigator.bluetooth) {
      toast({ variant: 'destructive', title: 'Web Bluetooth Not Supported', description: 'Please use a compatible browser like Chrome.' });
      return;
    }
  
    try {
      toast({ title: 'Scanning for devices...', description: 'Please choose "AcousticCurtain-Module" from the list.' });
  
      const device = await navigator.bluetooth.requestDevice({
        filters: [{ services: ['4fafc201-1fb5-459e-8fcc-c5c9c331914b'] }],
      });
  
      if (!device) {
        toast({ variant: 'destructive', title: 'Scan Canceled', description: 'No device was selected.' });
        return;
      }
  
      clearBluetoothState();
      toast({ title: 'Device Found!', description: `Connecting to ${device.name || 'AcousticCurtain-Module'}...` });
  
      device.addEventListener('gattserverdisconnected', () => {
        clearBluetoothState();
        toast({ title: 'Bluetooth Disconnected', description: 'You can reconnect at any time.'});
      });
  
      const server = await device.gatt?.connect();
      toast({ title: 'Connected!', description: 'Getting services...'});

      const service = await server?.getPrimaryService('4fafc201-1fb5-459e-8fcc-c5c9c331914b');
      
      if (!service) {
        throw new Error("Bluetooth service not found. Make sure the device is on and in range.");
      }
      
      toast({ title: 'Service Found', description: 'Setting up characteristics...'});
      const commandChar = await service.getCharacteristic('beb5483e-36e1-4688-b7f5-ea07361b26a8');
      setBtCharacteristic(commandChar);
  
      const decoder = new TextDecoder('utf-8');
      
      await Promise.allSettled([
        (async () => {
          try {
            const char = await service.getCharacteristic('19b10001-e8f2-537e-4f6c-d104768a1214');
            await char.startNotifications();
            char.addEventListener('characteristicvaluechanged', handleNoiseValueChanged);
            setBtNoiseCharacteristic(char);
          } catch (error) {
            console.warn("Noise characteristic not found.", error);
          }
        })(),
        (async () => {
          try {
            const char = await service.getCharacteristic('19b10002-e8f2-537e-4f6c-d104768a1214');
            const value = await char.readValue();
            setMotorSpeed(parseFloat(decoder.decode(value)));
            setBtSpeedCharacteristic(char);
          } catch (error) {
            console.warn("Speed characteristic not found. Motor settings will not be available.", error);
          }
        })(),
        (async () => {
          try {
            const char = await service.getCharacteristic('19b10003-e8f2-537e-4f6c-d104768a1214');
            const value = await char.readValue();
            setMotorAccel(parseFloat(decoder.decode(value)));
            setBtAccelCharacteristic(char);
          } catch (error) {
            console.warn("Acceleration characteristic not found. Motor settings will not be available.", error);
          }
        })(),
      ]);
  
      toast({ title: 'Device Ready' });
  
    } catch (error: any) {
      console.error('Bluetooth connection failed:', error);
      let description = 'An unknown error occurred.';
      if (error.name === 'NotFoundError') {
        description = 'No compatible device found. Make sure the device is advertising and in range.';
      } else if (error.name === 'NotAllowedError' || error.name === 'SecurityError') {
        description = 'You must grant permission to access Bluetooth devices.';
      } else {
        description = error.message;
      }
      toast({ variant: 'destructive', title: 'Bluetooth Connection Failed', description });
    }
  }, [toast, clearBluetoothState, handleNoiseValueChanged]);

  const sendBleCommand = useCallback(async (command: string) => {
    if (!btCharacteristic) {
      toast({
        variant: "destructive",
        title: "Not Connected",
        description: "Please connect to the hardware via Bluetooth.",
      });
      return false;
    }
    try {
      const encoder = new TextEncoder();
      await btCharacteristic.writeValueWithResponse(encoder.encode(command));
      return true;
    } catch (error: any) {
      console.error("BLE write error:", error);
      toast({
        variant: "destructive",
        title: "Bluetooth Error",
        description: "Could not send command.",
      });
      return false;
    }
  }, [btCharacteristic, toast]);
  
  const writeBleCharacteristic = useCallback(async (characteristic: BluetoothRemoteGATTCharacteristic | null, value: string) => {
    if (!characteristic) return;
    try {
      const encoder = new TextEncoder();
      await characteristic.writeValueWithResponse(encoder.encode(value));
    } catch (error) {
      console.error("BLE characteristic write error:", error);
    }
  }, []);

  const handleShutdown = () => {
      if (!isHardwareConnected) {
          toast({
              variant: 'destructive',
              title: 'Not Connected',
              description: 'Please connect to the hardware first.',
          });
          return;
      }
      sendBleCommand('restart');
      toast({ title: 'Shutting Down Device', description: 'The ESP32 will reboot. You will need to reconnect.' });
  };
  
  useEffect(() => {
    if (!isHardwareConnected || !isClient) return;

    const handler = setTimeout(() => {
      const configString = `config:auto=${isAutoMode ? 1 : 0};noise=${isNoiseAutoCloseEnabled ? 1 : 0};thresh=${closeThreshold}`;
      sendBleCommand(configString);
    }, 500);

    return () => {
      clearTimeout(handler);
    };
  }, [isAutoMode, isNoiseAutoCloseEnabled, closeThreshold, isHardwareConnected, isClient, sendBleCommand]);


  const sendCurtainCommand = useCallback(async (percentage: number, reason: string) => {
    const now = Date.now();
    if (now - lastMovementTimestamp < MOVEMENT_COOLDOWN_MS) return;
    
    setLastMovementTimestamp(now);
    setCurtainOpenness(percentage);
    
    await sendBleCommand(`goto:${percentage}`);
  }, [ lastMovementTimestamp, sendBleCommand ]);
  
  useEffect(() => {
    setIsClient(true);
    const savedTheme = localStorage.getItem('acousticcurtain-module-theme') || 'dark';
    setTheme(savedTheme);
    document.documentElement.classList.toggle('dark', savedTheme === 'dark');
  }, []);
  
  const toggleTheme = () => {
    const newTheme = theme === 'light' ? 'dark' : 'light';
    setTheme(newTheme);
    localStorage.setItem('acousticcurtain-module-theme', newTheme);
    document.documentElement.classList.toggle('dark', newTheme === 'dark');
  };

  if (!isClient) {
    return null; // or a skeleton loader
  }
  
  return (
    <div className="grid grid-cols-1 lg:grid-cols-3 gap-6 xl:gap-8 items-start">
      <div className="lg:col-span-2 grid grid-cols-1 gap-6">
        <Card>
          <CardHeader className="flex flex-row items-start justify-between">
            <div>
              <CardTitle className="flex items-center gap-2">
                <div className={cn("w-2 h-2 rounded-full", isHardwareConnected ? "bg-green-500" : "bg-red-500")}></div>
                 Control Center
              </CardTitle>
              <CardDescription>Directly control the curtain and automation settings</CardDescription>
            </div>
            <div className="flex flex-col items-end gap-2">
                <div className="flex items-center gap-2">
                    <Button
                        variant="destructive"
                        size="sm"
                        onClick={handleShutdown}
                        disabled={!isHardwareConnected}
                    >
                        <Power className="h-4 w-4" />
                        Shutdown
                    </Button>
                    <Button variant="ghost" size="icon" className="h-8 w-8" onClick={toggleTheme}>
                        {theme === 'light' ? <Moon className="h-5 w-5" /> : <Sun className="h-5 w-5" />}
                        <span className="sr-only">Toggle Theme</span>
                    </Button>
                </div>
                {isHardwareConnected ? (
                  <Button variant="outline" size="sm" onClick={handleBluetoothDisconnect} className="w-full">
                    <BluetoothOff className="h-4 w-4 mr-2" />
                    Disconnect
                  </Button>
                ) : (
                  <Button variant="default" size="sm" onClick={handleBluetoothConnect} className="w-full">
                    <Bluetooth className="h-4 w-4 mr-2" />
                    Connect
                  </Button>
                )}
            </div>
          </CardHeader>
          <CardContent className={cn("space-y-6", !isHardwareConnected && "opacity-50 pointer-events-none")}>
            <div className='space-y-4'>
              <h3 className="text-base font-semibold">Manual Control</h3>
              <div className="flex items-center gap-2">
                <Button
                  size="lg"
                  className="flex-1"
                  variant="secondary"
                  onClick={() => sendCurtainCommand(0, 'Manual close')}
                  disabled={curtainOpenness === 0}
                >
                  <ChevronsLeft className="h-5 w-5 mr-2" />
                  Close
                </Button>
                <Button
                  size="lg"
                  className="flex-1"
                  variant="default"
                  onClick={() => sendCurtainCommand(100, 'Manual open')}
                  disabled={curtainOpenness === 100}
                >
                  Open
                  <ChevronsRight className="h-5 w-5 ml-2" />
                </Button>
              </div>
            </div>
            <Separator />
            <div className='space-y-4'>
              <div className="flex items-center justify-between">
                <h3 className="text-base font-semibold flex items-center gap-2"><Bot /> Automation</h3>
                <Switch 
                  id="auto-mode" 
                  checked={isAutoMode} 
                  onCheckedChange={setIsAutoMode} 
                />
              </div>
              <p className="text-sm text-muted-foreground -mt-2">{isAutoMode ? 'Automation is active.' : 'Automation is disabled.'}</p>
              
              <div className={cn("grid md:grid-cols-1 gap-4 pt-2", !isAutoMode && "opacity-50 pointer-events-none")}>
                <div className="space-y-4 p-4 rounded-lg border bg-background">
                  <div className="flex items-center justify-between">
                    <Label htmlFor="auto-close-mode" className="flex items-center gap-2 font-medium">
                      <ShieldAlert className="h-4 w-4" /> Noise-Responsive
                    </Label>
                    <Switch 
                      id="auto-close-mode" 
                      checked={isNoiseAutoCloseEnabled} 
                      onCheckedChange={setIsNoiseAutoCloseEnabled}
                    />
                  </div>
                   <p className="text-sm text-muted-foreground -mt-2">
                      Automatically closes when noise is high.
                   </p>

                   <div className="space-y-3 pt-4">
                      <div className="flex justify-between items-center">
                          <Label htmlFor="live-noise" className="text-sm flex items-center gap-2 text-muted-foreground">
                              <Volume2 className="h-4 w-4" /> Live Noise Level
                          </Label>
                          <span id="live-noise" className="font-mono font-medium text-lg">
                              {noiseLevel !== null ? `${noiseLevel} dB(A)` : '...'}
                          </span>
                      </div>
                      <Progress value={noiseLevel === null ? 0 : Math.min(120, Math.max(0, noiseLevel))} max={120} aria-label={`${noiseLevel || 0} dB`} />
                  </div>

                  <Separator className="my-4" />

                  <div className="space-y-3">
                    <div className="flex justify-between items-center">
                      <Label htmlFor="threshold" className="text-sm">Noise Threshold</Label>
                      <span className="font-mono font-medium text-sm">{closeThreshold} dB(A)</span>
                    </div>
                    <Slider
                      id="threshold"
                      min={0} max={120} step={1}
                      value={[closeThreshold]}
                      onValueChange={(value) => setCloseThreshold(value[0])}
                      disabled={!isNoiseAutoCloseEnabled}
                    />
                  </div>
                </div>
              </div>
            </div>

            {isHardwareConnected && (btSpeedCharacteristic || btAccelCharacteristic) && (
              <>
                <Separator />
                <Accordion type="single" collapsible className="w-full">
                  <AccordionItem value="motor-settings">
                    <AccordionTrigger>
                      <h3 className="text-base font-semibold flex items-center gap-2">
                        <Settings /> Motor Settings
                      </h3>
                    </AccordionTrigger>
                    <AccordionContent className="pt-4 space-y-6">
                        <p className="text-sm text-muted-foreground -mt-2">
                            Fine-tune the motor performance. Lower these values if the motor is skipping steps.
                        </p>
                        {btSpeedCharacteristic && (
                          <div className="space-y-3">
                              <div className="flex justify-between items-center">
                                  <Label htmlFor="speed" className="text-sm">Maximum Speed</Label>
                                  <span className="font-mono font-medium text-sm">{motorSpeed} steps/sec</span>
                              </div>
                              <Slider
                                  id="speed"
                                  min={100} max={2000} step={50}
                                  value={[motorSpeed]}
                                  onValueChange={(value) => setMotorSpeed(value[0])}
                                  onValueCommit={(value) => writeBleCharacteristic(btSpeedCharacteristic, String(value[0]))}
                              />
                          </div>
                        )}
                        {btAccelCharacteristic && (
                          <div className="space-y-3">
                              <div className="flex justify-between items-center">
                                  <Label htmlFor="accel" className="text-sm">Acceleration</Label>
                                  <span className="font-mono font-medium text-sm">{motorAccel} steps/sec²</span>
                              </div>
                              <Slider
                                  id="accel"
                                  min={50} max={1000} step={10}
                                  value={[motorAccel]}
                                  onValueChange={(value) => setMotorAccel(value[0])}
                                  onValueCommit={(value) => writeBleCharacteristic(btAccelCharacteristic, String(value[0]))}
                              />
                          </div>
                        )}
                    </AccordionContent>
                  </AccordionItem>
                </Accordion>
              </>
            )}
          </CardContent>
        </Card>
      </div>

      <div className="lg:col-span-1">
        <NoiseLevelReference />
      </div>
    </div>
  );
}
