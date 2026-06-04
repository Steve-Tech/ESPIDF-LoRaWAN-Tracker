const LATITUDE_MIN = 0;
const LATITUDE_MAX = (1 << 22) - 1;
const LONGITUDE_MIN = 0;
const LONGITUDE_MAX = (1 << 23) - 1;
const ALTITUDE_MIN = -1024;

const PORTS = {
    1: "GPS",
    2: "DGPS",
    6: "Dead reckoning",
}

function map(value, in_min, in_max, out_min, out_max) {
    const run = in_max - in_min;
    const rise = out_max - out_min;
    const delta = value - in_min;
    return (delta * rise) / run + out_min;
}

function decodeUplink(input) {
    let data = {};
    let warnings = [];
    let errors = [];

    if (input.fPort in PORTS) {
        data.mode = PORTS[input.fPort];
    } else {
        data.mode = "Unknown";
        warnings.push(`Unknown port: ${input.fPort}`);
    }

    let i = 0;

    data.time = input.bytes[i++] | input.bytes[i++] << 8 | input.bytes[i++] << 16 | input.bytes[i++] << 24;
    data.time = new Date(data.time * 1000).toISOString();

    data.temp1 = (input.bytes[i] & 0x1f) << 1;

    data.latitude = (input.bytes[i++] >> 5) | (input.bytes[i++] << 3) | (input.bytes[i++] << 11) | (input.bytes[i] & 0x07) << 19;
    data.latitude = map(data.latitude, LATITUDE_MIN, LATITUDE_MAX, -90, 90);

    data.longitude = (input.bytes[i++] >> 3) | (input.bytes[i++] << 5) | (input.bytes[i++] << 13) | (input.bytes[i] & 0x03) << 21;
    data.longitude = map(data.longitude, LONGITUDE_MIN, LONGITUDE_MAX, -180, 180);

    data.altitude = ((input.bytes[i++] >> 2) | (input.bytes[i++] << 6)) + ALTITUDE_MIN;

    data.heading = input.bytes[i++] | (input.bytes[i] & 0x01) << 8;
    data.speed = ((input.bytes[i++] >> 1) | (input.bytes[i] & 0x07) << 7) / 10;
    data.sats = input.bytes[i++] >> 4;
    data.hdop = input.bytes[i] / 10;

    return { data, warnings, errors };
}
