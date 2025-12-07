#
# Generates a binary output (stream of bytes) from Python standard PNG
# and sends it to stdout in an infinite loop.
#
import sys, random
while True:
    random_bytes = random.randbytes(1024)
    sys.stdout.buffer.write(random_bytes)
