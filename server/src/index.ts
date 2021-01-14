import SerialPort from 'serialport';
import WebSocket from 'ws';

interface Color {
  r: number;
  g: number;
  b: number;
}

interface Message {
  type: 'SetLeds';
  colors: Color[];
}

const isMessage = (message: unknown): message is Message => {
  return (
    message.hasOwnProperty('type') && message.hasOwnProperty('colors') && Array.isArray((message as Message).colors)
  );
};

const PrepareBuffer = (colors: Color[]): Buffer => {
  const count = colors.length * 3 + 4;
  const buffer = Buffer.alloc(count);
  buffer[0] = 'M'.charCodeAt(0);
  buffer[1] = 'G'.charCodeAt(0);
  buffer[2] = colors.length;
  buffer[count - 1] = '!'.charCodeAt(0);
  for (let i = 0; i < colors.length; ++i) {
    const j = i * 3 + 3;
    buffer[j] = colors[i].r;
    buffer[j + 1] = colors[i].g;
    buffer[j + 2] = colors[i].b;
  }
  return buffer;
};

const main = async () => {
  console.log('hello world');
  const list = await SerialPort.list();
  const port = list.find(x => x.manufacturer.includes('Arduino'));
  if (!port) {
    throw Error('port not found;');
  }
  console.log(`using port ${port.path}`);

  const serial = new SerialPort(port.path, error => {
    if (error) {
      console.log('serial port error', error);
      process.exit(1);
    }
  });

  const wss = new WebSocket.Server({ port: 8080 });

  wss.on('connection', function connection(ws) {
    console.log('new websocket connection');
    ws.on('message', function incoming(message) {
      try {
        const data = JSON.parse(message as string);
        if (!isMessage(data)) {
          console.log(data, message);
          throw Error('object parse failure');
        }
        serial.write(PrepareBuffer(data.colors));
      } catch (error) {
        console.log('error processing websocket message', error, message);
      }
    });
  });

  const message = PrepareBuffer([{ r: 10, g: 20, b: 30 }]);

  console.log(message, serial.write(message));

  console.log('done!');
};

main();
