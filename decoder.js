function getSigned24(input, index) {
    if (input.bytes[index + 2] & 0x80) {
        return -((~((input.bytes[index]) | (input.bytes[index + 1] << 8) | input.bytes[index + 2] << 16) + 1) & 0xffffff);
    } else {
        return (input.bytes[index]) | (input.bytes[index + 1] << 8) | input.bytes[index + 2] << 16;
    }
}

function getSigned16(input, index) {
    if (input.bytes[index + 1] & 0x80) {
        return -((~((input.bytes[index]) | (input.bytes[index + 1] << 8)) + 1) & 0xffff);
    } else {
        return (input.bytes[index]) | (input.bytes[index + 1] << 8);
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
    data.altitude = getSigned16(input, i);
    i += 2;
    data.course = input.bytes[i++] | (input.bytes[i] & 0x01) << 8;
    data.speed = ((input.bytes[i++] >> 1) | (input.bytes[i] & 0x07) << 7) / 10;
    data.sats_in_use = input.bytes[i++] & 0xF8;
    data.hdop = input.bytes[i] / 10;

    return {
        data: data,
        warnings: [],
        errors: []
    };
}
