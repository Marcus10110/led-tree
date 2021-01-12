import React from 'react';
import './App.css';
import { rgb, hsv } from 'color-convert';

const pxPerFoot = 250;

type Color = { r: number; g: number; b: number };
type Position = { x: number; y: number };

interface TreeData {
  ledCount: number;
  ledLengthPx: number;
  treeHeightPx: number;
  treeBasePx: number;
  ledPositions: Position[];
}

const defaultTree = {
  ledCount: 50,
  ledLengthPx: 11.5 * pxPerFoot,
  treeHeightPx: (20 / 12) * pxPerFoot,
  treeBasePx: (14 / 12) * pxPerFoot,
};

type LedFn = (data: TreeData, time: Date) => Color[];

const clamp = (n: number, start: number = 0, end: number = 255) => {
  if (start === 0 && end === 255) n = Math.round(n);
  if (n > end) n = end;
  if (n < start) n = start;
  return n;
};

const colorToString = ({ r, g, b }: Color): string => {
  return `rgb(${clamp(r)}, ${clamp(g)}, ${clamp(b)})`;
};

const testDraw: LedFn = (data, time) => {
  const result: { r: number; g: number; b: number }[] = [];
  const r = (Math.sin(time.getTime() / 500) + 1) * 127;

  for (let i = 0; i < data.ledCount; ++i) {
    const { x, y } = data.ledPositions[i];
    result.push({ r: r % 255, g: x % 255, b: y % 255 });
  }
  return result;
};

const rangeRandom = (min: number, max: number): number => {
  return Math.random() * (max - min) + min;
};

// depth, 0-1. 1 means 100% flicker (all the way off.) 0.1 means only flicker 10%.
// result is 0-1 multipler.
const flicker = (frequency: number, depth: number, timeSec: number) => {
  return (Math.cos(2 * Math.PI * frequency * timeSec) / 2 + 0.5) * depth + (1 - depth);
};

// brightness 0 to 1
const colorAdjustBrightness = ({ r, g, b }: Color, brightness: number): Color => {
  let [h, s, v] = rgb.hsv([r, g, b]);
  v *= brightness;
  const [r1, g1, b1] = hsv.rgb([h, s, v]);

  return { r: r1, g: g1, b: b1 };
};

// units in seconds. returns [0, 1]
const envelope = (period: number, duration: number, startOffset: number, timeSec: number) => {
  timeSec = (timeSec - startOffset) % period;
  if (timeSec < 0 || timeSec > duration) {
    return 0;
  }
  return Math.cos(((2 * Math.PI) / duration) * (timeSec - duration / 2)) / 2 + 0.5;
};

const drawStarsGen = (ledCount: number): LedFn => {
  const starColors: Color[] = [
    { r: 155, g: 176, b: 255 },
    { r: 170, g: 191, b: 255 },
    { r: 202, g: 216, b: 255 },
    { r: 248, g: 247, b: 255 },
    { r: 254, g: 244, b: 234 },
    { r: 254, g: 210, b: 163 },
    { r: 255, g: 203, b: 117 },
  ];
  const stars: ((time: Date) => Color)[] = [];
  for (let i = 0; i < ledCount; ++i) {
    const period = rangeRandom(10, 400);
    const duration = rangeRandom(period / 5, period * 0.9);
    const offset = rangeRandom(0, period);
    const maxBrightness = rangeRandom(0.15, 1);
    const flickerF = rangeRandom(0.05, 0.9);
    const flickerDepth = rangeRandom(0.05, 0.4);
    const color = starColors[Math.round(rangeRandom(0, starColors.length - 1))];
    stars.push((time: Date) => {
      const timeSec = time.getTime() / 1000;
      const brightness =
        envelope(period, duration, offset, timeSec) * maxBrightness * flicker(flickerF, flickerDepth, timeSec);
      return colorAdjustBrightness(color, brightness);
    });
  }

  return (data, time) => {
    const result: Color[] = [];

    for (let i = 0; i < data.ledCount; ++i) {
      const star = stars[i](time);
      result.push(star);
    }
    return result;
  };
};

const interpolate = (start: number, end: number, pos: number): number => {
  pos = clamp(pos, 0, 1);
  return (end - start) * pos + start;
};

const getPos = (start: number, end: number, current: number): number => {
  if (start > end) {
    return clamp((current - end) / (start - end), 0, 1);
  }
  const pos = clamp((current - start) / (end - start), 0, 1);
  //console.log(start, end, current, pos);
  return pos;
};

