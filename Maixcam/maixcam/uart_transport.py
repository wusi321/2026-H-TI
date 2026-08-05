def write_all(port, data):
    offset = 0
    length = len(data)
    while offset < length:
        written = port.write(data[offset:])
        if written is None:
            raise OSError("UART write returned no byte count")
        written = int(written)
        if written <= 0 or written > length - offset:
            raise OSError("invalid UART write count: {}".format(written))
        offset += written
    return offset
