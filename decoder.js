const LATITUDE_MIN = 0;
const LATITUDE_MAX = (1 << 21) - 1;
const LONGITUDE_MIN = 0;
const LONGITUDE_MAX = (1 << 22) - 1;

function map(value, in_min, in_max, out_min, out_max) {
    const run = in_max - in_min;
    const rise = out_max - out_min;
    const delta = value - in_min;
    return (delta * rise) / run + out_min;
}

function decodeUplink(input) {
    let i = 0;
    let data = {};
    data.time = input.bytes[i++] | input.bytes[i++] << 8 | input.bytes[i++] << 16 | input.bytes[i++] << 24;
    data.time = new Date(data.time * 1000).toISOString();

    data.temperature = (input.bytes[i] & 0x1f) << 1;

    data.latitude = input.bytes[i++] >> 5 | (input.bytes[i++] << 3) | (input.bytes[i++] << 11) | (input.bytes[i] & 0x03) << 19;
    data.latitude = map(data.latitude, LATITUDE_MIN, LATITUDE_MAX, -90, 90);

    data.longitude = input.bytes[i++] >> 2 | (input.bytes[i++] << 6) | (input.bytes[i++] << 14);
    data.longitude = map(data.longitude, LONGITUDE_MIN, LONGITUDE_MAX, -180, 180);

    data.altitude = input.bytes[i++] | (input.bytes[i++] << 8);
    if (data.altitude & 0x8000) {
        data.altitude = -((~data.altitude + 1) & 0x7fff);
    }

    data.course = input.bytes[i++] | (input.bytes[i] & 0x01) << 8;
    data.speed = ((input.bytes[i++] >> 1) | (input.bytes[i] & 0x07) << 7) / 10;
    data.sats_in_use = input.bytes[i++] >> 4;
    data.hdop = input.bytes[i] / 10;

    return {
        data: data,
        warnings: [],
        errors: []
    };
}