const interpolateColor = (start: Color, end: Color, pos: number): Color => {
  return {
    r: interpolate(start.r, end.r, pos),
    g: interpolate(start.g, end.g, pos),
    b: interpolate(start.b, end.b, pos),
  };
  /*
  HSV interpolation
  const [h1, s1, v1] = rgb.hsv([start.r, start.g, start.b]);
  const [h2, s2, v2] = rgb.hsv([end.r, end.g, end.b]);
  const h = interpolate(h1, h2, pos);
  const s = interpolate(s1, s2, pos);
  const v = interpolate(v1, v2, pos);
  const [r, g, b] = hsv.rgb([h, s, v]);
  return { r, g, b };
  */
};

const drawBlueSky: LedFn = (data, time) => {
  const result: Color[] = [];
  const topY = Math.min(...data.ledPositions.map(x => x.y));
  const bottomY = Math.max(...data.ledPositions.map(x => x.y));
  const topColor: Color = { r: 0, g: 120, b: 255 };
  const bottomColor: Color = { r: 194, g: 247, b: 255 };

  for (let i = 0; i < data.ledCount; ++i) {
    const height = getPos(bottomY, topY, data.ledPositions[i].y); // 0 to 1
    result.push(interpolateColor(bottomColor, topColor, height));
  }
  return result;
};

const computeLocations = (
  data: Pick<TreeData, 'ledCount' | 'ledLengthPx' | 'treeBasePx' | 'treeHeightPx'>,
  size: { width: number; height: number } = { width: 500, height: 500 },
): TreeData => {
  const ledPitch = data.ledLengthPx / data.ledCount;
  const squareHeight = data.treeBasePx * Math.PI;
  console.log(`data.ledLengthPx * 2 / squareHeight`, (data.ledLengthPx * 2) / squareHeight);

  const stripeCount = Math.floor(((data.ledLengthPx - squareHeight) * 2) / squareHeight) + 1;

  const stripePitch = data.treeHeightPx / stripeCount;
  console.log(data, { ledPitch, squareHeight, stripeCount, stripePitch });

  const ledLocations: { x: number; y: number }[] = [];
  let rowIndex = 0;
  let rowEnd = squareHeight;
  let rowStart = 0;
  for (let i = 0; i < data.ledCount; ++i) {
    const currentLength = i * ledPitch;
    if (currentLength >= rowEnd) {
      rowStart = currentLength;
      rowIndex += 1;
      const idealRowLength = squareHeight * (1 - rowIndex / stripeCount);
      const newRowLength = Math.round(idealRowLength / ledPitch) * ledPitch;
      rowEnd += newRowLength;
      console.log(`new row ${rowIndex}. start: ${rowStart}, end: ${rowEnd}`);
    }

    const rowLength = rowEnd - rowStart;
    const verticalShift =
      ((((currentLength - rowStart) / rowLength - 0.5) * stripePitch) / 1.5) * (rowLength / squareHeight);

    const horizontalShift = (squareHeight - rowLength) / 2;
    ledLocations.push({
      x: (currentLength - rowStart + horizontalShift) / Math.PI + 10,
      y: verticalShift * (i % 2 === 0 ? 1 : -1) + size.height - stripePitch * rowIndex - 10 - stripePitch,
    });
  }

  return { ...data, ledPositions: ledLocations };
};

const drawTree = (data: TreeData, drawFn: LedFn, context: CanvasRenderingContext2D) => {
  context.fillStyle = 'black';
  context.fillRect(0, 0, context.canvas.width, context.canvas.height);
  const time = new Date();
  // console.log(`calling draw tree`, time)
  const colorData = drawFn(data, time);
  if (data.ledCount !== data.ledPositions.length || data.ledCount !== colorData.length)
    throw Error('LED count mismatch');
  for (let i = 0; i < data.ledCount; ++i) {
    const { x, y } = data.ledPositions[i];
    const color = colorData[i];
    context.fillStyle = colorToString(color);
    context.beginPath();
    context.arc(x, y, 9, 0, 2 * Math.PI);
    context.fill();
  }
};

function App() {
  const canvasRef = React.useRef<HTMLCanvasElement>(null);
  React.useEffect(() => {
    if (canvasRef.current) {
      const computedTree = computeLocations(defaultTree);
      const fn = drawStarsGen(defaultTree.ledCount);
      //const fn = drawBlueSky;
      const canvas = canvasRef.current;
      const context = canvas.getContext('2d');
      if (!context) throw Error('failed to get context');
      drawTree(computedTree, fn, context);

      const id = setInterval(() => {
        drawTree(computedTree, fn, context);
      }, 66);
      return () => {
        clearInterval(id);
      };
    }
  }, [canvasRef]);

  return (
    <div className="container">
      <canvas ref={canvasRef} width={500} height={500}></canvas>
    </div>
  );
}

export default App;
