function getSigned24(input, index) {
    if (input.bytes[index + 2] & 0x80) {
        return -((~((input.bytes[index]) | (input.bytes[index + 1] << 8) | input.bytes[index + 2] << 16) + 1) & 0xffffff);
    } else {
        return (input.bytes[index]) | (input.bytes[index + 1] << 8) | input.bytes[index + 2] << 16;
    }
}

function decodeUplink(input) {
    let i = 0;
    data = {};
    data.time = input.bytes[i++] | input.bytes[i++] << 8 | input.bytes[i++] << 16 | input.bytes[i++] << 24;
    data.latitude = getSigned24(input, i) / 10000;
    i += 3;
    data.longitude = getSigned24(input, i) / 10000;
    i += 3;
    data.altitude = input.bytes[i++] | input.bytes[i++] << 8;
    data.course = input.bytes[i++] | (input.bytes[i] & 0x01) << 8;
    data.speed = ((input.bytes[i++] >> 1) | input.bytes[i++] << 7) / 10;
    data.sats_in_use = input.bytes[i++];
    data.hdop = input.bytes[i] / 10;

    return {
        data: data,
        warnings: [],
        errors: []
    };
}
