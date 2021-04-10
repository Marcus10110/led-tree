import * as fs from 'fs';
console.log('hello');

const ledPitch = 1.3123; // inches between LEDs

interface Point {
  x: number;
  y: number;
}

interface LedPoint extends Point {
  index: number;
}

const inputData: Point[][] = [
  [
    { x: 0, y: 0 },
    { x: -129.5, y: 0 },
    { x: -173.352, y: 51.118 },
  ],
  [
    { x: 0, y: 0 },
    { x: 0, y: 70 },
    { x: 13, y: 70 },
    { x: 13, y: 113.25 },
    { x: 0, y: 113.25 },
    { x: 0, y: 170.850394 },
  ],
  [
    { x: -174.751381, y: 183.75 },
    { x: -174.751381, y: 52.75 },
    { x: -173.351518, y: 51.118165 },
  ],
  [
    { x: -174.751381, y: 183.75 },
    { x: 0, y: 183.75 },
    { x: 0, y: 170.85 },
  ],
];

// b - a, AKA A -> B
function delta(a: Point, b: Point): Point {
  return { x: b.x - a.x, y: b.y - a.y };
}

function magnitude(a: Point, b?: Point): number {
  const vector = b !== undefined ? delta(a, b) : a;
  return Math.sqrt(vector.x ** 2 + vector.y ** 2);
}

// b - a, A -> B
function unitVector(a: Point, b?: Point): Point {
  const vector = b !== undefined ? delta(a, b) : a;
  const mag = magnitude(vector);
  return { x: vector.x / mag, y: vector.y / mag };
}

function scale(a: Point, mag: number): Point {
  return { x: a.x * mag, y: a.y * mag };
}

function add(a: Point, b: Point): Point {
  return { x: a.x + b.x, y: a.y + b.y };
}

function travel(start: Point, direction: Point, distance: number): Point {
  const toAdd = scale(unitVector(direction), distance);
  return add(start, toAdd);
}

function printLeds(ledStrip: LedPoint[]) {
  console.log(ledStrip.map(point => `${point.index}, ${point.x}, ${point.y}`).join('\n'));
}

// what we want:
// strip: {index, x, y}[]
const ledStrips: LedPoint[][] = [];
for (const strip of inputData) {
  // console.log(strip);
  const ledStrip: LedPoint[] = [];
  let remainder = 0;
  for (let i = 0; i < strip.length - 1; ++i) {
    const currentPoint = strip[i];
    const nextPoint = strip[i + 1];
    const ledsToAdd = Math.max(Math.floor((magnitude(currentPoint, nextPoint) - remainder) / ledPitch), 0);
    let workingPoint = currentPoint;
    workingPoint = travel(workingPoint, unitVector(currentPoint, nextPoint), remainder); // advance by the leftovers.
    for (let j = 0; j < ledsToAdd; ++j) {
      ledStrip.push({ index: ledStrip.length, ...workingPoint });
      workingPoint = travel(workingPoint, unitVector(currentPoint, nextPoint), ledPitch);
      //
    }
    remainder = magnitude(workingPoint, nextPoint);
  }
  // console.log(ledStrip, ledStrip.length);
  // printLeds(ledStrip);
  ledStrips.push(ledStrip);
  // break;
}

const fileName = 'strips.csv';
const jsFilename = 'strips.js';

const content = ledStrips
  .map((strip, stripIndex) => strip.map(led => `strip${stripIndex}, ${led.index}, ${led.x}, ${led.y}`).join('\n'))
  .join('\n');

fs.writeFileSync(fileName, content, { encoding: 'utf8' });

const jsContent =
  'const strips = [' +
  ledStrips.map(strip => '[' + strip.map(led => `{x: ${led.x}, y: ${led.y}}`).join(',\n') + ']').join(',\n') +
  '];\n';
fs.writeFileSync(jsFilename, jsContent, { encoding: 'utf8' });
